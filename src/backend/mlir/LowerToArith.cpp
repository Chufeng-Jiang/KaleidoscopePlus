#include "KaleDialect.h"
#include "Passes.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

namespace kaleidoscope::mlir_backend {
namespace {
struct ConstantLowering : mlir::OpConversionPattern<ConstantOp> {
    using OpConversionPattern::OpConversionPattern;
    mlir::LogicalResult matchAndRewrite(ConstantOp op, OpAdaptor adaptor,
                                        mlir::ConversionPatternRewriter &rewriter) const override {
        rewriter.replaceOpWithNewOp<mlir::arith::ConstantOp>(op, adaptor.getValue());
        return mlir::success();
    }
};

template <typename SourceOp, typename FloatOp, typename IntOp>
struct BinaryLowering : mlir::OpConversionPattern<SourceOp> {
    using mlir::OpConversionPattern<SourceOp>::OpConversionPattern;
    mlir::LogicalResult matchAndRewrite(SourceOp op, typename SourceOp::Adaptor adaptor,
                                        mlir::ConversionPatternRewriter &rewriter) const override {
        // Use remapped operands: their producers may already have been replaced.
        if (op.getResult().getType().isF64())
            rewriter.template replaceOpWithNewOp<FloatOp>(op, adaptor.getLhs(), adaptor.getRhs());
        else
            rewriter.template replaceOpWithNewOp<IntOp>(op, adaptor.getLhs(), adaptor.getRhs());
        return mlir::success();
    }
};

struct LowerKaleToArithPass
    : mlir::PassWrapper<LowerKaleToArithPass, mlir::OperationPass<mlir::ModuleOp>> {
    MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerKaleToArithPass)
    llvm::StringRef getArgument() const final { return "kale-to-arith"; }
    llvm::StringRef getDescription() const final {
        return "Convert Kale scalar operations to arith, retaining func structure";
    }
    void getDependentDialects(mlir::DialectRegistry &registry) const override {
        registry.insert<mlir::arith::ArithDialect, mlir::func::FuncDialect>();
    }
    void runOnOperation() override {
        mlir::ConversionTarget target(getContext());
        target.addLegalOp<mlir::ModuleOp>();
        target.addLegalDialect<mlir::arith::ArithDialect, mlir::func::FuncDialect>();
        target.addLegalDialect<mlir::scf::SCFDialect, mlir::memref::MemRefDialect, mlir::cf::ControlFlowDialect>();
        target.addIllegalDialect<KaleDialect>();
        mlir::RewritePatternSet patterns(&getContext());
        patterns
            .add<ConstantLowering, BinaryLowering<AddOp, mlir::arith::AddFOp, mlir::arith::AddIOp>,
                 BinaryLowering<SubOp, mlir::arith::SubFOp, mlir::arith::SubIOp>,
                 BinaryLowering<MulOp, mlir::arith::MulFOp, mlir::arith::MulIOp>>(&getContext());
        // Types stay f64/i32/i1: this conversion does not need a TypeConverter.
        if (mlir::failed(mlir::applyFullConversion(getOperation(), target, std::move(patterns))))
            signalPassFailure();
    }
};
} // namespace
std::unique_ptr<mlir::Pass> createLowerKaleToArithPass() {
    return std::make_unique<LowerKaleToArithPass>();
}
void registerKalePasses() { mlir::PassRegistration<LowerKaleToArithPass>(); }
} // namespace kaleidoscope::mlir_backend
