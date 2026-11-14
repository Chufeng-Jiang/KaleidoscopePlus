#pragma once
#include "kaleidoscope/Token.h"
#include <istream>
#include <string>
namespace kaleidoscope {
class Lexer {
  public:
    explicit Lexer(std::istream &input) : Input(input) {}
    int next();
    const std::string &identifier() const { return IdentifierStr; }
    double number() const { return NumVal; }

  private:
    std::istream &Input;
    int LastChar = ' ';
    std::string IdentifierStr;
    double NumVal = 0;
};
} // namespace kaleidoscope
