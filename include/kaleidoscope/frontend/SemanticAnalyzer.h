#pragma once
#include "kaleidoscope/frontend/AST.h"
#include "kaleidoscope/frontend/Operators.h"
#include "kaleidoscope/support/Diagnostics.h"
#include <map>

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
        Kind Category;
        FunctionType Signature;
        Type ValueType = Type::Unknown;
        bool Defined;
        SourceLocation Location;
    };
    Diagnostics &Diag;
    OperatorTable &Operators;
    std::map<std::string, Symbol> Symbols;
    std::vector<std::map<std::string, Type>> Scopes;
    const PrototypeAST *CurrentFunction = nullptr;
    bool checkPrototype(const PrototypeAST &prototype, bool definition, bool temporary = false);
    Type variableType(const std::string &name) const;
    Type infer(const ExprAST &expression);
    bool checkCondition(Type type, SourceLocation location);
    bool checkLiteral(double value, Type type, SourceLocation location);
    bool requireType(Type actual, Type expected, SourceLocation location,
                     const std::string &context);
    Type checkCall(const std::string &name, const std::vector<Type> &arguments,
                   SourceLocation location);
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
