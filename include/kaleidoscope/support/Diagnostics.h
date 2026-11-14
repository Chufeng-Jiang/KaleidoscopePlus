#pragma once
#include "kaleidoscope/support/SourceLocation.h"
#include "kaleidoscope/support/SourceManager.h"
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
namespace kaleidoscope {
class Diagnostics {
  public:
    explicit Diagnostics(std::ostream &output, const SourceManager *source = nullptr)
        : Output(output), Source(source) {}
    void error(std::string_view message, SourceLocation location = {});
    void note(std::string_view message, SourceLocation location);
    unsigned errorCount() const { return Errors; }
    void setSourceFile(std::string file) { SourceFile = std::move(file); }
    bool hasError() const { return Errors != 0; }

  private:
    std::ostream &Output;
    unsigned Errors = 0;
    const SourceManager *Source;
    void emit(std::string_view severity, std::string_view message, SourceLocation location);
    std::string SourceFile = "<stdin>";
};
} // namespace kaleidoscope
