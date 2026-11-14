#pragma once
#include "kaleidoscope/SourceLocation.h"
#include <ostream>
#include <string>
#include <utility>
namespace kaleidoscope {
class Diagnostics {
  public:
    explicit Diagnostics(std::ostream &output) : Output(output) {}
    void error(const char *message, SourceLocation location = {});
    void setSourceFile(std::string file) { SourceFile = std::move(file); }
    bool hasError() const { return HasError; }

  private:
    std::ostream &Output;
    bool HasError = false;
    std::string SourceFile = "<stdin>";
};
} // namespace kaleidoscope
