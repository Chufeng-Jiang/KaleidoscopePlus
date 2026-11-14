#include "CodegenContext.h"
#include "llvm/IR/Constants.h"

namespace kaleidoscope {

Value *CodegenContext::visit(NumberExprAST &node) {
    auto &ctx = *this;
    LocationGuard location(*this, node.location());
    return scalarConstant(node.Val, node.type());
}

Value *CodegenContext::visit(UnaryExprAST &node) {
    auto &ctx = *this;
    LocationGuard location(*this, node.location());
    Value *OperandV = node.Operand->accept(ctx);
    if (!OperandV)
        return nullptr;

    if (node.Operand->type() == Type::I32 && node.Opcode == '-')
        return Builder->CreateNeg(OperandV, "neg");
    if (node.Operand->type() == Type::I32 && node.Opcode == '+')
        return OperandV;
    if (node.Operand->type() == Type::Bool && node.Opcode == '!')
        return Builder->CreateNot(OperandV, "not");
    Function *F = ctx.getFunction(std::string("unary") + node.Opcode);
    if (!F)
        return ctx.LogErrorV("Unknow unary operator");

    return call(F, {OperandV}, "unop");
}

Value *CodegenContext::visit(BinaryExprAST &node) {
    auto &ctx = *this;
    LocationGuard location(*this, node.location());
    if (node.Op == '=') {
        const std::string *LHSE = node.LHS->variableName();
        if (!LHSE)
            return ctx.LogErrorV("destination of '=' must be a variable");

        Value *Val = node.RHS->accept(ctx);
        if (!Val)
            return nullptr;

        Value *Variable = ctx.getVariableStorage(*LHSE);
        if (!Variable)
            return ctx.LogErrorV("Unknown variable name");

        ctx.Builder->CreateStore(Val, Variable);
        return Val;
    }

    Value *L = node.LHS->accept(ctx);
    Value *R = node.RHS->accept(ctx);
    if (!L || !R)
        return nullptr;

    switch (node.Op) {
    case '+':
        return node.type() == Type::I32 ? Builder->CreateAdd(L, R, "addtmp")
                                        : Builder->CreateFAdd(L, R, "addtmp");
    case '-':
        return node.type() == Type::I32 ? Builder->CreateSub(L, R, "subtmp")
                                        : Builder->CreateFSub(L, R, "subtmp");
    case '*':
        return node.type() == Type::I32 ? Builder->CreateMul(L, R, "multmp")
                                        : Builder->CreateFMul(L, R, "multmp");
    case '<':
        if (node.LHS->type() == Type::I32)
            return Builder->CreateICmpSLT(L, R, "cmptmp");
        L = ctx.Builder->CreateFCmpULT(L, R, "cmptmp");
        return ctx.Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*ctx.TheContext), "booltmp");
    default:
        break;
    }

    Function *F = ctx.getFunction(std::string("binary") + node.Op);
    if (!F)
        return ctx.LogErrorV("binary operator not found");

    Value *Ops[] = {L, R};
    return call(F, Ops, "binop");
}

Value *CodegenContext::visit(CallExprAST &node) {
    auto &ctx = *this;
    LocationGuard location(*this, node.location());
    if (namedType(node.Callee) != Type::Unknown) {
        auto *value = node.Args[0]->accept(ctx);
        return value ? convert(value, node.type()) : nullptr;
    }
    Function *CalleeF = ctx.getFunction(node.Callee);
    if (!CalleeF)
        return ctx.LogErrorV("Unknown function referenced");

    if (CalleeF->arg_size() != node.Args.size())
        return ctx.LogErrorV("Incorrect # arguments passed");

    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = node.Args.size(); i != e; ++i) {
        ArgsV.push_back(node.Args[i]->accept(ctx));
        if (!ArgsV.back())
            return nullptr;
    }

    return call(CalleeF, ArgsV, "calltmp");
}

} // namespace kaleidoscope
