#include "KaleDialect.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/OpImplementation.h"
// Generated declarations must follow the MLIR includes.
#include "KaleDialect.cpp.inc"
#define GET_OP_CLASSES
#include "KaleOps.cpp.inc"

namespace kaleidoscope::mlir_backend {
void KaleDialect::initialize() {
    addOperations<
#define GET_OP_LIST
#include "KaleOps.cpp.inc"
        >();
}
mlir::LogicalResult ConstantOp::verify() {
    if (getValue().getType() != getResult().getType())
        return emitOpError("value attribute type must match result type");
    return mlir::success();
}
} // namespace kaleidoscope::mlir_backend
