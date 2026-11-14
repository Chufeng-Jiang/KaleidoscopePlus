#include "kaleidoscope/Diagnostics.h"
namespace kaleidoscope {

void Diagnostics::error(const char *message) {
    Output << "Error: " << message << '\n';
    HasError = true;
}

} // namespace kaleidoscope
