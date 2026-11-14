#include "CodegenContext.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"

namespace kaleidoscope {

Value *CodegenContext::LogErrorV(const char *Str) {
    Diag.error(Str);
    return nullptr;
}

Function *CodegenContext::getFunction(std::string Name) {
    if (auto *F = TheModule->getFunction(Name))
        return F;

    auto FI = FunctionProtos.find(Name);
    if (FI != FunctionProtos.end())
        return generate(*FI->second);

    return nullptr;
}

AllocaInst *CodegenContext::CreateEntryBlockAlloca(Function *TheFunction, StringRef VarName) {
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());

    return TmpB.CreateAlloca(Type::getDoubleTy(*TheContext), nullptr, VarName);
}

Value *CodegenContext::visit(NumberExprAST &node) {
    auto &ctx = *this;
    return ConstantFP::get(*ctx.TheContext, APFloat(node.Val));
}

Value *CodegenContext::visit(VariableExprAST &node) {
    auto &ctx = *this;
    AllocaInst *A = ctx.NamedValues[node.Name];
    if (!A)
        return ctx.LogErrorV("Unknown variable name");
    return ctx.Builder->CreateLoad(A->getAllocatedType(), A, node.Name.c_str());
}

Value *CodegenContext::visit(UnaryExprAST &node) {
    auto &ctx = *this;
    Value *OperandV = node.Operand->accept(ctx);
    if (!OperandV)
        return nullptr;

    Function *F = ctx.getFunction(std::string("unary") + node.Opcode);
    if (!F)
        return ctx.LogErrorV("Unknow unary operator");

    return ctx.Builder->CreateCall(F, OperandV, "unop");
}

Value *CodegenContext::visit(BinaryExprAST &node) {
    auto &ctx = *this;
    if (node.Op == '=') {
        const std::string *LHSE = node.LHS->variableName();
        if (!LHSE)
            return ctx.LogErrorV("destination of '=' must be a variable");

        Value *Val = node.RHS->accept(ctx);
        if (!Val)
            return nullptr;

        Value *Variable = ctx.NamedValues[*LHSE];
        if (!Variable)
            return ctx.LogErrorV("Unknown variable name");

        ctx.Builder->CreateStore(Val, Variable);
        return Val;
    }

    Value *L = node.LHS->accept(ctx);
    Value *R = node.RHS->accept(ctx);
    if (!L || !R)
        return nullptr;

    switch (node.Op) {
    case '+':
        return ctx.Builder->CreateFAdd(L, R, "addtmp");
    case '-':
        return ctx.Builder->CreateFSub(L, R, "subtmp");
    case '*':
        return ctx.Builder->CreateFMul(L, R, "multmp");
    case '<':
        L = ctx.Builder->CreateFCmpULT(L, R, "cmptmp");
        return ctx.Builder->CreateUIToFP(L, Type::getDoubleTy(*ctx.TheContext), "booltmp");
    default:
        break;
    }

    Function *F = ctx.getFunction(std::string("binary") + node.Op);
    if (!F)
        return ctx.LogErrorV("binary operator not found");

    Value *Ops[] = {L, R};
    return ctx.Builder->CreateCall(F, Ops, "binop");
}

Value *CodegenContext::visit(CallExprAST &node) {
    auto &ctx = *this;
    Function *CalleeF = ctx.getFunction(node.Callee);
    if (!CalleeF)
        return ctx.LogErrorV("Unknown function referenced");

    if (CalleeF->arg_size() != node.Args.size())
        return ctx.LogErrorV("Incorrect # arguments passed");

    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = node.Args.size(); i != e; ++i) {
        ArgsV.push_back(node.Args[i]->accept(ctx));
        if (!ArgsV.back())
            return nullptr;
    }

    return ctx.Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Value *CodegenContext::visit(IfExprAST &node) {
    auto &ctx = *this;
    Value *CondV = node.Cond->accept(ctx);
    if (!CondV)
        return nullptr;

    CondV =
        ctx.Builder->CreateFCmpONE(CondV, ConstantFP::get(*ctx.TheContext, APFloat(0.0)), "ifcond");
    Function *TheFunction = ctx.Builder->GetInsertBlock()->getParent();

    BasicBlock *ThenBB = BasicBlock::Create(*ctx.TheContext, "then", TheFunction);
    BasicBlock *ElseBB = BasicBlock::Create(*ctx.TheContext, "else");
    BasicBlock *MergeBB = BasicBlock::Create(*ctx.TheContext, "ifcont");

    ctx.Builder->CreateCondBr(CondV, ThenBB, ElseBB);

    ctx.Builder->SetInsertPoint(ThenBB);
    Value *ThenV = node.Then->accept(ctx);
    if (!ThenV)
        return nullptr;

    ctx.Builder->CreateBr(MergeBB);
    ThenBB = ctx.Builder->GetInsertBlock();

    TheFunction->insert(TheFunction->end(), ElseBB);
    ctx.Builder->SetInsertPoint(ElseBB);
    Value *ElseV = node.Else->accept(ctx);
    if (!ElseV)
        return nullptr;

    ctx.Builder->CreateBr(MergeBB);
    ElseBB = ctx.Builder->GetInsertBlock();

    TheFunction->insert(TheFunction->end(), MergeBB);
    ctx.Builder->SetInsertPoint(MergeBB);
    PHINode *PN = ctx.Builder->CreatePHI(Type::getDoubleTy(*ctx.TheContext), 2, "iftmp");

    PN->addIncoming(ThenV, ThenBB);
    PN->addIncoming(ElseV, ElseBB);
    return PN;
}

Value *CodegenContext::visit(ForExprAST &node) {
    auto &ctx = *this;
    Function *TheFunction = ctx.Builder->GetInsertBlock()->getParent();
    AllocaInst *Alloca = ctx.CreateEntryBlockAlloca(TheFunction, node.VarName);

    Value *StartVal = node.Start->accept(ctx);
    if (!StartVal)
        return nullptr;
    ctx.Builder->CreateStore(StartVal, Alloca);

    BasicBlock *LoopBB = BasicBlock::Create(*ctx.TheContext, "loop", TheFunction);

    ctx.Builder->CreateBr(LoopBB);
    ctx.Builder->SetInsertPoint(LoopBB);

    AllocaInst *OldVal = ctx.NamedValues[node.VarName];
    ctx.NamedValues[node.VarName] = Alloca;

    if (!node.Body->accept(ctx))
        return nullptr;

    Value *StepVal = nullptr;
    if (node.Step) {
        StepVal = node.Step->accept(ctx);
        if (!StepVal)
            return nullptr;
    } else {
        StepVal = ConstantFP::get(*ctx.TheContext, APFloat(1.0));
    }

    Value *EndCond = node.End->accept(ctx);
    if (!EndCond)
        return nullptr;

    Value *CurVar =
        ctx.Builder->CreateLoad(Alloca->getAllocatedType(), Alloca, node.VarName.c_str());
    Value *NextVar = ctx.Builder->CreateFAdd(CurVar, StepVal, "nextvar");
    ctx.Builder->CreateStore(NextVar, Alloca);

    EndCond = ctx.Builder->CreateFCmpONE(EndCond, ConstantFP::get(*ctx.TheContext, APFloat(0.0)),
                                         "loopcond");

    BasicBlock *AfterBB = BasicBlock::Create(*ctx.TheContext, "afterloop", TheFunction);

    ctx.Builder->CreateCondBr(EndCond, LoopBB, AfterBB);
    ctx.Builder->SetInsertPoint(AfterBB);

    if (OldVal)
        ctx.NamedValues[node.VarName] = OldVal;
    else
        ctx.NamedValues.erase(node.VarName);

    return Constant::getNullValue(Type::getDoubleTy(*ctx.TheContext));
}

Value *CodegenContext::visit(VarExprAST &node) {
    auto &ctx = *this;
    std::vector<AllocaInst *> OldBindings;
    Function *TheFunction = ctx.Builder->GetInsertBlock()->getParent();

    for (unsigned i = 0, e = node.VarNames.size(); i != e; ++i) {
        const std::string &VarName = node.VarNames[i].first;
        ExprAST *Init = node.VarNames[i].second.get();

        Value *InitVal;
        if (Init) {
            InitVal = Init->accept(ctx);
            if (!InitVal)
                return nullptr;
        } else {
            InitVal = ConstantFP::get(*ctx.TheContext, APFloat(0.0));
        }

        AllocaInst *Alloca = ctx.CreateEntryBlockAlloca(TheFunction, VarName);
        ctx.Builder->CreateStore(InitVal, Alloca);

        OldBindings.push_back(ctx.NamedValues[VarName]);
        ctx.NamedValues[VarName] = Alloca;
    }

    Value *BodyVal = node.Body->accept(ctx);
    if (!BodyVal)
        return nullptr;

    for (unsigned i = 0, e = node.VarNames.size(); i != e; ++i) {
        ctx.NamedValues[node.VarNames[i].first] = OldBindings[i];
    }

    return BodyVal;
}

Function *CodegenContext::generate(PrototypeAST &node) {
    auto &ctx = *this;
    std::vector<Type *> Doubles(node.Args.size(), Type::getDoubleTy(*ctx.TheContext));
    FunctionType *FT = FunctionType::get(Type::getDoubleTy(*ctx.TheContext), Doubles, false);
    Function *F = Function::Create(FT, Function::ExternalLinkage, node.Name, ctx.TheModule.get());

    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(node.Args[Idx++]);

    return F;
}

Function *CodegenContext::generate(FunctionAST &node) {
    auto &ctx = *this;
    auto &P = *node.Proto;
    ctx.FunctionProtos[node.Proto->getName()] = std::move(node.Proto);
    Function *TheFunction = ctx.getFunction(P.getName());

    if (!TheFunction)
        return nullptr;

    if (P.isBinaryOp())
        ctx.BinopPrecedence[P.getOperatorName()] = P.getBinaryPrecedence();

    BasicBlock *BB = BasicBlock::Create(*ctx.TheContext, "entry", TheFunction);
    ctx.Builder->SetInsertPoint(BB);

    ctx.NamedValues.clear();
    for (auto &Arg : TheFunction->args()) {
        AllocaInst *Alloca = ctx.CreateEntryBlockAlloca(TheFunction, Arg.getName());
        ctx.Builder->CreateStore(&Arg, Alloca);
        ctx.NamedValues[std::string(Arg.getName())] = Alloca;
    }

    if (Value *RetVal = node.Body->accept(ctx)) {
        ctx.Builder->CreateRet(RetVal);
        verifyFunction(*TheFunction);

        ctx.TheFPM->run(*TheFunction, *ctx.TheFAM);

        return TheFunction;
    }

    TheFunction->eraseFromParent();
    if (P.isBinaryOp())
        ctx.BinopPrecedence.erase(P.getOperatorName());
    return nullptr;
}

void CodegenContext::initializeModule() {
    TheContext = std::make_unique<LLVMContext>();
    TheModule = std::make_unique<Module>("Kaleidoscope", *TheContext);
    if (EmitObject) {
        TheModule->setDataLayout(TheTM->createDataLayout());
        TheModule->setTargetTriple(TheTM->getTargetTriple());
    } else
        TheModule->setDataLayout(TheJIT->getDataLayout());
    Builder = std::make_unique<IRBuilder<>>(*TheContext);

    TheFPM = std::make_unique<FunctionPassManager>();
    TheLAM = std::make_unique<LoopAnalysisManager>();
    TheFAM = std::make_unique<FunctionAnalysisManager>();
    TheCGAM = std::make_unique<CGSCCAnalysisManager>();
    TheMAM = std::make_unique<ModuleAnalysisManager>();
    ThePIC = std::make_unique<PassInstrumentationCallbacks>();
    TheSI = std::make_unique<StandardInstrumentations>(*TheContext, true);
    TheSI->registerCallbacks(*ThePIC, TheMAM.get());

    TheFPM->addPass(PromotePass()); // Mem2Reg
    TheFPM->addPass(InstCombinePass());
    TheFPM->addPass(ReassociatePass());
    TheFPM->addPass(GVNPass());
    TheFPM->addPass(SimplifyCFGPass());

    PassBuilder PB;
    PB.registerModuleAnalyses(*TheMAM);
    PB.registerFunctionAnalyses(*TheFAM);
    PB.crossRegisterProxies(*TheLAM, *TheFAM, *TheCGAM, *TheMAM);
}

} // namespace kaleidoscope
