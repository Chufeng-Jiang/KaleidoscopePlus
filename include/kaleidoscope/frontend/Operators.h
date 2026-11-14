#pragma once
#include <map>
namespace kaleidoscope {
// Shared only within one compilation session; custom operators extend this table.
using OperatorTable = std::map<char, int>;
} // namespace kaleidoscope
