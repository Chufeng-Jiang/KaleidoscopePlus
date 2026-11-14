#include "kaleidoscope/Parser.h"
#include <cctype>

namespace kaleidoscope {
namespace {
template <class T, class... Args>
std::unique_ptr<T> makeLocated(SourceLocation location, Args &&...args) {
    auto result = std::make_unique<T>(std::forward<Args>(args)...);
    result->setLocation(location);
    return result;
}
} // namespace

int Parser::getNextToken() { return CurTok = Lex.next(); }

int Parser::GetTokPrecedence() {
    if (!isascii(CurTok))
        return -1;

    auto entry = BinopPrecedence.find(static_cast<char>(CurTok));
    if (entry == BinopPrecedence.end())
        return -1;
    int TokPrec = entry->second;
    if (TokPrec <= 0)
        return -1;

    return TokPrec;
}

std::unique_ptr<ExprAST> Parser::LogError(const char *Str) {
    Diag.error(Str, Lex.location());
    return nullptr;
}

std::unique_ptr<PrototypeAST> Parser::LogErrorP(const char *Str) {
    LogError(Str);
    return nullptr;
}

// numberexpr ::= number
std::unique_ptr<ExprAST> Parser::ParseNumberExpr() {
    auto Result = makeLocated<NumberExprAST>(Lex.location(), Lex.number());
    getNextToken();
    return Result;
}

// parenexpr ::= '(' expression ')'
std::unique_ptr<ExprAST> Parser::ParseParenExpr() {
    getNextToken();
    auto V = ParseExpression();
    if (!V)
        return nullptr;

    if (CurTok != ')')
        return LogError("expected ')'");
    getNextToken();
    return V;
}

// identifierexpr
//  ::= identifier
//  ::= identifier '(' expression* ')'
std::unique_ptr<ExprAST> Parser::ParseIdentifierExpr() {
    auto Loc = Lex.location();
    std::string IdName = Lex.identifier();
    getNextToken();

    // Variable
    if (CurTok != '(')
        return makeLocated<VariableExprAST>(Loc, IdName);

    // Call
    getNextToken();
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (CurTok != ')') {
        while (true) {
            if (auto Arg = ParseExpression())
                Args.push_back(std::move(Arg));
            else
                return nullptr;

            if (CurTok == ')')
                break;

            if (CurTok != ',')
                return LogError("Expected ')' or ',' in argument list");

            getNextToken();
        }
    }

    getNextToken();
    return makeLocated<CallExprAST>(Loc, IdName, std::move(Args));
}

// ifexpr ::= 'if' expression 'then' expression 'else' expression
std::unique_ptr<ExprAST> Parser::ParseIfExpr() {
    auto Loc = Lex.location();
    getNextToken();

    auto Cond = ParseExpression();
    if (!Cond)
        return nullptr;

    if (CurTok != tok_then)
        return LogError("expected then");
    getNextToken();

    auto Then = ParseExpression();
    if (!Then)
        return nullptr;

    if (CurTok != tok_else)
        return LogError("expected else");
    getNextToken();

    auto Else = ParseExpression();
    if (!Else)
        return nullptr;

    return makeLocated<IfExprAST>(Loc, std::move(Cond), std::move(Then), std::move(Else));
}

// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
std::unique_ptr<ExprAST> Parser::ParseForExpr() {
    auto Loc = Lex.location();
    getNextToken();
    if (CurTok != tok_identifier)
        return LogError("expected identifier after for");

    auto VariableLoc = Lex.location();
    std::string IdName = Lex.identifier();
    getNextToken();
    if (CurTok != '=')
        return LogError("expected '=' after for");
    getNextToken();

    auto Start = ParseExpression();
    if (!Start)
        return nullptr;
    if (CurTok != ',')
        return LogError("expected ',' after for start value");
    getNextToken();

    auto End = ParseExpression();
    if (!End)
        return nullptr;

    std::unique_ptr<ExprAST> Step;
    if (CurTok == ',') {
        getNextToken();
        Step = ParseExpression();
        if (!Step)
            return nullptr;
    }

    if (CurTok != tok_in)
        return LogError("expected 'in' after for");
    getNextToken();

    auto Body = ParseExpression();
    if (!Body)
        return nullptr;

    auto result = makeLocated<ForExprAST>(Loc, IdName, std::move(Start), std::move(End),
                                          std::move(Step), std::move(Body));
    result->setVariableLocation(VariableLoc);
    return result;
}

// varexpr ::= 'var' identifier ('=' expression)? (',' identifier ('=' expression)?)* 'in'
// expression
std::unique_ptr<ExprAST> Parser::ParseVarExpr() {
    auto Loc = Lex.location();
    getNextToken();

    std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
    std::vector<SourceLocation> VariableLocations;

    if (CurTok != tok_identifier)
        return LogError("excepted identifier after var");

    while (true) {
        VariableLocations.push_back(Lex.location());
        std::string Name = Lex.identifier();
        getNextToken();

        std::unique_ptr<ExprAST> Init = nullptr;
        if (CurTok == '=') {
            getNextToken();

            Init = ParseExpression();
            if (!Init)
                return nullptr;
        }

        VarNames.push_back(std::make_pair(Name, std::move(Init)));

        if (CurTok != ',')
            break;
        getNextToken();

        if (CurTok != tok_identifier)
            return LogError("expected identifier list after var");
    }

    if (CurTok != tok_in)
        return LogError("expected 'in' keyword after 'var'");
    getNextToken();

    auto Body = ParseExpression();
    if (!Body)
        return nullptr;

    auto result = makeLocated<VarExprAST>(Loc, std::move(VarNames), std::move(Body));
    result->setVariableLocations(std::move(VariableLocations));
    return result;
}

// primary
//  ::= identifier
//  ::= numberexpr
//  ::= parenexpr
//  ::= ifexpr
//  ::= forexpr
//  ::= varexpr
std::unique_ptr<ExprAST> Parser::ParsePrimary() {
    switch (CurTok) {
    default:
        return LogError("unkonwn token when expecting an expression");
    case tok_identifier:
        return ParseIdentifierExpr();
    case tok_number:
        return ParseNumberExpr();
    case '(':
        return ParseParenExpr();
    case tok_if:
        return ParseIfExpr();
    case tok_for:
        return ParseForExpr();
    case tok_var:
        return ParseVarExpr();
    }
}

// unary
//  ::= primary
//  ::= 'op' unary
std::unique_ptr<ExprAST> Parser::ParseUnary() {
    if (!isascii(CurTok) || CurTok == '(' || CurTok == ',')
        return ParsePrimary();

    auto Loc = Lex.location();
    int Opc = CurTok;
    getNextToken();
    if (auto Operand = ParseUnary())
        return makeLocated<UnaryExprAST>(Loc, Opc, std::move(Operand));

    return nullptr;
}

// binoprhs ::= ('+' unary)*
std::unique_ptr<ExprAST> Parser::ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS) {
    while (true) {
        int TokPrec = GetTokPrecedence();
        if (TokPrec < ExprPrec)
            return LHS;

        auto Loc = Lex.location();
        int BinOp = CurTok;
        getNextToken();

        auto RHS = ParseUnary();
        if (!RHS)
            return nullptr;

        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec) {
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            if (!RHS)
                return nullptr;
        }

        LHS = makeLocated<BinaryExprAST>(Loc, BinOp, std::move(LHS), std::move(RHS));
    }
}

// expression ::= unary binoprhs
std::unique_ptr<ExprAST> Parser::ParseExpression() {
    auto LHS = ParseUnary();
    if (!LHS)
        return nullptr;

    return ParseBinOpRHS(0, std::move(LHS));
}

// prototype
//  ::= id '('id*')'
//  ::= binary 'op' number? (id,id)
//  ::= unary 'op' (id)
std::unique_ptr<PrototypeAST> Parser::ParsePrototype() {
    auto Loc = Lex.location();
    std::string FnName;
    unsigned Kind = 0; // 0=identifier, 1=unary, 2=binary
    unsigned BinaryPrecedence = 30;

    switch (CurTok) {
    default:
        return LogErrorP("Expected function name in prototype");

    case tok_identifier:
        FnName = Lex.identifier();
        Kind = 0;
        getNextToken();
        break;

    case tok_unary:
        getNextToken();
        if (!isascii(CurTok))
            return LogErrorP("Expected unary operator");
        FnName = "unary";
        FnName += (char)CurTok;
        Kind = 1;
        getNextToken();
        break;

    case tok_binary:
        getNextToken();
        if (!isascii(CurTok))
            return LogErrorP("Expected binary operator");
        FnName = "binary";
        FnName += (char)CurTok;
        Kind = 2;
        getNextToken();
        if (CurTok == tok_number) {
            if (Lex.number() < 1 || Lex.number() > 100)
                return LogErrorP("Invalid precedence: must be 1...100");
            BinaryPrecedence = (unsigned)Lex.number();
            getNextToken();
        }
        break;
    }

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");

    std::vector<std::string> ArgNames;
    std::vector<SourceLocation> ArgLocations;
    while (getNextToken() == tok_identifier) {
        ArgNames.push_back(Lex.identifier());
        ArgLocations.push_back(Lex.location());
    }
    if (CurTok != ')')
        return LogErrorP("Expected ')' in prototype");

    getNextToken();

    if (Kind && ArgNames.size() != Kind)
        return LogErrorP("Invalid number of operands for operator");

    auto result = makeLocated<PrototypeAST>(Loc, FnName, ArgNames, Kind != 0, BinaryPrecedence);
    result->setArgumentLocations(std::move(ArgLocations));
    return result;
}

// definition ::= 'def' prototype expression
std::unique_ptr<FunctionAST> Parser::ParseDefinition() {
    getNextToken();
    auto Proto = ParsePrototype();
    if (!Proto)
        return nullptr;

    if (auto E = ParseExpression())
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    return nullptr;
}

// toplevelexpr ::= expression
std::unique_ptr<FunctionAST> Parser::ParseTopLevelExpr() {
    auto Loc = Lex.location();
    if (auto E = ParseExpression()) {
        auto Proto = makeLocated<PrototypeAST>(Loc, "__anon_expr", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

// external ::= 'extern' prototype
std::unique_ptr<PrototypeAST> Parser::ParseExtern() {
    getNextToken();
    return ParsePrototype();
}

} // namespace kaleidoscope
