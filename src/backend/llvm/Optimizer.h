#pragma once
namespace llvm {
class Module;
class TargetMachine;
} // namespace llvm
namespace kaleidoscope {
bool optimizeModule(llvm::Module &module, llvm::TargetMachine *target, unsigned level);
}
