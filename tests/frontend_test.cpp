#include "kaleidoscope/Parser.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace kaleidoscope;

void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

int main() {
    // Interleaved lexers must not share the lookahead character or token payload.
    std::istringstream left("alpha 12 # comment\n def"), right("beta 34");
    Lexer a(left), b(right);
    require(a.next() == tok_identifier && a.identifier() == "alpha", "first identifier");
    require(b.next() == tok_identifier && b.identifier() == "beta", "second identifier");
    require(a.next() == tok_number && a.number() == 12, "first number");
    require(b.next() == tok_number && b.number() == 34, "second number");
    require(a.next() == tok_def && a.next() == tok_eof, "comments and EOF");
    require(b.next() == tok_eof, "independent EOF");

    std::istringstream input1("def square(x) x*x;"), input2("extern sin(x);");
    std::ostringstream errors1, errors2;
    Diagnostics d1(errors1), d2(errors2);
    Lexer lex1(input1), lex2(input2);
    OperatorTable ops1{{'*', 40}}, ops2;
    Parser parser1(lex1, d1, ops1), parser2(lex2, d2, ops2);
    require(parser1.getNextToken() == tok_def, "definition token");
    require(parser2.getNextToken() == tok_extern, "extern token");
    require(parser1.ParseDefinition() != nullptr, "parse function without LLVM");
    auto proto = parser2.ParseExtern();
    require(proto && proto->getName() == "sin", "parse independent prototype");
    require(parser1.currentToken() == ';' && parser2.currentToken() == ';', "parser position");
    require(!d1.hasError() && !d2.hasError(), "unexpected diagnostic");
    require(ops2.empty(), "precedence tables must remain independent");
    d1.error("test diagnostic");
    require(d1.hasError() && !d2.hasError(), "diagnostics must remain independent");
    std::cout << "Independent frontend sessions passed\n";
}
