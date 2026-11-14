#pragma once
#include "DebugInfo.h"
#include "KaleidoscopeJIT.h"
#include "kaleidoscope/frontend/AST.h"
#include "kaleidoscope/support/Diagnostics.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Target/TargetMachine.h"
#include <map>
#include <memory>

#include <string>

namespace kaleidoscope {

using namespace llvm;
using namespace llvm::orc;

struct CodegenContext : ExprVisitor {
    explicit CodegenContext(Diagnostics &diagnostics) : Diag(diagnostics) {}
    Diagnostics &Diag;
    std::unique_ptr<LLVMContext> TheContext;
    std::unique_ptr<Module> TheModule;
    std::unique_ptr<IRBuilder<>> Builder;
    std::map<std::string, AllocaInst *> NamedValues;

    std::unique_ptr<KaleidoscopeJIT> TheJIT;
    ExitOnError ExitOnErr;
    std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

    // Lowering caches only: semantic validity/definitions belong to SemanticAnalyzer.
    std::map<std::string, Type> GlobalTypes;
    bool FileMode = false;
    bool EmitDebugInfo = false;
    unsigned Optimization = 2;
    std::string SourceFile;
    std::unique_ptr<DebugInfo> Debug;
    SourceLocation CurrentLocation;

    // Restore both diagnostic and instruction locations after visiting a child.
    class LocationGuard {
        CodegenContext &Context;
        SourceLocation Previous;
        llvm::DebugLoc PreviousDebug;

      public:
        LocationGuard(CodegenContext &context, SourceLocation location)
            : Context(context), Previous(context.CurrentLocation),
              PreviousDebug(context.Builder->getCurrentDebugLocation()) {
            Context.CurrentLocation = location;
            if (Context.Debug)
                Context.Debug->emitLocation(*Context.Builder, location);
        }
        ~LocationGuard() {
            Context.CurrentLocation = Previous;
            Context.Builder->SetCurrentDebugLocation(PreviousDebug);
        }
        LocationGuard(const LocationGuard &) = delete;
        LocationGuard &operator=(const LocationGuard &) = delete;
    };
    std::unique_ptr<TargetMachine> TheTM;

    llvm::Type *lowerType(Type type);
    Constant *scalarConstant(double value, Type type);
    Value *convert(Value *value, Type destination);
    Value *condition(Value *value);
    Value *call(Function *function, ArrayRef<Value *> arguments, const Twine &name);
    Value *getVariableStorage(const std::string &name);
    GlobalVariable *generate(const GlobalAST &node);
    Value *LogErrorV(const char *message);
    Function *getFunction(std::string name);
    AllocaInst *CreateEntryBlockAlloca(Function *function, StringRef name, Type type);
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
