#pragma once
#include "KaleidoscopeJIT.h"
#include "kaleidoscope/AST.h"
#include "kaleidoscope/Diagnostics.h"
#include "kaleidoscope/Operators.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Target/TargetMachine.h"
#include <map>
#include <memory>
#include <string>

namespace kaleidoscope {

using namespace llvm;
using namespace llvm::orc;

struct CodegenContext : ExprVisitor {
    CodegenContext(Diagnostics &diagnostics, OperatorTable &operators)
        : Diag(diagnostics), BinopPrecedence(operators) {}
    Diagnostics &Diag;
    OperatorTable &BinopPrecedence;
    std::unique_ptr<LLVMContext> TheContext;
    std::unique_ptr<Module> TheModule;
    std::unique_ptr<IRBuilder<>> Builder;
    std::map<std::string, AllocaInst *> NamedValues;

    std::unique_ptr<FunctionPassManager> TheFPM;
    std::unique_ptr<LoopAnalysisManager> TheLAM;
    std::unique_ptr<FunctionAnalysisManager> TheFAM;
    std::unique_ptr<CGSCCAnalysisManager> TheCGAM;
    std::unique_ptr<ModuleAnalysisManager> TheMAM;
    std::unique_ptr<PassInstrumentationCallbacks> ThePIC;
    std::unique_ptr<StandardInstrumentations> TheSI;

    std::unique_ptr<KaleidoscopeJIT> TheJIT;
    ExitOnError ExitOnErr;
    std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

    bool EmitObject = false;
    std::unique_ptr<TargetMachine> TheTM;

    Value *LogErrorV(const char *message);
    Function *getFunction(std::string name);
    AllocaInst *CreateEntryBlockAlloca(Function *function, StringRef name);
    void initializeModule();
    Value *visit(NumberExprAST &node) override;
    Value *visit(VariableExprAST &node) override;
    Value *visit(UnaryExprAST &node) override;
    Value *visit(BinaryExprAST &node) override;
    Value *visit(CallExprAST &node) override;
    Value *visit(IfExprAST &node) override;
    Value *visit(ForExprAST &node) override;
    Value *visit(VarExprAST &node) override;
    Function *generate(PrototypeAST &node);
    Function *generate(FunctionAST &node);
};

} // namespace kaleidoscope
