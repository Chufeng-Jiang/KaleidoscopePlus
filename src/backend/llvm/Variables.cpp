#include "CodegenContext.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"

namespace kaleidoscope {

AllocaInst *CodegenContext::CreateEntryBlockAlloca(Function *TheFunction, StringRef VarName,
                                                   Type type) {
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());

    return TmpB.CreateAlloca(lowerType(type), nullptr, VarName);
}

Value *CodegenContext::visit(VariableExprAST &node) {
    auto &ctx = *this;
    LocationGuard location(*this, node.location());
    Value *A = ctx.getVariableStorage(node.Name);
    if (!A)
        return ctx.LogErrorV("Unknown variable name");
    return ctx.Builder->CreateLoad(lowerType(node.type()), A, node.Name.c_str());
}

Value *CodegenContext::visit(VarExprAST &node) {
    auto &ctx = *this;
    LocationGuard location(*this, node.location());
    const auto depth = Debug ? Debug->scopeDepth() : 0;
    auto restoreScopes = llvm::make_scope_exit([&] {
        if (Debug)
            Debug->restoreScopes(depth);
    });
    std::vector<AllocaInst *> OldBindings;
    auto restoreBindings = llvm::make_scope_exit([&] {
        for (std::size_t i = OldBindings.size(); i > 0; --i) {
            const auto &name = node.VarNames[i - 1].first;
            if (OldBindings[i - 1])
                NamedValues[name] = OldBindings[i - 1];
            else
                NamedValues.erase(name);
        }
    });
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
            InitVal = scalarConstant(0, node.BindingTypes[i]);
        }

        AllocaInst *Alloca = ctx.CreateEntryBlockAlloca(TheFunction, VarName, node.BindingTypes[i]);
        if (Debug) {
            auto loc =
                i < node.VariableLocations.size() ? node.VariableLocations[i] : node.location();
            Debug->pushScope(loc);
            Debug->emitLocation(*Builder, loc);
            Debug->declareVariable(Alloca, VarName, loc, Builder->GetInsertBlock());
        }
        ctx.Builder->CreateStore(InitVal, Alloca);

        OldBindings.push_back(ctx.NamedValues[VarName]);
        ctx.NamedValues[VarName] = Alloca;
    }

    Value *BodyVal = node.Body->accept(ctx);
    if (!BodyVal)
        return nullptr;

    return BodyVal;
}

Value *CodegenContext::getVariableStorage(const std::string &name) {
    auto local = NamedValues.find(name);
    if (local != NamedValues.end() && local->second)
        return local->second;
    if (!GlobalTypes.count(name))
        return nullptr;
    if (auto *global = TheModule->getNamedGlobal(name))
        return global;
    // A prior JIT module owns the storage; subsequent modules only reference it.
    return new GlobalVariable(*TheModule, lowerType(GlobalTypes.at(name)), false,
                              GlobalValue::ExternalLinkage, nullptr, name);
}

GlobalVariable *CodegenContext::generate(const GlobalAST &node) {
    auto *global = TheModule->getNamedGlobal(node.Name);
    if (!global) {
        global = new GlobalVariable(*TheModule, lowerType(node.type()), false,
                                    GlobalValue::ExternalLinkage, nullptr, node.Name);
    }
    if (!node.External) {
        global->setInitializer(scalarConstant(node.Initializer, node.type()));
        if (Debug)
            Debug->declareGlobal(*global, node.location());
    }
    GlobalTypes.insert_or_assign(node.Name, node.type());
    return global;
}

} // namespace kaleidoscope
