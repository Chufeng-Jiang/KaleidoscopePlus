#pragma once
#include "mlir/Pass/Pass.h"
#include <memory>
namespace kaleidoscope::mlir_backend {
std::unique_ptr<mlir::Pass> createLowerKaleToArithPass();
void registerKalePasses();
} // namespace kaleidoscope::mlir_backend

namespace mlir {
class PassManager;
}
namespace kaleidoscope::mlir_backend {
std::unique_ptr<mlir::Pass> createLowerToLLVMPass();
void addLLVMStages(mlir::PassManager &);
void registerLLVMStagePasses();
} // namespace kaleidoscope::mlir_backend
