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
  Codegen.cpp         LLVM IR generation, symbol validation, and module initialization
  Globals.cpp         Global storage, external declarations, and local-first variable lookup
  CompilerOptions.*  Command-line options and option compatibility checks
  Optimizer.*        LLVM default module optimization pipelines and IR verification
  Output.*           Textual IR/object emission with transactional file replacement
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

The backend owns the LLVM context, module, builder, symbol tables, and JIT in
`CodegenContext`; optimization managers live within each optimizer invocation.
There is no mutable file-scope global state.
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
Global variables are also supported as described below.

## Global Variables

Global variables are mutable `double` values. Use `global` at the top level;
a declaration must end with a semicolon:

```text
global counter = 10;
global offset = -2.5;
global untouched;
extern global shared;
```

An omitted initializer means zero. Initializers accept a single decimal numeric
literal with an optional `+` or `-` sign, including `.5`, `2.`, and `-0`.
Arithmetic expressions, function calls, and references to other variables are not
allowed in an initializer; perform those computations through assignments instead.
Malformed numeric literals such as `.` and `1.2.3`, and literals outside the
supported conversion range, produce an error rather than silently accepting a prefix.

Declare a global before using it in a function. `extern global name;` can precede
a later definition in the same file or declare storage supplied by another object
file. An extern declaration never allocates or initializes storage and cannot have
an initializer. Repeated extern declarations are allowed, but a global has only one
definition. Globals and functions share the top-level symbol namespace; conflicts
are diagnosed. `global` is now a reserved keyword.

Parameters, `var` declarations, and `for` variables shadow globals of the same name.
Their initializers can still read the outer binding, and leaving the local scope
restores lookup of the outer variable. Assignments update the nearest visible binding.

Try persistent state in the JIT:

```bash
printf 'global counter=10; def bump(delta) counter=counter+delta; bump(5); counter;\n' | ./build/kale
```

Both expressions evaluate to 15. Global storage persists for the JIT session and
is not removed when temporary top-level expressions are unloaded. A global is
initialized once, not each time a function or expression references it. An extern
global must be supplied before code referencing it is executed; unresolved external
symbols remain a linker/JIT error, just as with external functions.

Compile and run the complete example, which checks global mutation, zero/default
initialization, parameter shadowing, `var` shadowing, and loop-variable shadowing:

```bash
./build/kale -O2 examples/Globals.kale -o build/globals.o
c++ examples/Globals_Driver.cpp build/globals.o -o build/globals
./build/globals
```

Expected output:

```text
counter: 15
local: 98
```

Globals use LLVM's [external-linkage global variables](https://llvm.org/docs/LangRef.html#global-variables).
A C++ consumer can declare a Kaleidoscope global as `extern "C" { extern double counter; }`.
Conversely, a C++ definition such as `extern "C" { double shared = 4; }` can provide
storage for `extern global shared;`. All participating declarations must agree on
the `double` type; the compiler does not validate the ABI of a separately supplied object.

To inspect globals and distinguish them from shadowing locals, build with `-g`:

```bash
./build/kale -g examples/Globals.kale -o build/globals.o
c++ -g -O0 examples/Globals_Driver.cpp build/globals.o -o build/globals
lldb-21 build/globals
```

Inside LLDB:

```text
breakpoint set --file Globals.kale --line 5
run
frame variable delta
target variable counter offset untouched
thread step-out
target variable counter
breakpoint delete --force
breakpoint set --file Globals.kale --line 7
continue
frame variable counter
target variable counter
```

The first breakpoint shows `delta = 5`, global `counter = 10`, `offset = -2`, and
`untouched = 0`. After stepping out, global `counter` is 15. At the second breakpoint,
the parameter `counter` is 1 while the global remains 15. Debug metadata for a global
definition lives in its defining compilation unit; an extern declaration relies on
the defining object for that global's debug information.

## Compilation Options and LLVM IR

```text
kale [-g] [-O0|-O1|-O2|-O3] [--emit-llvm] [-o output] [--] [source.kale]
```

| Option | Behavior |
| --- | --- |
| `-O0` | Skip LLVM IR optimization passes and use unoptimized machine-code generation |
| `-O1`, `-O2`, `-O3` | Use LLVM's corresponding default module pipeline and machine-code optimization level |
| `-g` | Emit DWARF and select `-O0`; combining it with a final positive optimization level is an error |
| `--emit-llvm` | Emit textual LLVM IR instead of an object file; requires an input file |
| `-o path` | Choose the output path; requires an input file |
| `-o -` | Write IR to stdout; requires `--emit-llvm` |
| `--` | End option parsing, allowing an input filename starting with `-` |

The default optimization level is `-O2`, for both file compilation and JIT.
Repeated `-O` options use the last specified level. Without `-o`, file compilation
writes `<input>.o`, or `<input>.ll` with `--emit-llvm`. File compilation is quiet
on success; interactive prompts and expression results remain in JIT mode.

Build an optimized object at a chosen location:

```bash
./build/kale -O2 examples/Fib.kale -o build/fib.o
c++ examples/Fib_Driver.cpp build/fib.o -o build/fib
./build/fib
```

Inspect and validate IR, then compile it through the external LLVM tools:

```bash
./build/kale -O0 --emit-llvm examples/Fib.kale -o build/fib-O0.ll
./build/kale -O2 --emit-llvm examples/Fib.kale -o build/fib-O2.ll
/usr/lib/llvm-21/bin/llvm-as build/fib-O2.ll -o build/fib.bc
/usr/lib/llvm-21/bin/opt -passes=verify -disable-output build/fib.bc
/usr/lib/llvm-21/bin/llc -filetype=obj -relocation-model=pic build/fib.bc -o build/fib-from-ir.o
c++ examples/Fib_Driver.cpp build/fib-from-ir.o -o build/fib-from-ir
./build/fib-from-ir
```

Adjust the LLVM tool paths to your installation. Both executables print 55 for
`fib(10)` and `fibi(10)`. The emitted IR includes the native target triple and data
layout; this CLI does not yet provide cross-compilation or promise arbitrary
retargeting of emitted IR. IR can also be piped without prompts mixed into stdout:

```bash
./build/kale --emit-llvm examples/Fib.kale -o - | /usr/lib/llvm-21/bin/llvm-as -o build/piped.bc
printf 'def twice(x) x*2; twice(21);\n' | ./build/kale -O3
```

Functions and complete modules are verified before use, and modules are verified
again after optimization. No fast-math flags are enabled. Repeated compatible
`extern` declarations reuse a symbol; conflicting signatures, duplicate parameter
names, and repeated definitions are rejected. Failed definitions restore operator
precedence and leave previously accepted prototypes available to the REPL.

File output is written to a temporary file next to the destination and renamed
only after successful emission and close. Failed compilation leaves any previous
output intact, so always check the compiler's exit status before using an existing
artifact. Output paths that alias the input (including symlinks/hard links),
directories, and special files are rejected. IR verification and these checks do
not sandbox JIT programs or validate arbitrary external-function ABI contracts.

## Source-Level Debugging

Compile a source file with `-g` to emit DWARF debug information. This disables
both IR optimization and machine-code optimization for that compilation, preserving
parameter and local-variable storage for inspection. Normal object compilation and
interactive JIT use the selected optimization level (default `-O2`).

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
functions, double types, parameters, global definitions, `var` declarations, and `for` variables.
Lexical scopes distinguish shadowed names. Source paths are recorded as an absolute
directory plus filename, so keep the source files available to the debugger.
The implementation follows LLVM's [source-level debugging model](https://llvm.org/docs/SourceLevelDebugging.html).
LLDB 21 has been tested end to end; GDB has not been tested in this environment.

`-g` currently requires a source file: JIT source debugging is not implemented.
By default it generates an object file to link with a C++ driver. Combining
`-g --emit-llvm` instead emits unoptimized IR containing debug metadata.
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
- `compiler_pipeline` checks all four optimization levels in JIT and object mode,
  validates IR with `llvm-as`/`opt`, rebuilds it with `llc`, and tests floating-point
  behavior, declaration recovery, and output protection. CMake enables it when
  those three LLVM tools are available.
- `global_variables` checks globals at all optimization levels and with `-g`,
  including persistent JIT state, forward declarations, cross-file linkage,
  C++ interoperability, shadowing, invalid declarations, and preserved output on errors.
  It verifies global IR/DWARF when the corresponding LLVM tools are available.
- `global_debugger` uses LLDB to inspect global values before and after assignment
  and distinguish a shadowing parameter from the global. It is enabled when LLDB
  and `llvm-dwarfdump` are available.
- `.clang-format` standardizes C++ formatting. Add syntax in Token/Parser/AST,
  implement new IR generation logic in the backend visitor, and keep command-line
  workflows in Driver to avoid accumulating logic in main again.

This refactoring preserves the existing language semantics and REPL output format.
Supporting a non-LLVM backend would still require changing the visitor result type;
this prototype does not introduce a full multi-backend framework.

## Chapter 10 Extension Roadmap

The compiler-workflow improvements apply the [Chapter 10](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl10.html)
discussion of IR, optimization, and language-level checks to the compiler workflow.
Optimization uses LLVM's [new pass manager](https://llvm.org/docs/NewPassManager.html)
and standard pipelines; no language-specific optimization pass is claimed here.

Global variables are implemented, including native linkage and debug information.
Further language features remain separate work: explicit types,
arrays/structures, a richer runtime, heap allocation/GC, and exceptions. Types and
memory semantics should be designed before adding `sizeof`, field offsets, or GC.
The language still uses double values and native compilation targets.
