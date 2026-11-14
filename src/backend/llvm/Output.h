#pragma once
#include <string>
namespace llvm {
class Module;
class TargetMachine;
} // namespace llvm
namespace kaleidoscope {
// Emit to a sibling temporary file and replace the destination only after success.
bool emitOutput(llvm::Module &module, llvm::TargetMachine &target, bool ir,
                const std::string &input, const std::string &output);
} // namespace kaleidoscope
