#include "kaleidoscope/Lexer.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace kaleidoscope {

int Lexer::next() {

    // skip white
    while (isspace(LastChar))
        LastChar = Input.get();

    // keyword & identifier
    if (isalpha(LastChar)) {
        IdentifierStr = LastChar;
        while (isalnum(LastChar = Input.get()))
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
            LastChar = Input.get();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), 0);
        return tok_number;
    }

    // comment
    if (LastChar == '#') {
        do
            LastChar = Input.get();
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return next();
    }

    // end
    if (LastChar == EOF)
        return tok_eof;

    // other character
    int ThisChar = LastChar;
    LastChar = Input.get();
    return ThisChar;
}

} // namespace kaleidoscope
