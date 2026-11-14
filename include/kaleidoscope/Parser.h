#pragma once
#include "kaleidoscope/AST.h"
#include "kaleidoscope/Diagnostics.h"
#include "kaleidoscope/Lexer.h"
#include "kaleidoscope/Operators.h"
namespace kaleidoscope {
class Parser {
  public:
    Parser(Lexer &lexer, Diagnostics &diagnostics, OperatorTable &operators)
        : Lex(lexer), Diag(diagnostics), BinopPrecedence(operators) {}
    int currentToken() const { return CurTok; }
    int getNextToken();
    std::unique_ptr<FunctionAST> ParseDefinition();
    std::unique_ptr<FunctionAST> ParseTopLevelExpr();
    std::unique_ptr<PrototypeAST> ParseExtern();

  private:
    Lexer &Lex;
    Diagnostics &Diag;
    OperatorTable &BinopPrecedence;
    int CurTok = tok_eof;
    int GetTokPrecedence();
    std::unique_ptr<ExprAST> ParseNumberExpr();
    std::unique_ptr<ExprAST> ParseParenExpr();
    std::unique_ptr<ExprAST> ParseIdentifierExpr();
    std::unique_ptr<ExprAST> ParseIfExpr();
    std::unique_ptr<ExprAST> ParseForExpr();
    std::unique_ptr<ExprAST> ParseVarExpr();
    std::unique_ptr<ExprAST> ParsePrimary();
    std::unique_ptr<ExprAST> ParseUnary();
    std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS);
    std::unique_ptr<ExprAST> ParseExpression();
    std::unique_ptr<PrototypeAST> ParsePrototype();
    std::unique_ptr<ExprAST> LogError(const char *message);
    std::unique_ptr<PrototypeAST> LogErrorP(const char *message);
};
} // namespace kaleidoscope
