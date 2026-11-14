# Kaleidoscope

A small LLVM-based compiler supporting interactive JIT execution and native object file generation.

## Module Structure

```text
include/kaleidoscope/  Public interfaces: Token, SourceLocation, Lexer, AST, Parser, Diagnostics, Driver
src/
  Lexer.cpp           Input stream → tokens; each Lexer owns its cursor and token values
  Parser.cpp          Recursive descent and operator precedence parsing → AST
  Diagnostics.cpp     Per-session errors with source file, line, and column
  CodegenContext.h    Internal backend state and AST visitor; not a public interface
  Codegen.cpp         LLVM IR generation, optimization pipeline, and module initialization
  DebugInfo.*         DWARF compilation units, functions, lexical scopes, and variables
  KaleidoscopeJIT.*   ORC JIT declarations and implementation (original LLVM license retained)
  Driver.cpp          Input files, REPL, JIT/object emission, and session lifetime
  Runtime.cpp         Exported printd / putchard runtime functions
  main.cpp            Calls the driver entry point only
examples/             Kaleidoscope programs and their C++ drivers
tests/                Frontend isolation tests and end-to-end regression tests
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
./build/kale examples/Fib.kale
c++ examples/Fib_Driver.cpp examples/Fib.kale.o -o build/fib
./build/fib

./build/kale examples/Mandelbrot.kale
c++ examples/Mandelbrot_Driver.cpp examples/Mandelbrot.kale.o -o build/mandelbrot
./build/mandelbrot
```

The language retains support for numbers, functions, `extern`, conditional expressions,
`for`, `var`, assignment, and custom unary/binary operators. Function prototype
parameters are separated by spaces; function call arguments are separated by commas.

## Source-Level Debugging

Compile a source file with `-g` to emit DWARF debug information. This disables
both IR optimization and machine-code optimization for that compilation, preserving
parameter and local-variable storage for inspection. Normal object compilation and
interactive JIT retain their existing optimization pipeline.

```bash
./build/kale -g examples/Debug.kale
c++ -g -O0 examples/Debug_Driver.cpp examples/Debug.kale.o -o build/debug-demo
lldb-21 build/debug-demo
```

Use `lldb` instead if your installation uses an unversioned executable name.
Inside LLDB:

```text
breakpoint set --file Debug.kale --line 5
run
frame variable x y
thread step-in
frame variable x
thread backtrace
```

At line 5, `x` is 3 and `y` is 4. Stepping into `helper` shows its parameter
`x` as 4. Additional breakpoints at lines 7, 9, and 10 demonstrate the inner
shadowed `y` (13), the loop variables (`y = 8`, `i = 1` on the first iteration),
and the restored outer `y` (11). The program prints `debugdemo(3): 11`.

The generated object contains standard DWARF 4 metadata for source files,
functions, double types, parameters, `var` declarations, and `for` variables.
Lexical scopes distinguish shadowed names. Source paths are recorded as an absolute
directory plus filename, so keep the source files available to the debugger.
The implementation follows LLVM's [source-level debugging model](https://llvm.org/docs/SourceLevelDebugging.html).
LLDB 21 has been tested end to end; GDB has not been tested in this environment.

`-g` currently requires a source file: JIT source debugging is not implemented.
It generates an object file, which still needs to be linked with a C++ driver.
Building the compiler itself with `CMAKE_BUILD_TYPE=Debug` is independent of this option.
Run `./build/kale --help` for command-line usage. `compile.sh` remains unchanged.

Lexer tokens and AST nodes retain one-based source lines and byte columns, with
LF, CRLF, and CR line endings supported (a tab counts as one byte). Parser and
code-generation errors include locations even without `-g`, for example:

```text
examples/invalid.kale:2:3: Error: Unknown variable name
```

## Validation and Maintenance

- `frontend_sessions` links only the frontend library and verifies state isolation
  across multiple Lexer, Parser, and Diagnostics instances, plus source positions
  and positioned syntax errors.
- `compiler_regression` covers operator precedence, function calls, conditionals,
  loops, variables, custom operators, runtime symbols, error recovery, and rejection
  of invalid source files. It also compiles, links, and runs the Fib and Mandelbrot examples.
- `compiler_debug_regression` runs the same object-file checks with `-g`.
- `debug_info` uses `llvm-dwarfdump` to verify DWARF and checks diagnostics and CLI
  behavior. It also confirms ordinary compilation omits debug metadata.
- `debugger_integration` uses LLDB to verify source breakpoints, step-in, backtraces,
  parameters, local variables, shadowing, and loop variables. It requires permission
  to launch and trace a child process. CMake enables these last two tests when the
  corresponding tools are found and reports when they are unavailable.
- `.clang-format` standardizes C++ formatting. Add syntax in Token/Parser/AST,
  implement new IR generation logic in the backend visitor, and keep command-line
  workflows in Driver to avoid accumulating logic in main again.

This refactoring preserves the existing language semantics and REPL output format.
Supporting a non-LLVM backend would still require changing the visitor result type;
this prototype does not introduce a full multi-backend framework.
