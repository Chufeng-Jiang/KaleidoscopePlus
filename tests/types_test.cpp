#include "kaleidoscope/frontend/Parser.h"
#include "kaleidoscope/frontend/SemanticAnalyzer.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kaleidoscope;
static void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
int main() {
    static_assert(isAssignable(Type::Double, Type::Double));
    static_assert(!isAssignable(Type::Double, Type::Unknown));
    static_assert(!isAssignable(Type::Error, Type::Error));
    std::ostringstream messages;
    Diagnostics diagnostics(messages);
    OperatorTable operators{{'=', 2}, {'<', 10}, {'+', 20}, {'-', 20}, {'*', 40}};
    SemanticAnalyzer semantics(diagnostics, operators);
    PrototypeAST external("external", {"x", "y"});
    require(external.signature() == FunctionType{{Type::Double, Type::Double}, Type::Double},
            "signature types");
    require(semantics.analyze(external), "extern analysis");
    semantics.commit(external);
    GlobalAST global("g", 2, false);
    require(global.type() == Type::Double, "global type");
    require(semantics.analyze(global), "global analysis");
    semantics.commit(global);
    // All legacy constructs have an explicit double result, including comparison and loops.
    const char *programs[] = {"def literal() 1;",
                              "def param(x) x;",
                              "def arithmetic(x) x+2*3;",
                              "def comparison(x) x<2;",
                              "def conditional(x) if x then 1 else 2;",
                              "def call() external(1,2);",
                              "def recursive(x) if x then recursive(x-1) else 0;",
                              "def loop() for i=1,i<3 in i;",
                              "def stepped() for i=1,i<3,2 in i;",
                              "def local() var x,y=2 in x=y;",
                              "def readglobal() g;",
                              "def storeglobal() g=3;",
                              "def shadow() var g=g+1,g=g+2 in g;",
                              "def binary % 30 (x y) x*y;",
                              "def custom() 2%3;",
                              "def unary !(x) if x then 0 else 1;",
                              "def unarycall() !2;"};
    for (auto program : programs) {
        std::istringstream input(program);
        Lexer lexer(input);
        Parser parser(lexer, diagnostics, operators);
        parser.getNextToken();
        auto function = parser.ParseDefinition();
        require(bool(function), "parse failed");
        require(function->body().type() == Type::Unknown, "unanalyzed expression has a type");
        require(semantics.analyze(*function), "valid expression rejected");
        require(function->body().type() == Type::Double, "wrong result type");
        require(semantics.analyze(*function), "reanalysis failed");
        semantics.commit(*function);
    }
    // Retain child references to verify annotation and avoid cascades from an invalid binding.
    auto missing = std::make_unique<VariableExprAST>("missing");
    auto *missingNode = missing.get();
    auto reference = std::make_unique<VariableExprAST>("x");
    auto *referenceNode = reference.get();
    std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> bindings;
    bindings.emplace_back("x", std::move(missing));
    auto var = std::make_unique<VarExprAST>(std::move(bindings), std::move(reference));
    FunctionAST invalid(std::make_unique<PrototypeAST>("invalid", std::vector<std::string>{}),
                        std::move(var));
    auto before = diagnostics.errorCount();
    require(!semantics.analyze(invalid), "unknown initializer accepted");
    require(diagnostics.errorCount() == before + 1, "cascading type errors");
    require(missingNode->type() == Type::Error && referenceNode->type() == Type::Error &&
                invalid.body().type() == Type::Error,
            "error type did not propagate");
    // Reanalysis recomputes types rather than reusing a stale error annotation.
    GlobalAST repaired("missing", 1, false);
    require(semantics.analyze(repaired), "repair declaration");
    semantics.commit(repaired);
    require(semantics.analyze(invalid), "stale error annotation");
    require(missingNode->type() == Type::Double && referenceNode->type() == Type::Double &&
                invalid.body().type() == Type::Double,
            "types were not recomputed");
    std::cout
        << "Type signatures, all expression kinds, error propagation, and reanalysis passed\n";
}
