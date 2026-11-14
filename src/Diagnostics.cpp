#include "kaleidoscope/Diagnostics.h"
namespace kaleidoscope {

void Diagnostics::error(const char *message, SourceLocation location) {
    if (location.Line)
        Output << SourceFile << ":" << location.Line << ":" << location.Column << ": ";
    Output << "Error: " << message << '\n';
    HasError = true;
}

} // namespace kaleidoscope
