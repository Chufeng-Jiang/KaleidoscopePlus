#pragma once
#include "mlir/IR/BuiltinOps.h"
#include <string>
namespace kaleidoscope::mlir_backend {
bool exportLLVM(mlir::ModuleOp module, const std::string &mode, const std::string &kernel,
                std::string &output);
bool writeOutput(const std::string &input, const std::string &path, const std::string &data);
} // namespace kaleidoscope::mlir_backend
