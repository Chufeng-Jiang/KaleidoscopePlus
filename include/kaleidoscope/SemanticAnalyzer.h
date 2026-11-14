#pragma once
#include "kaleidoscope/AST.h"
#include "kaleidoscope/Diagnostics.h"
#include "kaleidoscope/Operators.h"
#include <map>
#include <set>
namespace kaleidoscope {
// The authoritative language-level namespace. Contains no LLVM objects.
class SemanticAnalyzer : private ConstExprVisitor {
  public:
    SemanticAnalyzer(Diagnostics &diagnostics, OperatorTable &operators)
        : Diag(diagnostics), Operators(operators) {}
    bool analyze(const PrototypeAST &prototype);
    bool analyze(const GlobalAST &global);
    bool analyze(const FunctionAST &function, bool temporary = false);
    // Driver commits only after successful analysis and lowering. ASTs stay intact.
    void commit(const PrototypeAST &prototype);
    void commit(const GlobalAST &global);
    void commit(const FunctionAST &function, bool temporary = false);

  private:
    enum class Kind { Function, Global };
    struct Symbol {
        Kind Type;
        std::size_t Arity;
        bool Defined;
        SourceLocation Location;
    };
    Diagnostics &Diag;
    OperatorTable &Operators;
    std::map<std::string, Symbol> Symbols;
    std::vector<std::set<std::string>> Scopes;
    const PrototypeAST *CurrentFunction = nullptr;
    bool checkPrototype(const PrototypeAST &prototype, bool definition, bool temporary = false);
    bool variableExists(const std::string &name) const;
    void checkCall(const std::string &name, std::size_t arity, SourceLocation location);
    void previous(const Symbol &symbol);
    void visit(const NumberExprAST &node) override;
    void visit(const VariableExprAST &node) override;
    void visit(const UnaryExprAST &node) override;
    void visit(const BinaryExprAST &node) override;
    void visit(const CallExprAST &node) override;
    void visit(const IfExprAST &node) override;
    void visit(const ForExprAST &node) override;
    void visit(const VarExprAST &node) override;
};
} // namespace kaleidoscope
