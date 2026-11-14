#include "CompilerOptions.h"
namespace kaleidoscope {
void printUsage(std::ostream &output) {
    output << "Usage: kale [-g] [-O0|-O1|-O2|-O3] [--emit-llvm] [-o output] [--] [source.kale]\n"
              "No source file: interactive JIT. Default optimization: -O2.\n"
              "-g requires an input file and selects -O0; -g with -O1/-O2/-O3 is rejected.\n"
              "--emit-llvm writes textual LLVM IR; -o - sends IR to stdout.\n"
              "Default output: <source>.o, or <source>.ll with --emit-llvm.\n";
}
bool parseOptions(int argc, char *argv[], CompilerOptions &options, std::ostream &errors) {
    bool positional = false, explicitOptimization = false, explicitOutput = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (!positional && arg == "--")
            positional = true;
        else if (!positional && (arg == "--help" || arg == "-h"))
            options.Help = true;
        else if (!positional && arg == "-g")
            options.Debug = true;
        else if (!positional && arg == "--emit-llvm")
            options.EmitIR = true;
        else if (!positional && arg == "-o") {
            if (++i == argc || std::string(argv[i]).empty()) {
                errors << "-o requires an output path\n";
                return false;
            }
            options.OutputFile = argv[i];
            explicitOutput = true;
        } else if (!positional && arg.size() == 3 && arg.substr(0, 2) == "-O" && arg[2] >= '0' &&
                   arg[2] <= '3') {
            options.Optimization = arg[2] - '0';
            explicitOptimization = true;
        } else if (arg.empty() || (!positional && arg[0] == '-') || !options.InputFile.empty()) {
            errors << "Unexpected argument: " << arg << '\n';
            return false;
        } else
            options.InputFile = arg;
    }
    if (options.Help)
        return true;
    if (options.Debug && explicitOptimization && options.Optimization != 0) {
        errors << "-g requires -O0 for reliable source debugging\n";
        return false;
    }
    if (options.Debug)
        options.Optimization = 0;
    if (options.InputFile.empty() && (options.Debug || options.EmitIR || explicitOutput)) {
        errors << "-g, --emit-llvm or -o requires an input file\n";
        return false;
    }
    if (options.OutputFile == "-" && !options.EmitIR) {
        errors << "-o - is supported only with --emit-llvm\n";
        return false;
    }
    if (!options.InputFile.empty() && !explicitOutput)
        options.OutputFile = options.InputFile + (options.EmitIR ? ".ll" : ".o");
    return true;
}
} // namespace kaleidoscope
