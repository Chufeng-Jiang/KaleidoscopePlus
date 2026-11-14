#pragma once
#include "kaleidoscope/frontend/Type.h"
#include "kaleidoscope/support/SourceLocation.h"
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
class SemanticAnalyzer;
class NumberExprAST;
class VariableExprAST;
class UnaryExprAST;
class BinaryExprAST;
class CallExprAST;
class IfExprAST;
class ForExprAST;
class VarExprAST;

class ConstExprVisitor {
  public:
    virtual ~ConstExprVisitor() = default;
    virtual void visit(const NumberExprAST &node) = 0;
    virtual void visit(const VariableExprAST &node) = 0;
    virtual void visit(const UnaryExprAST &node) = 0;
    virtual void visit(const BinaryExprAST &node) = 0;
    virtual void visit(const CallExprAST &node) = 0;
    virtual void visit(const IfExprAST &node) = 0;
    virtual void visit(const ForExprAST &node) = 0;
    virtual void visit(const VarExprAST &node) = 0;
};

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
    friend class SemanticAnalyzer;
    SourceLocation Location;
    // Analysis metadata; the expression tree itself remains unchanged.
    mutable Type InferredType = Type::Unknown;

  public:
    SourceLocation location() const { return Location; }
    void setLocation(SourceLocation loc) { Location = loc; }
    Type type() const { return InferredType; }
    virtual ~ExprAST() = default;
    virtual const std::string *variableName() const { return nullptr; }
    virtual llvm::Value *accept(ExprVisitor &visitor) = 0;
    virtual void accept(ConstExprVisitor &visitor) const = 0;
};

class NumberExprAST : public ExprAST {
    friend struct CodegenContext;
    friend class SemanticAnalyzer;
    double Val;
    Type LiteralType;

  public:
    double value() const { return Val; }
    NumberExprAST(double Val, Type type = Type::Double) : Val(Val), LiteralType(type) {}

    llvm::Value *accept(ExprVisitor &visitor) override { return visitor.visit(*this); }
    void accept(ConstExprVisitor &visitor) const override { visitor.visit(*this); }
};

class VariableExprAST : public ExprAST {
    friend struct CodegenContext;
    friend class SemanticAnalyzer;
    std::string Name;

  public:
    VariableExprAST(const std::string &Name) : Name(Name) {}
    const std::string &getName() const { return Name; }
    const std::string *variableName() const override { return &Name; }

    llvm::Value *accept(ExprVisitor &visitor) override { return visitor.visit(*this); }
    void accept(ConstExprVisitor &visitor) const override { visitor.visit(*this); }
};

class UnaryExprAST : public ExprAST {
    friend struct CodegenContext;
    friend class SemanticAnalyzer;
    char Opcode;
    std::unique_ptr<ExprAST> Operand;

  public:
    char opcode() const { return Opcode; }
    const ExprAST &operand() const { return *Operand; }
    UnaryExprAST(char Opcode, std::unique_ptr<ExprAST> Operand)
        : Opcode(Opcode), Operand(std::move(Operand)) {}

    llvm::Value *accept(ExprVisitor &visitor) override { return visitor.visit(*this); }
    void accept(ConstExprVisitor &visitor) const override { visitor.visit(*this); }
};

class BinaryExprAST : public ExprAST {
    friend struct CodegenContext;
    friend class SemanticAnalyzer;
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;

  public:
    char opcode() const { return Op; }
    const ExprAST &lhs() const { return *LHS; }
    const ExprAST &rhs() const { return *RHS; }
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

    llvm::Value *accept(ExprVisitor &visitor) override { return visitor.visit(*this); }
    void accept(ConstExprVisitor &visitor) const override { visitor.visit(*this); }
};

class CallExprAST : public ExprAST {
    friend struct CodegenContext;
    friend class SemanticAnalyzer;
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

  public:
    const std::string &callee() const { return Callee; }
    const auto &arguments() const { return Args; }
    CallExprAST(const std::string &Callee, std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}

    llvm::Value *accept(ExprVisitor &visitor) override { return visitor.visit(*this); }
    void accept(ConstExprVisitor &visitor) const override { visitor.visit(*this); }
};

class IfExprAST : public ExprAST {
    friend struct CodegenContext;
    friend class SemanticAnalyzer;
    std::unique_ptr<ExprAST> Cond, Then, Else;

  public:
    const ExprAST &condition() const { return *Cond; }
    const ExprAST &thenExpr() const { return *Then; }
    const ExprAST &elseExpr() const { return *Else; }
    IfExprAST(std::unique_ptr<ExprAST> Cond, std::unique_ptr<ExprAST> Then,
              std::unique_ptr<ExprAST> Else)
        : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

    llvm::Value *accept(ExprVisitor &visitor) override { return visitor.visit(*this); }
    void accept(ConstExprVisitor &visitor) const override { visitor.visit(*this); }
};

class ForExprAST : public ExprAST {
    friend struct CodegenContext;
    friend class SemanticAnalyzer;
    std::string VarName;
    SourceLocation VariableLocation;
    Type BindingType = Type::Double;
    std::unique_ptr<ExprAST> Start, End, Step, Body;

  public:
    const std::string &variable() const { return VarName; }
    Type bindingType() const { return BindingType; }
    const ExprAST &start() const { return *Start; }
    const ExprAST &end() const { return *End; }
    const ExprAST *step() const { return Step.get(); }
    const ExprAST &body() const { return *Body; }
    void setBindingType(Type type) { BindingType = type; }
    void setVariableLocation(SourceLocation loc) { VariableLocation = loc; }
    ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
               std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
               std::unique_ptr<ExprAST> Body)
        : VarName(VarName), Start(std::move(Start)), End(std::move(End)), Step(std::move(Step)),
          Body(std::move(Body)) {}

    llvm::Value *accept(ExprVisitor &visitor) override { return visitor.visit(*this); }
    void accept(ConstExprVisitor &visitor) const override { visitor.visit(*this); }
};

class VarExprAST : public ExprAST {
    friend struct CodegenContext;
    friend class SemanticAnalyzer;
    std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
    std::vector<SourceLocation> VariableLocations;
    std::vector<Type> BindingTypes;
    std::unique_ptr<ExprAST> Body;

  public:
    const auto &bindings() const { return VarNames; }
    const auto &bindingTypes() const { return BindingTypes; }
    const ExprAST &body() const { return *Body; }
    void setBindingTypes(std::vector<Type> types) { BindingTypes = std::move(types); }
    void setVariableLocations(std::vector<SourceLocation> locations) {
        VariableLocations = std::move(locations);
    }
    VarExprAST(std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
               std::unique_ptr<ExprAST> Body)
        : VarNames(std::move(VarNames)), BindingTypes(this->VarNames.size(), Type::Double),
          Body(std::move(Body)) {}

    llvm::Value *accept(ExprVisitor &visitor) override { return visitor.visit(*this); }
    void accept(ConstExprVisitor &visitor) const override { visitor.visit(*this); }
};

class PrototypeAST {
    friend struct CodegenContext;
    friend class SemanticAnalyzer;
    std::string Name;
    std::vector<std::string> Args;
    bool IsOperator;
    unsigned Precedence;
    SourceLocation Location;
    std::vector<SourceLocation> ArgumentLocations;
    FunctionType Signature;

  public:
    PrototypeAST(const std::string &Name, std::vector<std::string> Args, bool IsOperator = false,
                 unsigned Prec = 0)
        : Name(Name), Args(std::move(Args)), IsOperator(IsOperator), Precedence(Prec),
          Signature{std::vector<Type>(this->Args.size(), Type::Double), Type::Double} {}
    const std::string &getName() const { return Name; }
    const std::vector<std::string> &arguments() const { return Args; }
    const FunctionType &signature() const { return Signature; }
    void setSignature(FunctionType signature) { Signature = std::move(signature); }
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

// A top-level mutable scalar definition, or a declaration supplied by another module.
class GlobalAST {
    friend struct CodegenContext;
    friend class SemanticAnalyzer;
    std::string Name;
    double Initializer;
    Type ValueType;
    Type InitializerType;
    bool External;
    SourceLocation Location;

  public:
    GlobalAST(std::string name, double initializer, bool external, Type type = Type::Double,
              Type initializerType = Type::Double)
        : Name(std::move(name)), Initializer(initializer), ValueType(type),
          InitializerType(initializerType), External(external) {}
    const std::string &getName() const { return Name; }
    Type type() const { return ValueType; }
    double initializer() const { return Initializer; }
    bool isExternal() const { return External; }
    SourceLocation location() const { return Location; }
    void setLocation(SourceLocation location) { Location = location; }
};

class FunctionAST {
    friend struct CodegenContext;
    friend class SemanticAnalyzer;
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

  public:
    const PrototypeAST &prototype() const { return *Proto; }
    const ExprAST &body() const { return *Body; }
    FunctionAST(std::unique_ptr<PrototypeAST> Proto, std::unique_ptr<ExprAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}
};

} // namespace kaleidoscope
