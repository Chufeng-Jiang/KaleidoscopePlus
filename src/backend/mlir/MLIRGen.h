#pragma once
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OwningOpRef.h"
#include <istream>
#include <string>
namespace kaleidoscope {
class Diagnostics;
class SourceManager;
namespace mlir_backend {
mlir::OwningOpRef<mlir::ModuleOp> generateMLIR(std::istream &input, const std::string &filename,
                                               mlir::MLIRContext &context, Diagnostics &diagnostics,
                                               SourceManager &source);
}
} // namespace kaleidoscope
