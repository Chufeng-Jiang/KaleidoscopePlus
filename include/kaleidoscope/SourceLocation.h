#pragma once
namespace kaleidoscope {
// One-based byte columns; zero denotes an unavailable location.
struct SourceLocation {
    unsigned Line = 0;
    unsigned Column = 0;
};
} // namespace kaleidoscope
