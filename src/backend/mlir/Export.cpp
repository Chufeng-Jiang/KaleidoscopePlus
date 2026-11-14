#include "Export.h"
#include "backend/llvm/Optimizer.h"
#include "mlir/Target/LLVMIR/Export.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IntrinsicsNVPTX.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include <filesystem>

namespace kaleidoscope::mlir_backend {
namespace {
// Lower the validated plan into a status-returning host wrapper.
bool addAccHostEntry(llvm::Module &m, llvm::StringRef name, llvm::StringRef elementName) {
    using namespace llvm;
    auto &ctx = m.getContext();
    auto *element = m.getFunction(elementName);
    auto *vector = m.getFunction(("_mlir_ciface_" + name).str());
    auto *ptr = PointerType::get(ctx, 0);
    auto *f64 = Type::getDoubleTy(ctx);
    auto *i32 = Type::getInt32Ty(ctx);
    auto *elementType = FunctionType::get(f64, {f64, f64, f64}, false);
    auto *vectorType = FunctionType::get(f64, {ptr, ptr, ptr, f64}, false);
    if (!element || element->isDeclaration() || element->getFunctionType() != elementType ||
        !vector || vector->isDeclaration() || vector->getFunctionType() != vectorType) {
        errs() << "Invalid #acc plan: missing compatible element or array function\n";
        return false;
    }
    auto *runtimeType = FunctionType::get(i32, {ptr, ptr, ptr, ptr, f64, ptr}, false);
    if (auto *existing = m.getFunction("krt_acc_map"))
        if (existing->getFunctionType() != runtimeType || !existing->isDeclaration()) {
            errs() << "Conflicting krt_acc_map runtime symbol\n";
            return false;
        }
    auto runtime = m.getOrInsertFunction("krt_acc_map", runtimeType);
    auto entryName = ("kacc_" + name).str();
    if (m.getNamedValue(entryName)) {
        errs() << "Conflicting #acc entry symbol\n";
        return false;
    }
    auto *entry = Function::Create(FunctionType::get(i32, {ptr, ptr, ptr, ptr, f64}, false),
                                   GlobalValue::ExternalLinkage, entryName, m);
    IRBuilder<> b(BasicBlock::Create(ctx, "entry", entry));
    b.CreateRet(b.CreateCall(runtime, {entry->getArg(0), entry->getArg(1), entry->getArg(2),
                                       entry->getArg(3), entry->getArg(4), element}));
    return true;
}

// Explicit map ABI: out[i] = element(x[i], y[i], a), with n guarding tail threads.
// Arithmetic is compiled from the user's Kaleidoscope function, not CUDA source.
bool addMapKernel(llvm::Module &m, const std::string &name) {
    using namespace llvm;
    auto *element = m.getFunction(name);
    auto &ctx = m.getContext();
    auto *f64 = Type::getDoubleTy(ctx);
    if (!element || element->isDeclaration() || element->arg_size() != 3 ||
        element->getReturnType() != f64) {
        errs() << "GPU map entry must be a defined (double,double,double)->double function\n";
        return false;
    }
    for (auto &a : element->args())
        if (a.getType() != f64) {
            errs() << "GPU map arguments must be double\n";
            return false;
        }
    // Export only the kernel and its reachable helpers.
    for (auto &f : m)
        if (!f.isDeclaration())
            f.setLinkage(GlobalValue::InternalLinkage);
    auto *ptr = PointerType::get(ctx, 0);
    auto *i64 = Type::getInt64Ty(ctx);
    auto *ft = FunctionType::get(Type::getVoidTy(ctx), {ptr, ptr, ptr, f64, i64}, false);
    if (m.getFunction("kale_map")) {
        errs() << "Reserved GPU symbol: kale_map\n";
        return false;
    }
    auto *kernel = Function::Create(ft, GlobalValue::ExternalLinkage, "kale_map", m);
    kernel->setCallingConv(CallingConv::PTX_Kernel);
    IRBuilder<> b(BasicBlock::Create(ctx, "entry", kernel));
    auto *tid =
        b.CreateCall(Intrinsic::getOrInsertDeclaration(&m, Intrinsic::nvvm_read_ptx_sreg_tid_x));
    auto *bid =
        b.CreateCall(Intrinsic::getOrInsertDeclaration(&m, Intrinsic::nvvm_read_ptx_sreg_ctaid_x));
    auto *ntid =
        b.CreateCall(Intrinsic::getOrInsertDeclaration(&m, Intrinsic::nvvm_read_ptx_sreg_ntid_x));
    auto *index = b.CreateAdd(b.CreateMul(b.CreateZExt(bid, i64), b.CreateZExt(ntid, i64)),
                              b.CreateZExt(tid, i64));
    auto *body = BasicBlock::Create(ctx, "body", kernel);
    auto *done = BasicBlock::Create(ctx, "done", kernel);
    b.CreateCondBr(b.CreateICmpULT(index, kernel->getArg(4)), body, done);
    b.SetInsertPoint(body);
    auto *x = b.CreateLoad(f64, b.CreateGEP(f64, kernel->getArg(0), index));
    auto *y = b.CreateLoad(f64, b.CreateGEP(f64, kernel->getArg(1), index));
    auto *v = b.CreateCall(element, {x, y, kernel->getArg(3)});
    b.CreateStore(v, b.CreateGEP(f64, kernel->getArg(2), index));
    b.CreateBr(done);
    b.SetInsertPoint(done);
    b.CreateRetVoid();
    return true;
}
} // namespace
bool exportLLVM(mlir::ModuleOp module, const std::string &mode, const std::string &kernel,
                std::string &output) {
    using namespace llvm;
    mlir::registerBuiltinDialectTranslation(*module.getContext());
    mlir::registerLLVMDialectTranslation(*module.getContext());
    LLVMContext context;
    auto ir = mlir::translateModuleToLLVMIR(module, context, "Kaleidoscope");
    if (!ir) {
        errs() << "LLVM IR translation failed\n";
        return false;
    }
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmPrinters();
    bool gpu = mode == "ptx";
    Triple triple(gpu ? "nvptx64-nvidia-cuda" : sys::getDefaultTargetTriple());
    std::string error;
    auto *target = TargetRegistry::lookupTarget(triple, error);
    if (!target) {
        errs() << error << '\n';
        return false;
    }
    TargetOptions options;
    std::unique_ptr<TargetMachine> machine(target->createTargetMachine(
        triple, gpu ? "sm_86" : "generic", gpu ? "+ptx75" : "", options,
        gpu ? std::optional<Reloc::Model>{} : std::optional<Reloc::Model>{Reloc::PIC_}));
    if (!machine) {
        errs() << "Cannot create target machine\n";
        return false;
    }
    ir->setTargetTriple(triple);
    ir->setDataLayout(machine->createDataLayout());
    if (!gpu) {
        if (auto plan = module->getAttrOfType<mlir::DictionaryAttr>("kale.acc_plan")) {
            auto fn = plan.getAs<mlir::StringAttr>("function");
            auto element = plan.getAs<mlir::StringAttr>("element");
            if (!fn || !element || !addAccHostEntry(*ir, fn.getValue(), element.getValue()))
                return false;
        }
    }
    if (gpu && !addMapKernel(*ir, kernel))
        return false;
    if (!optimizeModule(*ir, machine.get(), 2)) {
        errs() << "LLVM verification failed\n";
        return false;
    }
    if (gpu) {
        for (auto &f : *ir)
            if (f.isDeclaration() && !f.isIntrinsic() && !f.use_empty()) {
                errs() << "GPU map has unsupported external dependency: " << f.getName() << '\n';
                return false;
            }
    }
    if (mode == "llvm") {
        raw_string_ostream stream(output);
        ir->print(stream, nullptr);
        return true;
    }
    SmallVector<char, 0> bytes;
    raw_svector_ostream stream(bytes);
    legacy::PassManager passes;
    if (machine->addPassesToEmitFile(passes, stream, nullptr,
                                     gpu ? CodeGenFileType::AssemblyFile
                                         : CodeGenFileType::ObjectFile)) {
        errs() << "Target cannot emit requested output\n";
        return false;
    }
    passes.run(*ir);
    output.assign(bytes.begin(), bytes.end());
    return true;
}
bool writeOutput(const std::string &input, const std::string &path, const std::string &data) {
    using namespace llvm;
    if (path == "-") {
        outs().write(data.data(), data.size());
        outs().flush();
        if (outs().has_error()) {
            outs().clear_error();
            return false;
        }
        return true;
    }
    std::error_code ec;
    auto in = std::filesystem::weakly_canonical(input, ec);
    if (ec) {
        errs() << ec.message() << '\n';
        return false;
    }
    auto out = std::filesystem::weakly_canonical(path, ec);
    if (ec || in == out || sys::fs::equivalent(input, path)) {
        errs() << "Invalid output path or output aliases input\n";
        return false;
    }
    if (std::filesystem::exists(out, ec) && !std::filesystem::is_regular_file(out, ec)) {
        errs() << "Output must be a regular file\n";
        return false;
    }
    SmallString<256> temporary;
    int fd;
    ec = sys::fs::createUniqueFile(path + ".tmp-%%%%%%", fd, temporary);
    if (ec) {
        errs() << "Cannot create output: " << ec.message() << '\n';
        return false;
    }
    raw_fd_ostream stream(fd, true);
    stream.write(data.data(), data.size());
    stream.close();
    if (stream.has_error()) {
        errs() << "Cannot write output\n";
        stream.clear_error();
        sys::fs::remove(temporary);
        return false;
    }
    ec = sys::fs::rename(temporary, path);
    if (ec) {
        sys::fs::remove(temporary);
        errs() << ec.message() << '\n';
        return false;
    }
    return true;
}
} // namespace kaleidoscope::mlir_backend
