#include "kaleidoscope/frontend/Lexer.h"
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>

namespace kaleidoscope {

int Lexer::advance() {
    int ch = Source ? Source->get(Input) : Input.get();
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
        auto commentLocation = CharLocation;
        std::string comment;
        LastChar = advance();
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r') {
            comment += static_cast<char>(LastChar);
            LastChar = advance();
        }
        if (comment == "acc" || (comment.size() > 3 && comment.compare(0, 3, "acc") == 0 &&
                                isspace(static_cast<unsigned char>(comment[3])))) {
            TokenLocation = commentLocation;
            IdentifierStr = comment.substr(3);
            return tok_acc;
        }
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
        if (IdentifierStr == "global")
            return tok_global;
        if (IdentifierStr == "var")
            return tok_var;

        if (IdentifierStr == "true")
            return tok_true;
        if (IdentifierStr == "false")
            return tok_false;
        if (IdentifierStr == "i32" || IdentifierStr == "bool" || IdentifierStr == "double" || IdentifierStr == "array")
            return tok_type;
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

        char *end = nullptr;
        errno = 0;
        NumVal = strtod(NumStr.c_str(), &end);
        if (end == NumStr.c_str() || *end != '\0' || errno == ERANGE)
            return tok_invalid;
        if (isalpha(LastChar)) {
            std::string suffix;
            do {
                suffix += LastChar;
                LastChar = advance();
            } while (isalnum(LastChar));
            if (suffix != "i32" || NumStr.find('.') != std::string::npos)
                return tok_invalid;
            return tok_integer;
        }
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
