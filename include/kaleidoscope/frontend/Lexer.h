#pragma once
#include "kaleidoscope/frontend/Token.h"
#include "kaleidoscope/support/SourceLocation.h"
#include "kaleidoscope/support/SourceManager.h"
#include <istream>
#include <string>
namespace kaleidoscope {
class Lexer {
  public:
    explicit Lexer(std::istream &input, SourceManager *source = nullptr)
        : Input(input), Source(source) {}
    int next();
    SourceLocation location() const { return TokenLocation; }
    const std::string &identifier() const { return IdentifierStr; }
    double number() const { return NumVal; }

  private:
    std::istream &Input;
    SourceManager *Source;
    int advance();
    int LastChar = ' ';
    SourceLocation NextLocation{1, 1};
    SourceLocation CharLocation;
    SourceLocation TokenLocation;
    bool PreviousCR = false;
    std::string IdentifierStr;
    double NumVal = 0;
};
} // namespace kaleidoscope
