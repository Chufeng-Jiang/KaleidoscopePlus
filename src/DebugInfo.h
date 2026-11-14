#pragma once
#include "kaleidoscope/SourceLocation.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include <string>
#include <vector>

namespace kaleidoscope {
// Owns all DWARF metadata for one module. Used only for -g object compilation.
class DebugInfo {
  public:
    DebugInfo(llvm::Module &module, const std::string &sourceFile);
    void beginFunction(llvm::Function &function, SourceLocation location);
    void endFunction();
    void emitLocation(llvm::IRBuilder<> &builder, SourceLocation location);
    void pushScope(SourceLocation location);
    std::size_t scopeDepth() const { return Scopes.size(); }
    void restoreScopes(std::size_t depth) { Scopes.resize(depth); }
    void declareVariable(llvm::AllocaInst *storage, llvm::StringRef name, SourceLocation location,
                         llvm::BasicBlock *block, unsigned argument = 0);
    void declareGlobal(llvm::GlobalVariable &variable, SourceLocation location);
    void finalize();

  private:
    llvm::DIBuilder Builder;
    llvm::DICompileUnit *Unit;
    llvm::DIFile *File;
    llvm::DIType *DoubleType;
    std::vector<llvm::DILocalScope *> Scopes;
};
} // namespace kaleidoscope
