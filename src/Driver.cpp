#include "kaleidoscope/Driver.h"
#include "CodegenContext.h"
#include "kaleidoscope/Parser.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
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
        : Source(std::cin.rdbuf()), Diag(std::cerr), Lex(Source), Syntax(Lex, Diag, Operators),
          Context(Diag, Operators) {}
    int run(int argc, char *argv[]);

  private:
    std::ifstream Input;
    std::istream Source;
    Diagnostics Diag;
    OperatorTable Operators{{'=', 2}, {'<', 10}, {'+', 20}, {'-', 20}, {'*', 40}};
    Lexer Lex;
    Parser Syntax;
    CodegenContext Context;
    void HandleDefinition();
    void HandleExtern();
    void HandleTopLevelExpression();
    void MainLoop();
};

void Compiler::HandleDefinition() {
    if (auto FnAST = Syntax.ParseDefinition()) {
        if (auto *FnIR = Context.generate(*FnAST)) {
            fprintf(stderr, "Read function definition:");
            FnIR->print(errs());
            fprintf(stderr, "\n");

            if (!Context.EmitObject) {
                Context.ExitOnErr(Context.TheJIT->addModule(
                    ThreadSafeModule(std::move(Context.TheModule), std::move(Context.TheContext))));
                Context.initializeModule();
            }
        }
    } else
        Syntax.getNextToken();
}

void Compiler::HandleExtern() {
    if (auto ProtoAST = Syntax.ParseExtern()) {
        if (auto *FnIR = Context.generate(*ProtoAST)) {
            fprintf(stderr, "Read extern");
            FnIR->print(errs());
            fprintf(stderr, "\n");

            Context.FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
        }
    } else
        Syntax.getNextToken();
}

void Compiler::HandleTopLevelExpression() {
    if (auto FnAST = Syntax.ParseTopLevelExpr()) {
        if (Context.generate(*FnAST) && !Context.EmitObject) {
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
        Syntax.getNextToken();
}

// top ::= definition | external | expression | ';'
void Compiler::MainLoop() {
    while (true) {
        fprintf(stderr, "ready> ");
        switch (Syntax.currentToken()) {
        case tok_eof:
            return;
        case ';':
            Syntax.getNextToken();
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

    std::string OutputFile;
    if (argc == 2) {
        Context.EmitObject = true;
        std::string InputFile = argv[1];
        OutputFile = InputFile + ".o";
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
        Context.TheTM.reset(
            Target->createTargetMachine(TargetTriple, "generic", "", Opt, Reloc::PIC_));
    } else {
        Context.TheJIT = Context.ExitOnErr(KaleidoscopeJIT::Create());
    }

    fprintf(stderr, "ready> ");
    Syntax.getNextToken();

    Context.initializeModule();

    MainLoop();

    if (Context.EmitObject) {
        if (Diag.hasError()) {
            errs() << "Compile error!";
            return 1;
        }

        std::error_code EC;
        raw_fd_ostream Dest(OutputFile, EC, sys::fs::OF_None);
        if (EC) {
            errs() << "Could not open output file: " << EC.message();
            return 1;
        }

        legacy::PassManager Pass;
        if (Context.TheTM->addPassesToEmitFile(Pass, Dest, nullptr, CodeGenFileType::ObjectFile)) {
            errs() << "Could not emit object of this type";
            return 1;
        }

        Pass.run(*Context.TheModule);
        Dest.flush();
    }

    return 0;
}

int runCompiler(int argc, char *argv[]) {
    Compiler compiler;
    return compiler.run(argc, argv);
}

} // namespace kaleidoscope
