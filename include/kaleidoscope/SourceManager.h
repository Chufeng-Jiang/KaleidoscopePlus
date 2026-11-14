#pragma once
#include <istream>
#include <string>
#include <string_view>
#include <vector>
namespace kaleidoscope {
// Reads one physical line at a time, preserving text for file and REPL diagnostics.
class SourceManager {
  public:
    int get(std::istream &input);
    std::string_view line(unsigned number) const;

  private:
    void record(char ch);
    std::vector<std::string> Lines{1};
    std::string Pending;
    std::size_t Cursor = 0;
    bool PreviousCR = false;
};
} // namespace kaleidoscope
