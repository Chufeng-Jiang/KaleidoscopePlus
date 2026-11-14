"""Source excerpts, caret alignment, previous-declaration notes, and REPL recovery."""
import pathlib
import re
import subprocess
import sys
import tempfile

kale = sys.argv[1]

def run(arguments=(), source=None, expected=0):
    result = subprocess.run([kale, *arguments], input=source, text=True, capture_output=True, timeout=30)
    assert result.returncode == expected, result.stderr
    return result.stderr

def values(output):
    return [float(value) for value in re.findall(r"Evaluated to ([-0-9.]+)", output)]

for text, expected, errors in (
    ("def bad(x) missing+absent; 7;", [7], 2),
    ("def bad(x) x+; def good(x) x; good(9);", [9], 1),
    ("def broken(x) x+ def good(x) x; good(8);", [8], 1),
    ("global broken=3 global good=4; good;", [4], 1),
    ("def bad(x) missing; global bad=3; bad;", [3], 1),
    ("def bad(x x) x; def bad(x) x; bad(6);", [6], 1),
    ("def scope() var local=1 in local; def invalid() local; scope();", [1], 1),
    ("extern printd(x); def bad(x) (x printd(999)); 7;", [7], 1),
    ("global g=2; def read() var g=g+1 in g; read(); g;", [3,2], 0),
):
    output = run(source=text)
    assert values(output) == expected, output
    assert output.count("Error:") == errors, output
    assert "999.000000" not in output, output
    assert "Internal error" not in output, output

with tempfile.TemporaryDirectory(prefix="kale diagnostics-") as directory:
    tmp = pathlib.Path(directory)
    source = tmp / "errors.kale"
    output = tmp / "previous.o"
    output.write_bytes(b"previous output")
    source.write_bytes(b"extern f(x);\r\ndef f(x y) x;\r\ndef bad(x)\r\n\tmissing + absent;\r\ndef good(x) x;\r\n")
    diagnostics = run(["-g", str(source), "-o", str(output)], expected=1)
    for expected in (f"{source}:2:5: Error: Function signature", f"{source}:1:8: Note:",
                     f"{source}:4:2: Error: Unknown variable name 'missing'",
                     f"{source}:4:12: Error: Unknown variable name 'absent'"):
        assert expected in diagnostics, diagnostics
    assert diagnostics.count("Error:") == 3, diagnostics
    assert "          missing + absent;\n          ^" in diagnostics, diagnostics
    assert "          missing + absent;\n                    ^" in diagnostics, diagnostics
    assert "  extern f(x);\n         ^" in diagnostics, diagnostics
    assert output.read_bytes() == b"previous output"
    # An incomplete construct at EOF still has a precise caret.
    source.write_text("def incomplete(x)\n  x +")
    diagnostics = run([str(source), "-o", str(output)], expected=1)
    assert f"{source}:2:6: Error:" in diagnostics, diagnostics
    assert "    x +\n       ^" in diagnostics, diagnostics
    assert output.read_bytes() == b"previous output"
    # Bare CR line endings and no final newline retain source location accuracy.
    source.write_bytes(b"def bad(x)\r  missing;")
    diagnostics = run([str(source), "-o", str(output)], expected=1)
    assert f"{source}:2:3: Error:" in diagnostics and "    missing;\n    ^" in diagnostics, diagnostics
print("Source diagnostics, aligned carets, notes, multiple errors, and recovery passed")
