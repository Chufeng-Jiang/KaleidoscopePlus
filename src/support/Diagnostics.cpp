#include "kaleidoscope/support/Diagnostics.h"
#include <algorithm>
namespace kaleidoscope {
void Diagnostics::emit(std::string_view severity, std::string_view message,
                       SourceLocation location) {
    if (location.Line)
        Output << SourceFile << ":" << location.Line << ":" << location.Column << ": ";
    Output << severity << ": " << message << '\n';
    if (!Source || !location.Line || !location.Column)
        return;
    auto text = Source->line(location.Line);
    std::string expanded;
    std::size_t caret = 0;
    // Columns are byte offsets; expand tabs consistently in the line and marker.
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (i == location.Column - 1)
            caret = expanded.size();
        if (text[i] == '\t')
            expanded.append(8 - expanded.size() % 8, ' ');
        else
            expanded += text[i];
    }
    if (location.Column - 1 >= text.size())
        caret = expanded.size();
    Output << "  " << expanded << '\n' << "  " << std::string(caret, ' ') << "^\n";
}
void Diagnostics::error(std::string_view message, SourceLocation location) {
    ++Errors;
    emit("Error", message, location);
}
void Diagnostics::note(std::string_view message, SourceLocation location) {
    emit("Note", message, location);
}
} // namespace kaleidoscope
