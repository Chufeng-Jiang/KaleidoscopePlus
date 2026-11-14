#include "kaleidoscope/Lexer.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace kaleidoscope {

int Lexer::advance() {
    int ch = Input.get();
    CharLocation = NextLocation;
    if (ch == '\r') {
        ++NextLocation.Line;
        NextLocation.Column = 1;
    } else if (ch == '\n') {
        if (!PreviousCR)
            ++NextLocation.Line;
        NextLocation.Column = 1;
    } else if (ch != EOF) {
        ++NextLocation.Column;
    }
    PreviousCR = ch == '\r';
    return ch;
}

int Lexer::next() {

    // Skip whitespace and comments iteratively, including a comment ending at EOF.
    while (true) {
        while (isspace(LastChar))
            LastChar = advance();
        if (LastChar != '#')
            break;
        do
            LastChar = advance();
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');
    }

    TokenLocation = CharLocation;

    // keyword & identifier
    if (isalpha(LastChar)) {
        IdentifierStr = LastChar;
        while (isalnum(LastChar = advance()))
            IdentifierStr += LastChar;

        // keyword
        if (IdentifierStr == "def")
            return tok_def;
        if (IdentifierStr == "extern")
            return tok_extern;
        if (IdentifierStr == "if")
            return tok_if;
        if (IdentifierStr == "then")
            return tok_then;
        if (IdentifierStr == "else")
            return tok_else;
        if (IdentifierStr == "for")
            return tok_for;
        if (IdentifierStr == "in")
            return tok_in;
        if (IdentifierStr == "binary")
            return tok_binary;
        if (IdentifierStr == "unary")
            return tok_unary;
        if (IdentifierStr == "var")
            return tok_var;

        // identifier
        return tok_identifier;
    }

    // number
    if (isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = advance();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), 0);
        return tok_number;
    }

    // end
    if (LastChar == EOF)
        return tok_eof;

    // other character
    int ThisChar = LastChar;
    LastChar = advance();
    return ThisChar;
}

} // namespace kaleidoscope
