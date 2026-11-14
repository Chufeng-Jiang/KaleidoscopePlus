#include "Optimizer.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/OptimizationLevel.h"
#include "llvm/Passes/PassBuilder.h"
namespace kaleidoscope {
bool optimizeModule(llvm::Module &module, llvm::TargetMachine *target, unsigned level) {
    using namespace llvm;
    if (verifyModule(module, &errs()))
        return false;
    if (level == 0)
        return true;
    // Managers are local to this module and outlive their proxies in dependency order.
    LoopAnalysisManager loops;
    FunctionAnalysisManager functions;
    CGSCCAnalysisManager cgscc;
    ModuleAnalysisManager modules;
    PassBuilder builder(target);
    builder.registerModuleAnalyses(modules);
    builder.registerCGSCCAnalyses(cgscc);
    builder.registerFunctionAnalyses(functions);
    builder.registerLoopAnalyses(loops);
    builder.crossRegisterProxies(loops, functions, cgscc, modules);
    auto optimization = level == 1   ? OptimizationLevel::O1
                        : level == 2 ? OptimizationLevel::O2
                                     : OptimizationLevel::O3;
    auto pipeline = builder.buildPerModuleDefaultPipeline(optimization);
    pipeline.run(module, modules);
    return !verifyModule(module, &errs());
}
} // namespace kaleidoscope
