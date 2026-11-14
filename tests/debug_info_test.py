"""Validate DWARF and, when available, exercise source debugging in LLDB."""
import pathlib
import subprocess
import sys
import tempfile

kale, cxx, root, dwarf = sys.argv[1:5]
lldb = sys.argv[5] if len(sys.argv) > 5 else None
root = pathlib.Path(root)

def run(args, expected=0):
    result = subprocess.run(args, text=True, capture_output=True, timeout=45)
    output = result.stdout + result.stderr
    if result.returncode != expected:
        raise AssertionError(f"{args}: {result.returncode}\n{output}")
    return output

def require(condition, output):
    if not condition:
        raise AssertionError(output)

with tempfile.TemporaryDirectory(prefix="kale debug-") as directory:
    tmp = pathlib.Path(directory)
    source = tmp / "Debug.kale"
    source.write_bytes((root / "examples/Debug.kale").read_bytes())
    obj = pathlib.Path(str(source) + ".o")
    executable = tmp / "debug-demo"
    run([kale, "-g", str(source)])
    verified = run([dwarf, "--verify", str(obj)])
    require("No errors." in verified, verified)
    metadata = run([dwarf, "--debug-info", "--debug-line", str(obj)])
    for expected in ("DW_TAG_compile_unit", "DW_TAG_subprogram", "DW_TAG_formal_parameter",
                     "DW_TAG_variable", "DW_TAG_lexical_block", "DW_TAG_base_type",
                     '"debugdemo"', '"helper"', '"double"', '"x"', '"y"', '"i"', str(tmp)):
        require(expected in metadata, metadata)
    run([cxx, "-g", "-O0", str(root / "examples/Debug_Driver.cpp"), str(obj), "-o", str(executable)])
    require("debugdemo(3): 11" in run([str(executable)]), "wrong debug program result")

    if lldb:
        commands = [
            "settings set target.disable-aslr false",
            "settings set auto-confirm true",
            "breakpoint set --file Debug.kale --line 5", "run", "frame variable x y",
            "thread step-in", "frame variable x", "thread backtrace",
            "breakpoint delete --force",
            "breakpoint set --file Debug.kale --line 7", "continue", "frame variable x y",
            "breakpoint delete --force",
            "breakpoint set --file Debug.kale --line 9", "continue", "frame variable x y i",
            "breakpoint delete --force",
            "breakpoint set --file Debug.kale --line 10", "continue", "frame variable x y",
            "continue",
        ]
        args = [lldb, "--no-lldbinit", "--batch"]
        for command in commands:
            args += ["-o", command]
        output = run(args + ["--", str(executable)])
        for expected in ("Debug.kale:5:", "Debug.kale:2:", "Debug.kale:7:",
                         "Debug.kale:9:", "Debug.kale:10:", "helper(x=4)",
                         "debugdemo(x=3)", "(double) x = 3", "(double) x = 4",
                         "(double) y = 4", "(double) y = 13", "(double) y = 8",
                         "(double) i = 1", "(double) y = 11", "exited with status = 0"):
            require(expected in output, output)
        print("LLDB: source breakpoints, step-in, backtrace, parameters, shadowing, and loop locals passed")

    # Ordinary compilation must keep its original behavior and omit debug metadata.
    run([kale, str(source)])
    metadata = run([dwarf, "--debug-info", str(obj)])
    require("DW_TAG_compile_unit" not in metadata, metadata)
    run([cxx, str(root / "examples/Debug_Driver.cpp"), str(obj), "-o", str(executable)])
    run([str(executable)])
    invalid = tmp / "invalid.kale"
    for program, position, message in (
        ("def broken(x)\n  missing;", ":2:3:", "Unknown variable name"),
        ("def broken(x)\n  if x then 1;", ":2:14:", "expected else"),
        ("def broken(x)\n  var y = missing in y;\ndef good(x) x;", ":2:11:", "Unknown variable name"),
    ):
        invalid.write_text(program)
        output = run([kale, "-g", str(invalid)], expected=1)
        require(str(invalid) + position + " Error: " + message in output, output)
        require(not pathlib.Path(str(invalid) + ".o").exists(), "invalid source emitted object")
    require("requires an input file" in run([kale, "-g"], expected=1), "-g must reject JIT mode")
    run([kale, "--unknown"], expected=1)
    run([kale, str(source), str(source)], expected=1)
    require("Usage:" in run([kale, "--help"]), "missing help")
print("DWARF metadata, optimized mode, source diagnostics, and CLI checks passed")
