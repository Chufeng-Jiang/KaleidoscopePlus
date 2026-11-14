#include "kaleidoscope/support/SourceManager.h"
namespace kaleidoscope {
void SourceManager::record(char ch) {
    if (ch == '\r')
        Lines.emplace_back();
    else if (ch == '\n') {
        if (!PreviousCR)
            Lines.emplace_back();
    } else
        Lines.back() += ch;
    PreviousCR = ch == '\r';
}
int SourceManager::get(std::istream &input) {
    if (Cursor == Pending.size()) {
        Pending.clear();
        Cursor = 0;
        if (!std::getline(input, Pending))
            return std::char_traits<char>::eof();
        if (!input.eof())
            Pending += '\n';
        for (char ch : Pending)
            record(ch);
    }
    return static_cast<unsigned char>(Pending[Cursor++]);
}
std::string_view SourceManager::line(unsigned number) const {
    if (number == 0 || number > Lines.size())
        return {};
    return Lines[number - 1];
}
} // namespace kaleidoscope
