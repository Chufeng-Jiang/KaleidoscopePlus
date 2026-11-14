#include "backend/mlir/MLIRGen.h"
#include "backend/mlir/KaleDialect.h"
#include "backend/mlir/Passes.h"
#include "backend/mlir/Export.h"
#include "kaleidoscope/support/Diagnostics.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
    using namespace kaleidoscope::mlir_backend;
    std::string mode = "kale", filename, output = "-", kernel;
    bool inputMLIR = false, translateOnly = false, selectedMode = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i], nextMode;
        if (arg == "--help") {
            std::cout
                << "Usage: kale-mlir [mode] [-o output] source.kale\n"
                   "Modes: --lower-to-arith, --emit-llvm-dialect, --emit-llvm, --emit-object,\n"
                   "       --emit-ptx --gpu-map function (f64,f64,f64 -> f64, sm_86)\n"
                   "#acc sources infer the PTX entry and export kacc_<function> host wrappers.\n"
                   "Default: emit kale MLIR. --input-mlir reads MLIR instead of source.\n"
                   "--translate-only --input-mlir --emit-llvm tests LLVM IR translation "
                   "directly.\n";
            return 0;
        }
        if (arg == "--lower-to-arith")
            nextMode = "arith";
        else if (arg == "--emit-llvm-dialect")
            nextMode = "llvm-dialect";
        else if (arg == "--emit-llvm")
            nextMode = "llvm";
        else if (arg == "--emit-object")
            nextMode = "object";
        else if (arg == "--emit-ptx")
            nextMode = "ptx";
        else if ((arg == "-o" || arg == "--gpu-map") && i + 1 < argc) {
            if (arg == "-o")
                output = argv[++i];
            else
                kernel = argv[++i];
        } else if (arg == "--input-mlir")
            inputMLIR = true;
        else if (arg == "--translate-only")
            translateOnly = true;
        else if (!arg.empty() && arg[0] != '-' && filename.empty())
            filename = arg;
        else {
            std::cerr << "Invalid argument: " << arg << '\n';
            return 1;
        }
        if (!nextMode.empty()) {
            if (selectedMode) {
                std::cerr << "Select exactly one emission mode\n";
                return 1;
            }
            mode = nextMode;
            selectedMode = true;
        }
    }
    if (filename.empty() || (mode == "object" && output == "-") ||
        (translateOnly && (!inputMLIR || mode != "llvm")) || (mode != "ptx" && !kernel.empty())) {
        std::cerr << "Invalid option combination; use --help (objects require -o)\n";
        return 1;
    }
    std::ifstream input(filename);
    if (!input) {
        std::cerr << "Cannot open " << filename << '\n';
        return 1;
    }
    mlir::DialectRegistry registry;
    registry.insert<KaleDialect, mlir::func::FuncDialect, mlir::arith::ArithDialect,
                    mlir::scf::SCFDialect, mlir::memref::MemRefDialect,
                    mlir::cf::ControlFlowDialect, mlir::LLVM::LLVMDialect>();
    mlir::MLIRContext context(registry);
    kaleidoscope::SourceManager source;
    kaleidoscope::Diagnostics diagnostics(std::cerr, &source);
    diagnostics.setSourceFile(filename);
    auto module = inputMLIR ? mlir::parseSourceFile<mlir::ModuleOp>(filename, &context)
                            : generateMLIR(input, filename, context, diagnostics, source);
    if (!module)
        return 1;
    if (mode == "ptx") {
        if (auto plan =
                module->getOperation()->getAttrOfType<mlir::DictionaryAttr>("kale.acc_plan")) {
            auto entry = plan.getAs<mlir::StringAttr>("element");
            if (!entry || (!kernel.empty() && kernel != entry.getValue())) {
                std::cerr << "PTX entry conflicts with #acc plan\n";
                return 1;
            }
            kernel = entry.getValue().str();
        }
        if (kernel.empty()) {
            std::cerr << "PTX requires --gpu-map or a validated #acc directive\n";
            return 1;
        }
    }
    if (mode != "kale" && !translateOnly) {
        mlir::PassManager manager(&context);
        manager.addPass(createLowerKaleToArithPass());
        if (mode != "arith")
            addLLVMStages(manager);
        if (mlir::failed(manager.run(*module)))
            return 1;
    }
    std::string data;
    if (mode == "kale" || mode == "arith" || mode == "llvm-dialect") {
        llvm::raw_string_ostream stream(data);
        module->print(stream);
        stream << '\n';
    } else if (!exportLLVM(*module, mode, kernel, data))
        return 1;
    return writeOutput(filename, output, data) ? 0 : 1;
}
