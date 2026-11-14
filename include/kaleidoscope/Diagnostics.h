#pragma once
#include <ostream>
namespace kaleidoscope {
class Diagnostics {
  public:
    explicit Diagnostics(std::ostream &output) : Output(output) {}
    void error(const char *message);
    bool hasError() const { return HasError; }

  private:
    std::ostream &Output;
    bool HasError = false;
};
} // namespace kaleidoscope
