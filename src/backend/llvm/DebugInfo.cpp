#include "DebugInfo.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Module.h"
#include <filesystem>

namespace kaleidoscope {
using namespace llvm;

DebugInfo::DebugInfo(Module &module, const std::string &sourceFile) : Builder(module) {
    auto path = std::filesystem::absolute(sourceFile).lexically_normal();
    File = Builder.createFile(path.filename().string(), path.parent_path().string());
    module.addModuleFlag(Module::Warning, "Debug Info Version", DEBUG_METADATA_VERSION);
    module.addModuleFlag(Module::Warning, "Dwarf Version", 4);
    Unit = Builder.createCompileUnit(dwarf::DW_LANG_C, File, "Kaleidoscope Compiler", false, "", 0);
    DoubleType = Builder.createBasicType("double", 64, dwarf::DW_ATE_float);
    IntegerType = Builder.createBasicType("i32", 32, dwarf::DW_ATE_signed);
    BooleanType = Builder.createBasicType("bool", 8, dwarf::DW_ATE_boolean);
}

llvm::DIType *DebugInfo::debugType(llvm::Type *type) {
    if (type->isDoubleTy())
        return DoubleType;
    if (type->isIntegerTy(32))
        return IntegerType;
    if (type->isIntegerTy(1))
        return BooleanType;
    llvm_unreachable("Unsupported debug type");
}

void DebugInfo::beginFunction(Function &function, SourceLocation location) {
    std::vector<Metadata *> types{debugType(function.getReturnType())};
    for (auto &arg : function.args())
        types.push_back(debugType(arg.getType()));
    auto type = Builder.createSubroutineType(Builder.getOrCreateTypeArray(types));
    auto scope = Builder.createFunction(File, function.getName(), function.getName(), File,
                                        location.Line, type, location.Line, DINode::FlagPrototyped,
                                        DISubprogram::SPFlagDefinition);
    function.setSubprogram(scope);
    Scopes = {scope};
}

void DebugInfo::endFunction() {
    if (!Scopes.empty())
        Builder.finalizeSubprogram(Scopes.front()->getSubprogram());
    Scopes.clear();
}

void DebugInfo::emitLocation(IRBuilder<> &builder, SourceLocation location) {
    if (!location.Line || Scopes.empty()) {
        builder.SetCurrentDebugLocation(DebugLoc());
        return;
    }
    builder.SetCurrentDebugLocation(
        DILocation::get(File->getContext(), location.Line, location.Column, Scopes.back()));
}

void DebugInfo::pushScope(SourceLocation location) {
    Scopes.push_back(
        Builder.createLexicalBlock(Scopes.back(), File, location.Line, location.Column));
}

void DebugInfo::declareVariable(AllocaInst *storage, StringRef name, SourceLocation location,
                                BasicBlock *block, unsigned argument) {
    auto scope = Scopes.back();
    auto *type = debugType(storage->getAllocatedType());
    auto variable = argument
                        ? Builder.createParameterVariable(scope, name, argument, File,
                                                          location.Line, type, true)
                        : Builder.createAutoVariable(scope, name, File, location.Line, type, true);
    Builder.insertDeclare(
        storage, variable, Builder.createExpression(),
        DILocation::get(File->getContext(), location.Line, location.Column, scope), block);
}

void DebugInfo::declareGlobal(GlobalVariable &variable, SourceLocation location) {
    auto *expression = Builder.createGlobalVariableExpression(
        Unit, variable.getName(), variable.getName(), File, location.Line,
        debugType(variable.getValueType()), false, true);
    variable.addDebugInfo(expression);
}

void DebugInfo::finalize() { Builder.finalize(); }
} // namespace kaleidoscope
