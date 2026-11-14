#include "Passes.h"
#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVM.h"
#include "mlir/Conversion/LLVMCommon/ConversionTarget.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"
#include "mlir/Conversion/MemRefToLLVM/MemRefToLLVM.h"
#include "mlir/Conversion/SCFToControlFlow/SCFToControlFlow.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/DialectConversion.h"
namespace kaleidoscope::mlir_backend {
namespace {
struct LowerToLLVM : mlir::PassWrapper<LowerToLLVM, mlir::OperationPass<mlir::ModuleOp>> {
    MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerToLLVM)
    llvm::StringRef getArgument() const final { return "kale-to-llvm"; }
    llvm::StringRef getDescription() const final {
        return "Convert arith/func/memref/cf to LLVM dialect";
    }
    void getDependentDialects(mlir::DialectRegistry &r) const override {
        r.insert<mlir::LLVM::LLVMDialect>();
    }
    void runOnOperation() override {
        mlir::LLVMTypeConverter converter(&getContext());
        mlir::RewritePatternSet patterns(&getContext());
        mlir::arith::populateArithToLLVMConversionPatterns(converter, patterns);
        mlir::populateFuncToLLVMConversionPatterns(converter, patterns);
        mlir::populateFinalizeMemRefToLLVMConversionPatterns(converter, patterns);
        mlir::cf::populateControlFlowToLLVMConversionPatterns(converter, patterns);
        mlir::cf::populateAssertToLLVMConversionPattern(converter, patterns);
        mlir::LLVMConversionTarget target(getContext());
        target.addLegalOp<mlir::ModuleOp>();
        if (mlir::failed(mlir::applyFullConversion(getOperation(), target, std::move(patterns))))
            signalPassFailure();
    }
};
} // namespace
std::unique_ptr<mlir::Pass> createLowerToLLVMPass() { return std::make_unique<LowerToLLVM>(); }
void registerLLVMStagePasses() {
    mlir::PassRegistration<LowerToLLVM>();
    mlir::PassPipelineRegistration<>(
        "kale-lower-to-llvm",
        "Lower kale, structured control flow, memory, and functions to LLVM dialect",
        [](mlir::OpPassManager &pm) {
            pm.addPass(createLowerKaleToArithPass());
            pm.addPass(mlir::createSCFToControlFlowPass());
            pm.addPass(createLowerToLLVMPass());
        });
}
void addLLVMStages(mlir::PassManager &pm) {
    pm.addPass(mlir::createSCFToControlFlowPass());
    pm.addPass(createLowerToLLVMPass());
}
} // namespace kaleidoscope::mlir_backend
