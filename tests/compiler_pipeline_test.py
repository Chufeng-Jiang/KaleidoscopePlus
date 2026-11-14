"""Exercise public optimization/output options and semantic error recovery."""
import pathlib
import re
import subprocess
import sys
import tempfile

kale, cxx, llvm_as, opt, llc = sys.argv[1:6]

def run(args, *, source=None, expected=0, cwd=None):
    r = subprocess.run(args, input=source, text=True, capture_output=True, timeout=30, cwd=cwd)
    assert r.returncode == expected, f"{args}\n{r.stdout}\n{r.stderr}"
    return r

with tempfile.TemporaryDirectory(prefix="kale pipeline-") as directory:
    tmp = pathlib.Path(directory)
    source = tmp / "sample.kale"
    program = """extern twice(x);
extern twice(y);
def twice(value) var result = value * 2 in result + 1;
def fib(x) if x < 3 then 1 else fib(x-1)+fib(x-2);
def zero(x) x * 0;
def identity(x) x;
"""
    source.write_text(program)
    driver = tmp / "driver.cpp"
    driver.write_text("""#include <cmath>
#include <limits>
extern "C" double twice(double), fib(double), zero(double), identity(double);
int main() {
    return !(twice(2) == 5 && fib(10) == 55 &&
             std::isnan(zero(std::numeric_limits<double>::infinity())) &&
             std::signbit(identity(-0.0)));
}
""")
    irs = {}
    for level in range(4):
        flag = f"-O{level}"
        obj = tmp / f"level{level}.o"
        result = run([kale, flag, str(source), "-o", str(obj)])
        assert not result.stdout and not result.stderr, result.stderr
        executable = tmp / f"level{level}"
        run([cxx, str(driver), str(obj), "-o", str(executable)])
        run([str(executable)])
        jit = run([kale, flag], source=program + "twice(2); fib(10);")
        assert re.findall(r"Evaluated to ([0-9.]+)", jit.stderr) == ["5.000000", "55.000000"], jit.stderr
        ir = tmp / f"level{level}.ll"
        run([kale, flag, "--emit-llvm", str(source), "-o", str(ir)])
        irs[level] = ir.read_text()
        assert "@twice.1" not in irs[level]
        bitcode = tmp / f"level{level}.bc"
        run([llvm_as, str(ir), "-o", str(bitcode)])
        run([opt, "-passes=verify", "-disable-output", str(bitcode)])
        roundtrip = tmp / f"roundtrip{level}.o"
        run([llc, "-filetype=obj", "-relocation-model=pic", str(bitcode), "-o", str(roundtrip)])
        run([cxx, str(driver), str(roundtrip), "-o", str(executable)])
        run([str(executable)])
    assert "alloca double" in irs[0] and "alloca double" not in irs[2]
    result = run([kale, "--emit-llvm", str(source), "-o", "-"])
    assert "define" in result.stdout and not result.stderr
    run([llvm_as, "-o", str(tmp / "stdout.bc")], source=result.stdout)
    run([kale, "--emit-llvm", str(source)])
    assert pathlib.Path(str(source) + ".ll").is_file()
    debug_ir = run([kale, "-g", "-O0", "--emit-llvm", str(source), "-o", "-"]).stdout
    assert "!DICompileUnit" in debug_ir and "alloca double" in debug_ir
    run([llvm_as, "-o", str(tmp / "debug.bc")], source=debug_ir)
    dash = tmp / "-source.kale"
    dash.write_text(program)
    run([kale, "--", dash.name], cwd=tmp)
    assert pathlib.Path(str(dash) + ".o").exists()

    for args in (["-O4"], ["-o"], ["--emit-llvm"], ["-o", "x"],
                 ["-g", "-O2", str(source)], ["-O3", "-g", str(source)],
                 [str(source), "-o", "-"]):
        run([kale, *args], expected=1)
    # Direct paths, symlink aliases, and hard links must not overwrite source bytes.
    aliases = [source, tmp / "symlink.kale", tmp / "hardlink.kale"]
    aliases[1].symlink_to(source)
    aliases[2].hardlink_to(source)
    for alias in aliases:
        r = run([kale, str(source), "-o", str(alias)], expected=1)
        assert "must not overwrite" in r.stderr, r.stderr
        assert source.read_text() == program
    run([kale, str(source), "-o", str(tmp)], expected=1)
    run([kale, str(source), "-o", str(tmp / "missing/out.o")], expected=1)
    saved = tmp / "saved.o"
    saved.write_bytes(b"previous successful output")
    bad = tmp / "bad.kale"
    for text, message in (
        ("extern f(x); extern f(x y);", "signature does not match"),
        ("def f(x x) x;", "Duplicate parameter name"),
        ("def f(x) missing;", "Unknown variable name"),
    ):
        bad.write_text(text)
        r = run([kale, str(bad), "-o", str(saved)], expected=1)
        assert message in r.stderr, r.stderr
        assert saved.read_bytes() == b"previous successful output"
    assert not list(tmp.glob("*.tmp-*")), "temporary outputs leaked"

    # A failed definition/declaration must not poison a previously usable prototype.
    for text, values in (
        ("extern f(x); def f(x y) x; def f(x) x; f(7);", ["7.000000"]),
        ("extern f(x); def f(x) missing; def f(x) x; f(8);", ["8.000000"]),
        ("def f(x) x; extern f(x y); f(9);", ["9.000000"]),
        ("def f(x) x; def f(x) x+1; f(10);", ["10.000000"]),
        ("def binary + 30 (a b) missing; 1+2*3;", ["7.000000"]),
    ):
        r = run([kale], source=text)
        assert "Error:" in r.stderr, r.stderr
        assert re.findall(r"Evaluated to ([0-9.]+)", r.stderr) == values, r.stderr
print("Optimization, IR round trips, safe output, floating-point behavior, and recovery passed")
