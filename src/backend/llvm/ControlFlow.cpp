#include "CodegenContext.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"

namespace kaleidoscope {

Value *CodegenContext::visit(IfExprAST &node) {
    auto &ctx = *this;
    LocationGuard location(*this, node.location());
    Value *CondV = node.Cond->accept(ctx);
    if (!CondV)
        return nullptr;

    CondV = condition(CondV);
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
    PHINode *PN = ctx.Builder->CreatePHI(lowerType(node.type()), 2, "iftmp");

    PN->addIncoming(ThenV, ThenBB);
    PN->addIncoming(ElseV, ElseBB);
    return PN;
}

Value *CodegenContext::visit(ForExprAST &node) {
    auto &ctx = *this;
    LocationGuard location(*this, node.location());
    const auto depth = Debug ? Debug->scopeDepth() : 0;
    auto restoreScopes = llvm::make_scope_exit([&] {
        if (Debug)
            Debug->restoreScopes(depth);
    });
    Function *TheFunction = ctx.Builder->GetInsertBlock()->getParent();
    AllocaInst *Alloca = ctx.CreateEntryBlockAlloca(TheFunction, node.VarName, node.BindingType);

    Value *StartVal = node.Start->accept(ctx);
    if (!StartVal)
        return nullptr;
    if (Debug) {
        Debug->pushScope(node.VariableLocation);
        Debug->emitLocation(*Builder, node.location());
        Debug->declareVariable(Alloca, node.VarName, node.VariableLocation,
                               Builder->GetInsertBlock());
    }
    ctx.Builder->CreateStore(StartVal, Alloca);

    BasicBlock *LoopBB = BasicBlock::Create(*ctx.TheContext, "loop", TheFunction);

    ctx.Builder->CreateBr(LoopBB);
    ctx.Builder->SetInsertPoint(LoopBB);

    AllocaInst *OldVal = ctx.NamedValues[node.VarName];
    ctx.NamedValues[node.VarName] = Alloca;
    auto restoreBinding = llvm::make_scope_exit([&] {
        if (OldVal)
            ctx.NamedValues[node.VarName] = OldVal;
        else
            ctx.NamedValues.erase(node.VarName);
    });

    if (!node.Body->accept(ctx))
        return nullptr;

    Value *StepVal = nullptr;
    if (node.Step) {
        StepVal = node.Step->accept(ctx);
        if (!StepVal)
            return nullptr;
    } else {
        StepVal = scalarConstant(1, node.BindingType);
    }

    Value *EndCond = node.End->accept(ctx);
    if (!EndCond)
        return nullptr;

    Value *CurVar =
        ctx.Builder->CreateLoad(Alloca->getAllocatedType(), Alloca, node.VarName.c_str());
    Value *NextVar = node.BindingType == Type::I32
                         ? Builder->CreateAdd(CurVar, StepVal, "nextvar")
                         : Builder->CreateFAdd(CurVar, StepVal, "nextvar");
    ctx.Builder->CreateStore(NextVar, Alloca);

    EndCond = condition(EndCond);

    BasicBlock *AfterBB = BasicBlock::Create(*ctx.TheContext, "afterloop", TheFunction);

    ctx.Builder->CreateCondBr(EndCond, LoopBB, AfterBB);
    ctx.Builder->SetInsertPoint(AfterBB);

    return Constant::getNullValue(llvm::Type::getDoubleTy(*ctx.TheContext));
}

} // namespace kaleidoscope
