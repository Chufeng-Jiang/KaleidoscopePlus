#pragma once
#include <ostream>
#include <string>
namespace kaleidoscope {
struct CompilerOptions {
    std::string InputFile;
    std::string OutputFile;
    bool EmitIR = false;
    bool Debug = false;
    bool Help = false;
    unsigned Optimization = 2;
};
bool parseOptions(int argc, char *argv[], CompilerOptions &options, std::ostream &errors);
void printUsage(std::ostream &output);
} // namespace kaleidoscope
