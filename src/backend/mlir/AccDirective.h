#pragma once
#include "kaleidoscope/frontend/AST.h"
#include "kaleidoscope/support/Diagnostics.h"
#include <map>
namespace kaleidoscope::mlir_backend {
// A deliberately restricted teaching directive, not the OpenACC grammar.
struct AccDirective {
    std::string x, y, out;
    SourceLocation location;
};
bool parseAccDirective(const std::string &, SourceLocation, Diagnostics &, AccDirective &);
bool validateAccLoop(const AccDirective &, const FunctionAST &,
                     const std::map<std::string, std::unique_ptr<FunctionAST>> &, Diagnostics &,
                     std::string &element);
} // namespace kaleidoscope::mlir_backend
