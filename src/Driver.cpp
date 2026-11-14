#include "kaleidoscope/Driver.h"
#include "CodegenContext.h"
#include "CompilerOptions.h"
#include "Optimizer.h"
#include "Output.h"
#include "kaleidoscope/Parser.h"
#include "kaleidoscope/SemanticAnalyzer.h"
#include "kaleidoscope/SourceManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include <cstdio>
#include <fstream>
#include <iostream>

namespace kaleidoscope {

class Compiler {
  public:
    Compiler()
        : Source(std::cin.rdbuf()), Diag(std::cerr, &SourceText), Lex(Source, &SourceText),
          Syntax(Lex, Diag, Operators), Semantics(Diag, Operators), Context(Diag) {}
    int run(int argc, char *argv[]);

  private:
    std::ifstream Input;
    std::istream Source;
    SourceManager SourceText;
    Diagnostics Diag;
    OperatorTable Operators{{'=', 2}, {'<', 10}, {'+', 20}, {'-', 20}, {'*', 40}};
    Lexer Lex;
    Parser Syntax;
    SemanticAnalyzer Semantics;
    CodegenContext Context;
    void HandleDefinition();
    void HandleExtern();
    void HandleGlobal(bool external = false);
    bool SubmitJITModule();
    void HandleTopLevelExpression();
    void MainLoop();
};

void Compiler::HandleDefinition() {
    if (auto FnAST = Syntax.ParseDefinition()) {
        if (!Semantics.analyze(*FnAST))
            return;
        if (auto *FnIR = Context.generate(*FnAST)) {
            Semantics.commit(*FnAST);
            if (!Context.FileMode) {
                fprintf(stderr, "Read function definition:");
                FnIR->print(errs());
                fprintf(stderr, "\n");
            }

            if (!Context.FileMode)
                SubmitJITModule();
        }
    } else
        Syntax.synchronize();
}

void Compiler::HandleExtern() {
    Syntax.getNextToken();
    if (Syntax.currentToken() == tok_global) {
        HandleGlobal(true);
        return;
    }
    if (auto ProtoAST = Syntax.ParseExtern(true)) {
        if (!Semantics.analyze(*ProtoAST))
            return;
        if (auto *FnIR = Context.generate(*ProtoAST)) {
            Semantics.commit(*ProtoAST);
            if (!Context.FileMode) {
                fprintf(stderr, "Read extern");
                FnIR->print(errs());
                fprintf(stderr, "\n");
            }

            Context.FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
        }
    } else
        Syntax.synchronize();
}

void Compiler::HandleTopLevelExpression() {
    if (auto FnAST = Syntax.ParseTopLevelExpr()) {
        if (!Semantics.analyze(*FnAST, !Context.FileMode))
            return;
        if (!Context.generate(*FnAST))
            return;
        Semantics.commit(*FnAST, !Context.FileMode);
        if (!Context.FileMode) {
            if (!optimizeModule(*Context.TheModule, nullptr, Context.Optimization)) {
                Diag.error("Invalid generated module");
                return;
            }
            auto RT = Context.TheJIT->getMainJITDylib().createResourceTracker();
            auto TSM =
                ThreadSafeModule(std::move(Context.TheModule), std::move(Context.TheContext));
            Context.ExitOnErr(Context.TheJIT->addModule(std::move(TSM), RT));
            Context.initializeModule();

            auto ExprSymbol = Context.ExitOnErr(Context.TheJIT->lookup("__anon_expr"));
            double (*FP)() = ExprSymbol.toPtr<double (*)()>();
            fprintf(stderr, "Evaluated to %f\n", FP());

            Context.ExitOnErr(RT->remove());
        }
    } else
        Syntax.synchronize();
}

bool Compiler::SubmitJITModule() {
    if (!optimizeModule(*Context.TheModule, nullptr, Context.Optimization)) {
        Diag.error("Invalid generated module");
        return false;
    }
    Context.ExitOnErr(Context.TheJIT->addModule(
        ThreadSafeModule(std::move(Context.TheModule), std::move(Context.TheContext))));
    Context.initializeModule();
    return true;
}

void Compiler::HandleGlobal(bool external) {
    if (auto global = Syntax.ParseGlobal(external)) {
        if (!Semantics.analyze(*global))
            return;
        if (!Context.generate(*global))
            return;
        Semantics.commit(*global);
        if (!Context.FileMode && !global->isExternal()) {
            // Global definitions belong to the persistent JIT tracker, not a temporary expression.
            SubmitJITModule();
        }
    } else
        Syntax.synchronize();
}

// top ::= definition | global | external | expression | ';'
void Compiler::MainLoop() {
    while (true) {
        if (!Context.FileMode)
            fprintf(stderr, "ready> ");
        switch (Syntax.currentToken()) {
        case tok_eof:
            return;
        case ';':
            Syntax.getNextToken();
            break;
        case tok_global:
            HandleGlobal();
            break;
        case tok_def:
            HandleDefinition();
            break;
        case tok_extern:
            HandleExtern();
            break;
        default:
            HandleTopLevelExpression();
            break;
        }
    }
}

int Compiler::run(int argc, char *argv[]) {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    CompilerOptions options;
    if (!parseOptions(argc, argv, options, std::cerr)) {
        printUsage(std::cerr);
        return 1;
    }
    if (options.Help) {
        printUsage(std::cout);
        return 0;
    }
    Context.EmitDebugInfo = options.Debug;
    Context.Optimization = options.Optimization;
    const auto codegenLevel = static_cast<CodeGenOptLevel>(options.Optimization);
    const auto &InputFile = options.InputFile;
    if (!InputFile.empty()) {
        Context.FileMode = true;
        Context.SourceFile = InputFile;
        Diag.setSourceFile(InputFile);
        Input.open(InputFile);
        if (!Input) {
            errs() << "Could not open input file: " << InputFile;
            return 1;
        }

        Source.rdbuf(Input.rdbuf());

        Triple TargetTriple(sys::getDefaultTargetTriple());
        std::string Error;
        auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);
        if (!Target) {
            errs() << Error;
            return 1;
        }

        TargetOptions Opt;
        Context.TheTM.reset(Target->createTargetMachine(TargetTriple, "generic", "", Opt,
                                                        Reloc::PIC_, std::nullopt, codegenLevel));
    } else {
        Context.TheJIT = Context.ExitOnErr(KaleidoscopeJIT::Create(codegenLevel));
    }

    if (!Context.FileMode)
        fprintf(stderr, "ready> ");
    Syntax.getNextToken();

    Context.initializeModule();

    MainLoop();

    if (Context.FileMode) {
        if (Context.Debug)
            Context.Debug->finalize();
        if (Diag.hasError()) {
            errs() << "Compile error!";
            return 1;
        }

        if (!optimizeModule(*Context.TheModule, Context.TheTM.get(), Context.Optimization)) {
            errs() << "Invalid generated module\n";
            return 1;
        }
        if (!emitOutput(*Context.TheModule, *Context.TheTM, options.EmitIR, options.InputFile,
                        options.OutputFile))
            return 1;
    }

    return 0;
}

int runCompiler(int argc, char *argv[]) {
    Compiler compiler;
    return compiler.run(argc, argv);
}

} // namespace kaleidoscope
