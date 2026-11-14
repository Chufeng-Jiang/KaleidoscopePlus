#pragma once
#include "kaleidoscope/SourceLocation.h"
#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>
namespace llvm {
class Value;
} // namespace llvm

namespace kaleidoscope {

struct CodegenContext;
class NumberExprAST;
class VariableExprAST;
class UnaryExprAST;
class BinaryExprAST;
class CallExprAST;
class IfExprAST;
class ForExprAST;
class VarExprAST;

// Backends implement this interface; AST nodes do not link against LLVM.
class ExprVisitor {
  public:
    virtual ~ExprVisitor() = default;
    virtual llvm::Value *visit(NumberExprAST &node) = 0;
    virtual llvm::Value *visit(VariableExprAST &node) = 0;
    virtual llvm::Value *visit(UnaryExprAST &node) = 0;
    virtual llvm::Value *visit(BinaryExprAST &node) = 0;
    virtual llvm::Value *visit(CallExprAST &node) = 0;
    virtual llvm::Value *visit(IfExprAST &node) = 0;
    virtual llvm::Value *visit(ForExprAST &node) = 0;
    virtual llvm::Value *visit(VarExprAST &node) = 0;
};

class ExprAST {
    SourceLocation Location;

  public:
    SourceLocation location() const { return Location; }
    void setLocation(SourceLocation loc) { Location = loc; }
    virtual ~ExprAST() = default;
    virtual const std::string *variableName() const { return nullptr; }
    virtual llvm::Value *accept(ExprVisitor &visitor) = 0;
};

class NumberExprAST : public ExprAST {
    friend struct CodegenContext;
    double Val;

  public:
    NumberExprAST(double Val) : Val(Val) {}

    llvm::Value *accept(ExprVisitor &visitor) override { return visitor.visit(*this); }
};

class VariableExprAST : public ExprAST {
    friend struct CodegenContext;
    std::string Name;

  public:
    VariableExprAST(const std::string &Name) : Name(Name) {}
    const std::string &getName() const { return Name; }
    const std::string *variableName() const override { return &Name; }

    llvm::Value *accept(ExprVisitor &visitor) override { return visitor.visit(*this); }
};

class UnaryExprAST : public ExprAST {
    friend struct CodegenContext;
    char Opcode;
    std::unique_ptr<ExprAST> Operand;

  public:
    UnaryExprAST(char Opcode, std::unique_ptr<ExprAST> Operand)
        : Opcode(Opcode), Operand(std::move(Operand)) {}

    llvm::Value *accept(ExprVisitor &visitor) override { return visitor.visit(*this); }
};

class BinaryExprAST : public ExprAST {
    friend struct CodegenContext;
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;

  public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

    llvm::Value *accept(ExprVisitor &visitor) override { return visitor.visit(*this); }
};

class CallExprAST : public ExprAST {
    friend struct CodegenContext;
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

  public:
    CallExprAST(const std::string &Callee, std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}

    llvm::Value *accept(ExprVisitor &visitor) override { return visitor.visit(*this); }
};

class IfExprAST : public ExprAST {
    friend struct CodegenContext;
    std::unique_ptr<ExprAST> Cond, Then, Else;

  public:
    IfExprAST(std::unique_ptr<ExprAST> Cond, std::unique_ptr<ExprAST> Then,
              std::unique_ptr<ExprAST> Else)
        : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

    llvm::Value *accept(ExprVisitor &visitor) override { return visitor.visit(*this); }
};

class ForExprAST : public ExprAST {
    friend struct CodegenContext;
    std::string VarName;
    SourceLocation VariableLocation;
    std::unique_ptr<ExprAST> Start, End, Step, Body;

  public:
    void setVariableLocation(SourceLocation loc) { VariableLocation = loc; }
    ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
               std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
               std::unique_ptr<ExprAST> Body)
        : VarName(VarName), Start(std::move(Start)), End(std::move(End)), Step(std::move(Step)),
          Body(std::move(Body)) {}

    llvm::Value *accept(ExprVisitor &visitor) override { return visitor.visit(*this); }
};

class VarExprAST : public ExprAST {
    friend struct CodegenContext;
    std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
    std::vector<SourceLocation> VariableLocations;
    std::unique_ptr<ExprAST> Body;

  public:
    void setVariableLocations(std::vector<SourceLocation> locations) {
        VariableLocations = std::move(locations);
    }
    VarExprAST(std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
               std::unique_ptr<ExprAST> Body)
        : VarNames(std::move(VarNames)), Body(std::move(Body)) {}

    llvm::Value *accept(ExprVisitor &visitor) override { return visitor.visit(*this); }
};

class PrototypeAST {
    friend struct CodegenContext;
    std::string Name;
    std::vector<std::string> Args;
    bool IsOperator;
    unsigned Precedence;
    SourceLocation Location;
    std::vector<SourceLocation> ArgumentLocations;

  public:
    PrototypeAST(const std::string &Name, std::vector<std::string> Args, bool IsOperator = false,
                 unsigned Prec = 0)
        : Name(Name), Args(std::move(Args)), IsOperator(IsOperator), Precedence(Prec) {}
    const std::string &getName() const { return Name; }
    bool isUnaryOp() const { return IsOperator && Args.size() == 1; }
    bool isBinaryOp() const { return IsOperator && Args.size() == 2; }
    char getOperatorName() const {
        assert(isUnaryOp() || isBinaryOp());
        return Name[Name.size() - 1];
    }
    SourceLocation location() const { return Location; }
    void setLocation(SourceLocation loc) { Location = loc; }
    void setArgumentLocations(std::vector<SourceLocation> locations) {
        ArgumentLocations = std::move(locations);
    }
    unsigned getBinaryPrecedence() const { return Precedence; }
};

class FunctionAST {
    friend struct CodegenContext;
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

  public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto, std::unique_ptr<ExprAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}
};

} // namespace kaleidoscope
