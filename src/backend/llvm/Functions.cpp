#include "CodegenContext.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Verifier.h"

namespace kaleidoscope {

Function *CodegenContext::getFunction(std::string Name) {
    if (auto *F = TheModule->getFunction(Name))
        return F;

    auto FI = FunctionProtos.find(Name);
    if (FI != FunctionProtos.end())
        return generate(*FI->second);

    return nullptr;
}

Function *CodegenContext::generate(PrototypeAST &node) {
    auto &ctx = *this;
    const auto &signature = node.signature();
    std::vector<llvm::Type *> parameters;
    for (auto type : signature.Parameters)
        parameters.push_back(lowerType(type));
    auto *FT = llvm::FunctionType::get(lowerType(signature.Result), parameters, false);
    if (auto *existing = TheModule->getFunction(node.Name)) {
        if (existing->getFunctionType() != FT) {
            Diag.error("Internal error: conflicting lowered function type", node.location());
            return nullptr;
        }
        return existing;
    }
    Function *F = Function::Create(FT, Function::ExternalLinkage, node.Name, ctx.TheModule.get());
    // Match the native C/C++ bool ABI on the supported host target.
    if (signature.Result == Type::Bool)
        F->addRetAttr(Attribute::ZExt);
    for (unsigned i = 0; i < signature.Parameters.size(); ++i)
        if (signature.Parameters[i] == Type::Bool)
            F->addParamAttr(i, Attribute::ZExt);

    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(node.Args[Idx++]);

    return F;
}

Function *CodegenContext::generate(FunctionAST &node) {
    auto &ctx = *this;
    auto &P = *node.Proto;
    if (!isValueType(node.Body->type()) ||
        (P.getName() != "__anon_expr" && !isAssignable(P.signature().Result, node.Body->type()))) {
        Diag.error("Internal error: function body has not passed type analysis", P.location());
        return nullptr;
    }
    Function *TheFunction = generate(P);

    if (!TheFunction)
        return nullptr;

    if (TheFunction->arg_size() != P.Args.size()) {
        Diag.error("Internal error: conflicting lowered function type", P.location());
        return nullptr;
    }
    if (!TheFunction->empty()) {
        Diag.error("Internal error: duplicate lowered function", P.location());
        return nullptr;
    }
    // A definition may rename the parameters from an earlier extern declaration.
    for (auto &Arg : TheFunction->args())
        Arg.setName("");

    bool completed = false;
    auto rollback = llvm::make_scope_exit([&] {
        if (!completed) {
            TheFunction->deleteBody();
            TheFunction->setSubprogram(nullptr);
        }
    });

    BasicBlock *BB = BasicBlock::Create(*ctx.TheContext, "entry", TheFunction);
    ctx.Builder->SetInsertPoint(BB);

    Builder->SetCurrentDebugLocation(DebugLoc());
    if (Debug)
        Debug->beginFunction(*TheFunction, P.location());
    auto finishDebug = llvm::make_scope_exit([&] {
        if (Debug)
            Debug->endFunction();
        Builder->SetCurrentDebugLocation(DebugLoc());
    });
    ctx.NamedValues.clear();
    unsigned argumentIndex = 0;
    for (auto &Arg : TheFunction->args()) {
        Arg.setName(P.Args[argumentIndex]);
        AllocaInst *Alloca = ctx.CreateEntryBlockAlloca(TheFunction, Arg.getName(),
                                                        P.signature().Parameters[argumentIndex]);
        if (Debug) {
            auto loc = argumentIndex < P.ArgumentLocations.size()
                           ? P.ArgumentLocations[argumentIndex]
                           : P.location();
            Debug->declareVariable(Alloca, Arg.getName(), loc, Builder->GetInsertBlock(),
                                   argumentIndex + 1);
        }
        ++argumentIndex;
        ctx.Builder->CreateStore(&Arg, Alloca);
        ctx.NamedValues[std::string(Arg.getName())] = Alloca;
    }

    if (Value *RetVal = node.Body->accept(ctx)) {
        if (Debug)
            Debug->emitLocation(*Builder, node.Body->location());
        // Keep the REPL's double-returning trampoline ABI for all scalar results.
        if (P.getName() == "__anon_expr")
            RetVal = convert(RetVal, Type::Double);
        ctx.Builder->CreateRet(RetVal);
        if (Debug)
            Debug->endFunction();
        if (verifyFunction(*TheFunction, &errs())) {
            Diag.error("Invalid generated function", P.location());
            return nullptr;
        }

        completed = true;
        FunctionProtos[P.getName()] = std::make_unique<PrototypeAST>(P);
        return TheFunction;
    }

    if (Debug)
        Debug->endFunction();
    return nullptr;
}

} // namespace kaleidoscope
