#include "backend/mlir/KaleDialect.h"
#include "backend/mlir/Passes.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

int main(int argc, char **argv) {
    mlir::DialectRegistry registry;
    registry.insert<kaleidoscope::mlir_backend::KaleDialect, mlir::arith::ArithDialect,
                    mlir::func::FuncDialect, mlir::scf::SCFDialect, mlir::memref::MemRefDialect,
                    mlir::cf::ControlFlowDialect, mlir::LLVM::LLVMDialect>();
    kaleidoscope::mlir_backend::registerKalePasses();
    kaleidoscope::mlir_backend::registerLLVMStagePasses();
    return mlir::asMainReturnCode(
        mlir::MlirOptMain(argc, argv, "Kaleidoscope dialect conversion exercise\n", registry));
}
