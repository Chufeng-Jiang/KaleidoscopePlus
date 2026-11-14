"""Parser, scope legality, metadata, codegen and negative tests for teaching #acc."""
import pathlib
import subprocess
import sys
import tempfile
compiler,native,root=sys.argv[1:]
source=(pathlib.Path(root)/"examples/AccVector.kale").read_text()
def run(args,ok=True,contains=()):
    p=subprocess.run([str(a) for a in args],capture_output=True,text=True)
    assert (p.returncode==0)==ok,(args,p.returncode,p.stdout,p.stderr)
    for text in contains: assert text in p.stdout+p.stderr,(text,p.stdout,p.stderr)
    return p
with tempfile.TemporaryDirectory() as tmp:
    d=pathlib.Path(tmp); src=d/"input.kale"; src.write_text(source)
    run([compiler,src],contains=("kale.acc_plan","copyin","copyout"))
    run([compiler,"--emit-llvm",src],contains=("define i32 @kacc_vector","@krt_acc_map"))
    run([compiler,"--emit-ptx",src],contains=(".entry kale_map",))
    run([compiler,"--emit-ptx","--gpu-map","other",src],ok=False,contains=("conflicts",))
    run([native,src,"-o",d/"legacy.o"],ok=False,contains=("Error:",))
    directive="#acc parallel loop copyin(x,y) copyout(out)"
    invalid=[
        (source.replace(directive,directive+" async"),"unsupported"),
        (source.replace("parallel loop","kernels"),"unsupported"),
        (source.replace("copyin(x,y)","copyin(x,x)"),"first two"),
        (source.replace("copyout(out)","copyout(y)"),"third"),
        (source.replace("copyin(x,y)","copyin(missing,y)"),"first two"),
        (source.replace("if 0i32 < size(x)","if 1i32 < size(x)"),"outer"),
        (source.replace("i < size(x)-1i32","i < size(x)"),"condition"),
        (source.replace("0i32, i <","1i32, i <"),"starting at zero"),
        (source.replace("size(x)-1i32 in","size(x)-1i32, 2i32 in"),"unit step"),
        (source.replace("load(x,i)","load(x,i+1i32)"),"cross-element"),
        (source.replace("store(out, i,","store(out, i+1i32,"),"out[i]"),
        (source.replace("a*x+y","if a then x else y"),"pure double"),
        (source.replace("a*x+y","x=1"),"pure double"),
        (source.replace(directive,directive+"\n"+directive),"immediately precede"),
        (source+"\n"+directive,"Only one"),
        ("def element(x y a) a*x+y;\n"+directive,"immediately precede"),
        (source.replace(directive,directive+"\nextern extra();"),"immediately precede"),
    ]
    artifact=d/"keep.o"
    for program,message in invalid:
        src.write_text(program); artifact.write_text("KEEP")
        p=run([compiler,"--emit-object",src,"-o",artifact],ok=False,contains=(message,"input.kale:"))
        assert not p.stdout and artifact.read_text()=="KEEP"
    # Marker near-matches remain ordinary comments.
    src.write_text(source.replace("#acc ","#account "))
    p=run([compiler,src])
    assert "kale.acc_plan" not in p.stdout
    # Newlines do not change directive interpretation.
    src.write_bytes(source.replace("\n","\r\n").encode())
    run([compiler,src],contains=("kale.acc_plan",))
    # Explicit unit step is accepted.
    src.write_text(source.replace("size(x)-1i32 in","size(x)-1i32, 1i32 in"))
    run([compiler,src],contains=("kale.acc_plan",))
print("Teaching #acc parsing, legality, host/PTX codegen and failures passed")
