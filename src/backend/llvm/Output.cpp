#include "Output.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include <filesystem>
namespace kaleidoscope {
bool emitOutput(llvm::Module &module, llvm::TargetMachine &target, bool ir,
                const std::string &input, const std::string &output) {
    using namespace llvm;
    if (output == "-") {
        module.print(outs(), nullptr);
        outs().flush();
        if (outs().has_error()) {
            errs() << "Could not write IR to stdout: " << outs().error().message() << '\n';
            outs().clear_error();
            return false;
        }
        return true;
    }
    std::error_code error;
    auto in = std::filesystem::weakly_canonical(input, error);
    if (error) {
        errs() << error.message() << '\n';
        return false;
    }
    auto out = std::filesystem::weakly_canonical(output, error);
    if (error) {
        errs() << error.message() << '\n';
        return false;
    }
    if (in == out || sys::fs::equivalent(input, output)) {
        errs() << "Output path must not overwrite the input file\n";
        return false;
    }
    // Reject devices, directories, and special files rather than replacing them.
    if (std::filesystem::exists(out, error) && !std::filesystem::is_regular_file(out, error)) {
        errs() << "Output path must be a regular file\n";
        return false;
    }
    SmallString<256> temporary;
    int fd;
    error = sys::fs::createUniqueFile(output + ".tmp-%%%%%%", fd, temporary);
    if (error) {
        errs() << "Could not open output file: " << error.message() << '\n';
        return false;
    }
    auto cleanup = make_scope_exit([&] { sys::fs::remove(temporary); });
    raw_fd_ostream stream(fd, true);
    if (ir)
        module.print(stream, nullptr);
    else {
        legacy::PassManager passes;
        if (target.addPassesToEmitFile(passes, stream, nullptr, CodeGenFileType::ObjectFile)) {
            errs() << "Could not emit object of this type\n";
            return false;
        }
        passes.run(module);
    }
    stream.close();
    if (stream.has_error()) {
        errs() << "Could not write output file: " << stream.error().message() << '\n';
        stream.clear_error();
        return false;
    }
    error = sys::fs::rename(temporary, output);
    if (error) {
        errs() << "Could not replace output file: " << error.message() << '\n';
        return false;
    }
    return true;
}
} // namespace kaleidoscope
