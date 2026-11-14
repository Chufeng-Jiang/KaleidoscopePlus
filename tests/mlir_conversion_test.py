"""End-to-end tests: frontend, registered ops, legality, and SSA replacement."""
import pathlib
import subprocess
import sys
import tempfile

compiler, optimizer = sys.argv[1:]


def run(args, text=None, ok=True, contains=()):
    p = subprocess.run(args, input=text, text=True, capture_output=True)
    assert (p.returncode == 0) == ok, (args, p.returncode, p.stdout, p.stderr)
    for item in contains:
        assert item in p.stdout + p.stderr, (item, p.stdout, p.stderr)
    return p.stdout


pipeline = [optimizer, "--pass-pipeline=builtin.module(kale-to-arith)"]
with tempfile.TemporaryDirectory() as directory:
    source = pathlib.Path(directory) / "test.kale"

    def compile_text(text, lower=False, ok=True, contains=()):
        source.write_text(text)
        args = [compiler, str(source)]
        if lower:
            args.append("--lower-to-arith")
        output = run(args, ok=ok, contains=contains)
        if not ok:
            assert not output, "Failed compilation must not print partial IR"
        return output

    raw = compile_text(
        "def chain(a b) (a+2)*(b-3);"
        "def integer(a: i32 b: i32): i32 (a+2i32)*(b-3i32);"
        "def truth(): bool true; def identity(x: bool): bool x;",
        contains=("kale.constant", "kale.add", "kale.sub", "kale.mul", "i1"))
    lowered = run(pipeline, raw, contains=(
        "arith.addf", "arith.subf", "arith.mulf",
        "arith.addi", "arith.subi", "arith.muli", "return"))
    assert "kale." not in lowered
    # The printed output must parse and verify, and the pass must be idempotent.
    assert run(pipeline, lowered) == lowered
    direct = compile_text(source.read_text(), lower=True)
    assert direct.strip() == lowered.strip()

    # Shared block arguments and operation results survive operand remapping.
    shared = """module {
      func.func @shared(%x: f64) -> f64 {
        %a = kale.add %x, %x : f64
        %b = kale.mul %a, %a : f64
        return %b : f64
      }
    }"""
    run(pipeline, shared, contains=("arith.addf", "arith.mulf"))
    # Full conversion rejects a valid operation outside the target's allowlist.
    illegal = """module {
      func.func @unhandled(%x: i32) -> f64 {
        %a = builtin.unrealized_conversion_cast %x : i32 to f64
        return %a : f64
      }
    }"""
    run(pipeline, illegal, ok=False, contains=("failed to legalize", "unrealized_conversion_cast"))
    # ODS checks numeric operand constraints; bool arithmetic is not legal.
    run([optimizer], """module {
      func.func @bad(%x: i1) -> i1 {
        %a = "kale.add"(%x, %x) : (i1, i1) -> i1
        return %a : i1
      }
    }""", ok=False, contains=("kale.add",))
    # The handwritten constant verifier checks attribute/result agreement.
    run([optimizer], """module {
      func.func @bad() -> f64 {
        %a = "kale.constant"() {value = 1 : i32} : () -> f64
        return %a : f64
      }
    }""", ok=False, contains=("value attribute type must match result type",))
    compile_text("def bad(a: i32): i32 a+1;", ok=False, contains=("Error:",))
    compile_text("def bad(x) missing+x;", ok=False, contains=("Unknown variable",))
    compile_text("global x=1;", ok=False, contains=("function definitions and extern declarations only",))
    compile_text("def f(x) x; def f(x) x;", ok=False, contains=("Error:",))
    compile_text("def bad(x) x+;", ok=False, contains=("Error:",))
    compile_text("def choose(x) if x then x else 0;", lower=True, contains=("scf.if",))
    compile_text("def f(x) x; def g(x) f(x);", lower=True, contains=("call @f",))

print("MLIR conversion tests passed")
