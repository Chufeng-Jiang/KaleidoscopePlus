#include "kaleidoscope/Parser.h"
#include "kaleidoscope/SemanticAnalyzer.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kaleidoscope;
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

int main() {
    // This executable links only the frontend. No LLVM context or backend is used.
    std::istringstream input(
        "extern f(x); def bad(a) missing; def bad(a) a; def f(x y) x; def f(x) x; "
        "global g=1; def shadow(g) var x=g in x; def leak() x; "
        "def rec(x) if x<1 then 0 else rec(x-1); "
        "def wrong() f(1,2); def invalid() 1=2; def self() var y=y in y; "
        "def loop() for i=1,i<2,i in i; def leakedloop() i; "
        "def badloop() for i=i,i<2 in i; "
        "def sequence() var x=1,y=x in y; "
        "def binary % 30 (a b) missing; def binary % 30 (a b) a*b; "
        "def custom(x) x%2; extern unary !(x); def neg(x) !x; "
        "global f=1; extern g(x); def dupe(x x) x;");
    SourceManager source;
    std::ostringstream messages;
    Diagnostics diagnostics(messages, &source);
    Lexer lexer(input, &source);
    OperatorTable operators{{'=', 2}, {'<', 10}, {'+', 20}, {'-', 20}, {'*', 40}};
    Parser parser(lexer, diagnostics, operators);
    SemanticAnalyzer semantics(diagnostics, operators);
    const bool expected[] = {true,  false, true,  false, true, true,  true,  false,
                             true,  false, false, false, true, false, false, true,
                             false, true,  true,  true,  true, false, false, false};
    parser.getNextToken();
    for (bool valid : expected) {
        while (parser.currentToken() == ';')
            parser.getNextToken();
        bool result;
        if (parser.currentToken() == tok_def) {
            auto function = parser.ParseDefinition();
            require(bool(function), "definition did not parse");
            result = semantics.analyze(*function);
            if (result)
                semantics.commit(*function);
        } else if (parser.currentToken() == tok_extern) {
            auto prototype = parser.ParseExtern();
            require(bool(prototype), "extern did not parse");
            result = semantics.analyze(*prototype);
            if (result)
                semantics.commit(*prototype);
        } else {
            auto global = parser.ParseGlobal();
            require(bool(global), "global did not parse");
            result = semantics.analyze(*global);
            if (result)
                semantics.commit(*global);
        }
        require(result == valid, "unexpected semantic result");
    }
    require(operators.at('%') == 30, "successful operator not committed");
    auto text = messages.str();
    require(text.find("Previous declaration is here") != std::string::npos,
            "missing declaration note");
    require(text.find("Duplicate parameter name") != std::string::npos,
            "missing duplicate parameter");

    // Successful analysis itself does not commit a symbol. Failed checks also leave it untouched.
    FunctionAST function(std::make_unique<PrototypeAST>("pending", std::vector<std::string>{}),
                         std::make_unique<NumberExprAST>(1));
    require(semantics.analyze(function), "first preflight");
    require(semantics.analyze(function), "analysis mutated the symbol table");
    semantics.commit(function);
    require(!semantics.analyze(function), "committed duplicate accepted");
    FunctionAST anonymous(std::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>{}),
                          std::make_unique<NumberExprAST>(1));
    require(semantics.analyze(anonymous, true), "temporary expression");
    semantics.commit(anonymous, true);
    require(semantics.analyze(anonymous, true), "temporary expression polluted symbols");

    // Sessions do not share global declarations or function names.
    std::ostringstream secondMessages;
    Diagnostics secondDiagnostics(secondMessages);
    OperatorTable secondOperators;
    SemanticAnalyzer second(secondDiagnostics, secondOperators);
    require(second.analyze(function), "symbols leaked to a second session");
    std::cout << "Standalone semantic analysis, scopes, and transactional symbols passed\n";
}
