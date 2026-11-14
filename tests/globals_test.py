"""Global storage, external linkage, shadowing, error recovery, and debugger tests."""
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

def run(command, *, source=None, expected=0):
    result = subprocess.run(command, input=source, text=True, capture_output=True, timeout=45)
    assert result.returncode == expected, f"{command}\n{result.stdout}\n{result.stderr}"
    return result

def jit(source, expected, flags=(), error=None):
    result = run([args.kale, *flags], source=source)
    values = [float(v) for v in re.findall(r"Evaluated to ([-0-9.]+)", result.stderr)]
    assert values == expected, result.stderr
    if error:
        assert error in result.stderr, result.stderr
    else:
        assert "Error:" not in result.stderr, result.stderr

with tempfile.TemporaryDirectory(prefix="kale globals-") as directory:
    tmp = pathlib.Path(directory)
    source = tmp / "Globals.kale"
    source.write_bytes((args.root / "examples/Globals.kale").read_bytes())
    driver = args.root / "examples/Globals_Driver.cpp"
    obj = tmp / "globals.o"
    executable = tmp / "globals"

    if not args.debugger_only:
        for flags in (["-O0"], ["-O1"], ["-O2"], ["-O3"], ["-g"]):
            run([args.kale, *flags, str(source), "-o", str(obj)])
            run([args.cxx, str(driver), str(obj), "-o", str(executable)])
            assert run([str(executable)]).stdout == "counter: 15\nlocal: 98\n"
            if flags != ["-g"]:
                jit(source.read_text() + "\ncounter; bump(5); counter; localcounter(1); counter; "
                    "localvar(); loopshadow(); counter; offset; untouched;",
                    [10, 15, 15, 101, 15, 98, 0, 15, -2, 0], flags)
                jit("extern global n; extern global n; def read() n; global n = 3; read(); n=7; read();",
                    [3, 7, 7], flags)
        jit("global n; " + "n=n+1; " * 30 + "n;", list(range(1,31)) + [30])
        jit("global n=-.5; global positive=+2.; global zero=-0; n; positive; zero;", [-.5, 2, 0])
        jit("global n=1; n=8; global n=2; n;", [8, 8], error="already defined")
        jit("global n=5; def n(x) x; n;", [5], error="conflicts with a global")
        jit("def f(x) x; global f=1; f(9);", [9], error="conflicts with a function")
        jit("global n=1+2; global n=4; n;", [4], error="numeric literal")
        jit("global n=1; extern global n=8; n;", [1], error="cannot have an initializer")
        jit("n=3; global n=4; n;", [4], error="Unknown variable")

        # Separate Kaleidoscope translation units share one global through external linkage.
        owner = tmp / "owner.kale"
        consumer = tmp / "consumer.kale"
        owner.write_text("global shared = 4; global minuszero = -0;")
        consumer.write_text("extern global shared; extern global shared; def update(x) shared=shared+x;")
        cross_driver = tmp / "cross.cpp"
        cross_driver.write_text('#include <cmath>\nextern "C" { extern double shared, minuszero; double update(double); }\n'
                                'int main() { return !(update(3)==7 && shared==7 && std::signbit(minuszero)); }\n')
        for unit in (owner, consumer):
            run([args.kale, "-O2", str(unit)])
        run([args.cxx, str(cross_driver), str(owner)+".o", str(consumer)+".o", "-o", str(executable)])
        run([str(executable)])
        # An extern global can also be supplied by C++.
        host_driver = tmp / "host.cpp"
        host_driver.write_text('extern "C" { double shared=20; double update(double); }\n'
                               'int main() { return !(update(3)==23 && shared==23); }\n')
        run([args.cxx, str(host_driver), str(consumer)+".o", "-o", str(executable)])
        run([str(executable)])
        ir = run([args.kale, "--emit-llvm", "-O0", str(owner), "-o", "-"]).stdout
        assert "@shared = global double" in ir and "@minuszero = global double" in ir, ir
        external_ir = run([args.kale, "--emit-llvm", "-O0", str(consumer), "-o", "-"]).stdout
        assert external_ir.count("@shared = external global double") == 1, external_ir
        assert "@shared.1" not in external_ir, external_ir
        if args.llvm_as:
            run([args.llvm_as, "-o", str(tmp / "globals.bc")], source=ir)
            run([args.llvm_as, "-o", str(tmp / "extern.bc")], source=external_ir)
        invalid = tmp / "invalid.kale"
        for program, diagnostic in (
            ("global n=1; global n=2;", "already defined"),
            ("global n; extern n(x);", "conflicts with a global"),
            ("extern n(x); global n;", "conflicts with a function"),
            ("extern global n=1;", "cannot have an initializer"),
            ("global n=1+2;", "numeric literal"),
            ("global n=missing;", "numeric literal"),
            ("global n=.;", "numeric literal"),
            ("global n=1.2.3;", "numeric literal"),
            ("global n=1", "Expected ';'"),
            ("global =1;", "Expected name"),
            ("def f() later; global later=1;", "Unknown variable"),
        ):
            invalid.write_text(program)
            obj.write_bytes(b"old output")
            result = run([args.kale, "-g", str(invalid), "-o", str(obj)], expected=1)
            assert diagnostic in result.stderr and str(invalid)+":" in result.stderr, result.stderr
            assert obj.read_bytes() == b"old output"

    run([args.kale, "-g", str(source), "-o", str(obj)])
    run([args.cxx, "-g", "-O0", str(driver), str(obj), "-o", str(executable)])
    if args.dwarf:
        verified = run([args.dwarf, "--verify", str(obj)]).stdout
        assert "No errors." in verified, verified
        metadata = run([args.dwarf, "--debug-info", str(obj)]).stdout
        for value in ('"counter"', '"offset"', '"untouched"', "DW_TAG_variable", "DW_OP_addr"):
            assert value in metadata, metadata
    if args.lldb:
        commands = ["settings set target.disable-aslr false", "settings set auto-confirm true",
                    "breakpoint set --file Globals.kale --line 5", "run", "frame variable delta",
                    "target variable counter offset untouched", "thread step-out", "target variable counter",
                    "breakpoint delete --force", "breakpoint set --file Globals.kale --line 7",
                    "continue", "frame variable counter", "target variable counter",
                    "breakpoint delete --force", "continue"]
        command = [args.lldb, "--no-lldbinit", "--batch"]
        for item in commands:
            command.extend(["-o", item])
        result = run(command + ["--", str(executable)])
        output = result.stdout + result.stderr
        for expected in ("Globals.kale:5:", "Globals.kale:7:", "(double) delta = 5",
                         "(double) counter = 10", "(double) counter = 15", "(double) counter = 1\n",
                         "(double) offset = -2", "(double) untouched = 0", "exited with status = 0"):
            assert expected in output, output
print("Global variables, linkage, scopes, diagnostics, and enabled debug checks passed")
