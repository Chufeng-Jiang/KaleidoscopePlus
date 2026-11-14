#include "backend/mlir/KaleDialect.h"
#include "backend/mlir/Passes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/DialectConversion.h"
#include "llvm/Support/raw_ostream.h"

int main() {
    mlir::DialectRegistry registry;
    registry.insert<kaleidoscope::mlir_backend::KaleDialect, mlir::arith::ArithDialect,
                    mlir::func::FuncDialect>();
    mlir::MLIRContext context(registry);
    const char *input = R"mlir(
        module {
          func.func @sum(%x: f64) -> f64 {
            %0 = kale.add %x, %x : f64
            return %0 : f64
          }
        })mlir";
    auto module = mlir::parseSourceString<mlir::ModuleOp>(input, &context);
    if (!module)
        return 1;
    mlir::ConversionTarget target(context);
    target.addLegalOp<mlir::ModuleOp>();
    target.addLegalDialect<mlir::arith::ArithDialect, mlir::func::FuncDialect>();
    target.addIllegalDialect<kaleidoscope::mlir_backend::KaleDialect>();
    // Deliberately leave out AddOp's rewrite. Legal IR is not necessarily legal
    // for this conversion target; the conversion must report failure.
    bool diagnosed = false;
    mlir::ScopedDiagnosticHandler handler(&context, [&](mlir::Diagnostic &diag) {
        std::string message;
        llvm::raw_string_ostream stream(message);
        diag.print(stream);
        diagnosed |= message.find("failed to legalize") != std::string::npos;
        return mlir::success();
    });
    mlir::RewritePatternSet emptyPatterns(&context);
    if (mlir::succeeded(mlir::applyFullConversion(*module, target, std::move(emptyPatterns))) ||
        !diagnosed)
        return 1;
    // The same input succeeds with the production pass's complete pattern set.
    auto complete = mlir::parseSourceString<mlir::ModuleOp>(input, &context);
    mlir::PassManager manager(&context);
    manager.addPass(kaleidoscope::mlir_backend::createLowerKaleToArithPass());
    if (!complete || mlir::failed(manager.run(*complete)))
        return 1;
    bool residual = false;
    complete->walk(
        [&](mlir::Operation *op) { residual |= op->getName().getDialectNamespace() == "kale"; });
    return residual ? 1 : 0;
}
