# Kaleidoscope

A small LLVM-based compiler supporting interactive JIT execution and native object file generation.

## Module Structure

```text
include/kaleidoscope/  Public interfaces: Token, Lexer, AST, Parser, Diagnostics, Driver
src/
  Lexer.cpp           Input stream → tokens; each Lexer owns its cursor and token values
  Parser.cpp          Recursive descent and operator precedence parsing → AST
  Diagnostics.cpp     Per-session error state and diagnostic output
  CodegenContext.h    Internal backend state and AST visitor; not a public interface
  Codegen.cpp         LLVM IR generation, optimization pipeline, and module initialization
  KaleidoscopeJIT.*   ORC JIT declarations and implementation (original LLVM license retained)
  Driver.cpp          Input files, REPL, JIT/object emission, and session lifetime
  Runtime.cpp         Exported printd / putchard runtime functions
  main.cpp            Calls the driver entry point only
 tests/               Frontend isolation tests and end-to-end regression tests
```

`kaleidoscope_frontend` contains the lexer, parser, and diagnostics. It can be linked
independently without LLVM headers or libraries. AST nodes own their children and
are processed by the backend through the `ExprVisitor` interface, which only
forward-declares `llvm::Value` and includes no LLVM implementation.

The backend owns the LLVM context, module, builder, optimization managers, symbol
tables, and JIT in `CodegenContext`; there is no mutable file-scope global state.
`Compiler` manages component lifetimes and explicitly shares the operator precedence
table between the parser and backend to preserve custom operator semantics.
Runtime functions are compiled directly into the executable and exported for JIT lookup.

## Building

Requirements: a C++17 compiler, CMake 3.20+, and LLVM development packages
(verified with LLVM 21.1.8). Tests are enabled by default and also require Python 3;
disable them with `-DBUILD_TESTING=OFF`.

```bash
cmake -S . -B build -DLLVM_DIR=/usr/lib/llvm-21/lib/cmake/llvm -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Set `LLVM_DIR` to the `lib/cmake/llvm` directory of your LLVM installation.
The compatibility build script also builds the project and copies the executable
to `kale` in the project root:

```bash
bash compile.sh -DLLVM_DIR=/usr/lib/llvm-21/lib/cmake/llvm
```

## Usage

Run without arguments to enter JIT mode. End the input with Ctrl-D to exit:

```bash
printf 'def twice(x) x*2; twice(21);\n' | ./build/kale
```

Pass a file path to generate `<input-path>.o` alongside the input file,
preserving the original command-line interface:

```bash
./build/kale Fib.kale
c++ Fib_Driver.cpp Fib.kale.o -o build/fib
./build/fib

./build/kale Mandelbrot.kale
c++ Mandelbrot_Driver.cpp Mandelbrot.kale.o -o build/mandelbrot
./build/mandelbrot
```

The language retains support for numbers, functions, `extern`, conditional expressions,
`for`, `var`, assignment, and custom unary/binary operators. Function prototype
parameters are separated by spaces; function call arguments are separated by commas.

## Validation and Maintenance

- `frontend_sessions` links only the frontend library and verifies state isolation
  across multiple Lexer, Parser, and Diagnostics instances.
- `compiler_regression` covers operator precedence, function calls, conditionals,
  loops, variables, custom operators, runtime symbols, error recovery, and rejection
  of invalid source files. It also compiles, links, and runs the Fib and Mandelbrot examples.
- `.clang-format` standardizes C++ formatting. Add syntax in Token/Parser/AST,
  implement new IR generation logic in the backend visitor, and keep command-line
  workflows in Driver to avoid accumulating logic in main again.

This refactoring preserves the existing language semantics and REPL output format.
Supporting a non-LLVM backend would still require changing the visitor result type;
this prototype does not introduce a full multi-backend framework.
