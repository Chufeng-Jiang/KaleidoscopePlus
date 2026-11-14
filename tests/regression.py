"""End-to-end checks for the public JIT and object-file interfaces."""
import pathlib
import re
import subprocess
import sys
import tempfile

kale, cxx, root = sys.argv[1:4]
root = pathlib.Path(root)

def run(args, source=None):
    return subprocess.run(args, input=source, text=True, capture_output=True, timeout=30)

def require(condition, message):
    if not condition:
        raise AssertionError(message)

def jit(source, expected):
    result = run([kale], source)
    require(result.returncode == 0, result.stderr)
    values = [float(v) for v in re.findall(r"Evaluated to ([-0-9.]+)", result.stderr)]
    require(values == expected, f"expected {expected}, got {values}\n{result.stderr}")

jit("# comment\n1 + 2 * 3; (1 + 2) * 3; if 0 then 1 else 8;", [7, 9, 8])
jit("def twice(x) x * 2; twice(4); twice(5);", [8, 10])
jit("def unary - (x) 0-x; -4; def binary % 30 (a b) a*b; 2+3%4;", [-4, 14])
jit("def binary : 1 (x y) y; var a=0 in (for i=1, i<4 in a=a+i):a;", [10])
jit((root / "Fib.kale").read_text() + "\nfib(10); fibi(10);", [55, 55])
jit("extern printd(x); printd(42); extern putchard(x); putchard(65);", [0, 0])
# Errors must not prevent the REPL from processing the following expression.
jit("missing; 6;", [6])

with tempfile.TemporaryDirectory(prefix="kale-test-") as tmp:
    tmp = pathlib.Path(tmp)
    for example in ("Fib", "Mandelbrot"):
        source = tmp / (example + ".kale")
        source.write_text((root / source.name).read_text())
        result = run([kale, str(source)])
        require(result.returncode == 0, result.stderr)
        obj = pathlib.Path(str(source) + ".o")
        require(obj.is_file(), f"missing object: {obj}")
        executable = tmp / example
        result = run([cxx, str(root / (example + "_Driver.cpp")), str(obj), "-o", str(executable)])
        require(result.returncode == 0, result.stderr)
        result = run([str(executable)])
        require(result.returncode == 0, result.stderr)
        if example == "Fib":
            require(result.stdout == "fib(10) : 55\nfibi(10) : 55\n", result.stdout)
        else:
            require(len(result.stderr) > 100, "Mandelbrot did not render")
    for text, diagnostic in [
        ("def bad(x) if x then 1;", "expected else"),
        ("def bad(x) missing;", "Unknown variable"),
        ("def bad(x) 1 = 2;", "must be a variable"),
        ("def f(x) x; def bad(x) f(1,2);", "Incorrect # arguments"),
    ]:
        source = tmp / "invalid.kale"
        source.write_text(text)
        result = run([kale, str(source)])
        require(result.returncode == 1, result.stderr)
        require(diagnostic in result.stderr, result.stderr)
        require(not pathlib.Path(str(source) + ".o").exists(), "invalid input emitted an object")
    result = run([kale, str(tmp / "nonexistent.kale")])
    require(result.returncode == 1 and "Could not open input" in result.stderr, result.stderr)
print("JIT, object emission, example linking, and error handling passed")
