#include "CodegenContext.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"

namespace kaleidoscope {
Value *CodegenContext::getVariableStorage(const std::string &name) {
    auto local = NamedValues.find(name);
    if (local != NamedValues.end() && local->second)
        return local->second;
    if (!GlobalNames.count(name))
        return nullptr;
    if (auto *global = TheModule->getNamedGlobal(name))
        return global;
    // A prior JIT module owns the storage; subsequent modules only reference it.
    return new GlobalVariable(*TheModule, Type::getDoubleTy(*TheContext), false,
                              GlobalValue::ExternalLinkage, nullptr, name);
}

GlobalVariable *CodegenContext::generate(const GlobalAST &node) {
    auto *global = TheModule->getNamedGlobal(node.Name);
    if (!global) {
        global = new GlobalVariable(*TheModule, Type::getDoubleTy(*TheContext), false,
                                    GlobalValue::ExternalLinkage, nullptr, node.Name);
    }
    if (!node.External) {
        global->setInitializer(ConstantFP::get(*TheContext, APFloat(node.Initializer)));
        if (Debug)
            Debug->declareGlobal(*global, node.location());
    }
    GlobalNames.insert(node.Name);
    return global;
}
} // namespace kaleidoscope
