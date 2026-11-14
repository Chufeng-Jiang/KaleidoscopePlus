"""Typed scalar syntax, semantics, native ABI, JIT, IR, and DWARF regression tests."""
import argparse
import pathlib
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("kale")
parser.add_argument("cxx")
parser.add_argument("root", type=pathlib.Path)
parser.add_argument("--dwarf")
parser.add_argument("--llvm-as", dest="llvm_as")
parser.add_argument("--lldb")
parser.add_argument("--debugger-only", action="store_true")
args = parser.parse_args()

def run(command, source=None, expected=0):
    result = subprocess.run(command, input=source, text=True, capture_output=True, timeout=45)
    assert result.returncode == expected, f"{command}\n{result.stdout}\n{result.stderr}"
    return result

def jit(source, expected, flag="-O2", error=None):
    result = run([args.kale, flag], source)
    actual = [float(x) for x in re.findall(r"Evaluated to ([-0-9.]+)", result.stderr)]
    assert actual == expected, result.stderr
    if error:
        assert error in result.stderr, result.stderr
    else:
        assert "Error:" not in result.stderr, result.stderr

with tempfile.TemporaryDirectory(prefix="kale typed-") as directory:
    tmp = pathlib.Path(directory)
    source = tmp / "Types.kale"
    source.write_text((args.root / "examples/Types.kale").read_text())
    obj, executable = tmp / "types.o", tmp / "types"
    driver = args.root / "examples/Types_Driver.cpp"
    if not args.debugger_only:
        for flag in ("-O0", "-O1", "-O2", "-O3", "-g"):
            run([args.kale, flag, str(source), "-o", str(obj)])
            run([args.cxx, "-g", "-O0", str(driver), str(obj), "-o", str(executable)])
            assert run([str(executable)]).stdout == "Typed scalars: passed\n"
            if flag != "-g":
                jit(source.read_text() + "increment(5i32); less(-1i32,2i32); choose(false,1i32,2i32); "
                    "fibint(7i32); localtyped(4i32,true); looptotal(4i32); toggled(false); "
                    "counter; enabled=false; enabled;", [3,1,2,13,5,10,1,3,0,0], flag)
                jit("2147483647i32+1i32; -2147483648i32-1i32; 65536i32*65536i32; "
                    "var x:i32=-2147483648i32 in -x; i32(2147483648); i32(0-2147483649); "
                    "i32(3.9); i32(0-3.9); double(true); i32(false); bool(-2i32); "
                    "if 0i32 then 1i32 else 2i32; var a:bool,b:i32 in if a then b else 3i32;",
                    [-2147483648,2147483647,0,-2147483648,2147483647,-2147483648,3,-3,1,0,1,2,3], flag)
                jit("extern global shared:i32; def get():i32 shared; global shared:i32=7i32; "
                    "get(); var shared:bool=true in shared; get();", [7,1,7], flag)
                jit("def binary % 30 (a:i32 b:i32):i32 a*b; 3i32%4i32; "
                    "def unary ~(x:bool):bool !x; ~false;", [12,1], flag)
        invalid = [
            ("def bad(x:i32):i32 x+1;", "expected i32, got double"),
            ("def bad():bool 1i32;", "Function return value"),
            ("def bad():i32 if true then 1i32 else 2;", "Conditional branch"),
            ("def bad() var x:i32=1 in double(x);", "Variable initializer"),
            ("def bad() var x:bool=true in x=1i32;", "Assignment"),
            ("def f(x:i32):i32 x; def bad():i32 f(1);", "Argument 1"),
            ("extern f(x:i32):bool; def f(x:double):bool true;", "signature does not match"),
            ("extern f(x:i32):bool; extern f(x:i32):i32;", "signature does not match"),
            ("extern global x:i32; global x:bool=true;", "Global type does not match"),
            ("global x:i32=1;", "Global initializer"),
            ("global x:bool=1i32;", "Global initializer"),
            ("def bad() true+false;", "Arithmetic and ordering"),
            ("def bad() for i:bool=true,false in 0;", "Loop variable"),
            ("def bad() for i:i32=0i32,i<2i32,1 in 0;", "Loop step"),
            ("2147483648i32;", "outside the i32 range"),
            ("-2147483649i32;", "outside the i32 range"),
            ("global x:i32=2147483648i32;", "outside the i32 range"),
            ("1.5i32;", "Invalid numeric literal"),
            ("def f(x:unknown) x;", "Expected type"),
            ("i32();", "exactly one argument"),
            ("bool(1,2);", "exactly one argument"),
        ]
        bad = tmp / "bad.kale"
        for program, message in invalid:
            bad.write_text(program)
            obj.write_bytes(b"previous output")
            result = run([args.kale, str(bad), "-o", str(obj)], expected=1)
            assert message in result.stderr and "Internal error" not in result.stderr, result.stderr
            assert obj.read_bytes() == b"previous output"
        jit("def f(x:i32):i32 x+1; def f(x:i32):i32 x+1i32; f(2i32);", [3], error="expected i32")
        ir = tmp / "types.ll"
        run([args.kale, "-g", "--emit-llvm", str(source), "-o", str(ir)])
        text = ir.read_text()
        for marker in ("i32 @", "zeroext i1 @", "icmp slt i32", "llvm.fptosi.sat.i32.f64", 'name: "i32"', 'name: "bool"'):
            assert marker in text, marker
        if args.llvm_as:
            run([args.llvm_as, str(ir), "-o", str(tmp / "types.bc")])
        # Typed declarations remain compatible across independently compiled modules.
        owner, consumer, native = tmp / "owner.kale", tmp / "consumer.kale", tmp / "native.cpp"
        owner.write_text("global shared:i32=4i32; global flag:bool=true; def select(x:bool):i32 if x then shared else 0i32;")
        consumer.write_text("extern global shared:i32; extern global flag:bool; extern select(x:bool):i32; "
                            "extern nativebool(x:bool):bool; extern nativeint(x:i32):i32; "
                            "def update():i32 shared=nativeint(select(nativebool(flag))); ")
        native.write_text('extern "C" int update(); extern "C" bool nativebool(bool x) { return x; } '
                          'extern "C" int nativeint(int x) { return x+3; } int main() { return update()!=7; }')
        run([args.kale, str(owner)])
        run([args.kale, str(consumer)])
        run([args.cxx, str(native), str(owner)+".o", str(consumer)+".o", "-o", str(executable)])
        run([str(executable)])
    # Rebuild the normal example for debug inspection after negative/output-preservation tests.
    run([args.kale, "-g", str(source), "-o", str(obj)])
    run([args.cxx, "-g", "-O0", str(driver), str(obj), "-o", str(executable)])
    if args.dwarf:
        run([args.dwarf, "--verify", str(obj)])
        dwarf = run([args.dwarf, "--debug-info", str(obj)]).stdout
        assert '"i32"' in dwarf and '"bool"' in dwarf and "DW_ATE_signed" in dwarf and "DW_ATE_boolean" in dwarf
    if args.debugger_only:
        result = run([args.lldb, "--batch", "-o", "settings set target.disable-aslr false",
            "-o", "breakpoint set --file Types.kale --line 14", "-o", "run",
            "-o", "frame variable x flag y active", "-o", "target variable counter enabled",
            "-o", "breakpoint delete --force", "-o", "continue", str(executable)])
        output = (result.stdout + result.stderr).replace("(int)", "(i32)")
        for marker in ("(i32) x = 4", "(bool) flag = true", "(i32) y = 5", "(bool) active = true",
                       "(i32) counter = 3", "(bool) enabled = true", "Typed scalars: passed"):
            assert marker in output, output
print("Typed language, ABI, conversions, diagnostics, and enabled debug checks passed")
