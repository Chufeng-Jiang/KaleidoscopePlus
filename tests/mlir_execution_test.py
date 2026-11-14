"""Native differential execution, memref ABI, and conversion/export failures."""
import ctypes as C
import math
import pathlib
import resource
import signal
import subprocess
import sys
import tempfile

compiler, native, cxx, root, assembler = sys.argv[1:]
root = pathlib.Path(root)
def run(args, ok=True, contains=None):
    p = subprocess.run([str(a) for a in args], text=True, capture_output=True)
    if (p.returncode == 0) != ok:
        raise AssertionError((args,p.returncode,p.stdout,p.stderr))
    if contains and contains not in p.stdout+p.stderr:
        raise AssertionError((contains,p.stdout,p.stderr))
    return p.stdout

source = """
def fp(x y) (x+y)*(x-y);
def integer(x: i32 y: i32): i32 (x+y)*(x-y);
def choose(flag: bool x: i32 y: i32): i32 if flag then x else y;
def invert(flag: bool): bool !flag;
def less(x y) x<y;
def condition(x): bool bool(x);
def convert(x): i32 i32(x);
def inttodouble(x: i32) double(x);
def sign(x: i32): i32 -x;
def booltodouble(x: bool) double(x);
def booltoint(x: bool): i32 i32(x);
def count(n: i32): i32
  var s: i32=0i32, ignored=for i: i32=0i32, i<n in s=s+1i32 in s;
def guarded(n: i32): i32 if 0i32<n then count(n-1i32) else 0i32;
def shadow(x: i32): i32
  var y: i32 = (var x: i32=x+1i32 in x*2i32) in x+y;
def fib(n: i32): i32 if n<3i32 then 1i32 else fib(n-1i32)+fib(n-2i32);
extern later(x: i32): i32;
def caller(x: i32): i32 later(x)+1i32;
def later(x: i32): i32 x*2i32;
# Step is evaluated before the condition; both precede reading/updating i.
def effects(): i32
  var s: i32=0i32, ignored=for i: i32=0i32, (s=s+1i32)<5i32, (s=s+1i32) in s=s+1i32
  in s;
def nested(n: i32): i32
  var s: i32=0i32, ignored=for i: i32=0i32, i<n in
    for j: i32=0i32, j<1i32 in s=s+1i32
  in s;
"""
with tempfile.TemporaryDirectory() as tmp:
    d=pathlib.Path(tmp)
    src=d/"scalar.kale"; src.write_text(source)
    libs=[]
    for exe, name, options in ((native,"old",[]),(compiler,"new",["--emit-object"])):
        obj=d/(name+".o"); so=d/(name+".so")
        run([exe,*options,src,"-o",obj])
        run([cxx,"-shared",obj,"-o",so])
        libs.append(C.CDLL(str(so)))
    def compare(name, restype, argtypes, cases):
        fns=[getattr(lib,name) for lib in libs]
        for fn in fns: fn.argtypes=argtypes; fn.restype=restype
        for args in cases:
            a,b=[fn(*args) for fn in fns]
            if isinstance(a,float):
                assert (math.isnan(a) and math.isnan(b)) or a==b, (name,args,a,b)
                if a==0 and b==0:
                    assert math.copysign(1,a)==math.copysign(1,b), (name,args,a,b)
            else: assert a==b,(name,args,a,b)
    compare("fp",C.c_double,[C.c_double]*2,[(1.5,2.5),(-4,8),(0.,-0.),(1e20,1e-5)])
    compare("integer",C.c_int32,[C.c_int32]*2,[(1,2),(2147483647,1),(-2147483648,7)])
    compare("choose",C.c_int32,[C.c_bool,C.c_int32,C.c_int32],[(True,11,22),(False,11,22)])
    compare("invert",C.c_bool,[C.c_bool],[(True,),(False,)])
    compare("less",C.c_double,[C.c_double]*2,[(1,2),(2,1),(math.nan,1),(1,math.nan)])
    floats=[0.,-0.,math.nan,math.inf,-math.inf,3.9,-3.9,2147483648.,-2147483649.]
    compare("condition",C.c_bool,[C.c_double],[(v,) for v in floats])
    compare("convert",C.c_int32,[C.c_double],[(v,) for v in floats])
    compare("inttodouble",C.c_double,[C.c_int32],[(0,),(-1,),(-2147483648,)])
    compare("sign",C.c_int32,[C.c_int32],[(0,),(1,),(-2147483648,)])
    compare("booltodouble",C.c_double,[C.c_bool],[(False,),(True,)])
    compare("booltoint",C.c_int32,[C.c_bool],[(False,),(True,)])
    for name,cases in [
        ("count",[(-1,),(0,),(1,),(17,)]),("guarded",[(0,),(1,),(17,)]),
        ("shadow",[(3,),(-7,)]),("fib",[(1,),(10,)]),("caller",[(12,)]),
        ("effects",[()]),("nested",[(0,),(1,),(3,)])]:
        compare(name,C.c_int32,[] if name=="effects" else [C.c_int32],cases)
    assert libs[1].count(0)==1 and libs[1].guarded(0)==0
    assert libs[1].effects()==6
    text=run([compiler,"--emit-llvm",src])
    assert "define" in text and "target triple" in text and "zeroext" in text
    dialect=run([compiler,"--emit-llvm-dialect",src])
    assert "llvm.func" in dialect and "scf." not in dialect and "kale." not in dialect
    # Compile textual LLVM IR separately, proving export is consumable.
    ll=d/"export.ll"; ll.write_text(text)
    llvm_as=pathlib.Path(assembler)
    run([llvm_as,ll,"-o",d/"export.bc"])

    class Array(C.Structure):
        _fields_=[("allocated",C.POINTER(C.c_double)),("aligned",C.POINTER(C.c_double)),
                  ("offset",C.c_int64),("sizes",C.c_int64*1),("strides",C.c_int64*1)]
    def desc(buf,n): return Array(buf,buf,0,(C.c_int64*1)(n),(C.c_int64*1)(1))
    obj=d/"vector.o"; so=d/"vector.so"
    run([compiler,"--emit-object",root/"examples/Vector.kale","-o",obj])
    run([cxx,"-shared",obj,"-o",so])
    lib=C.CDLL(str(so))
    vector=lib._mlir_ciface_vector
    vector.restype=C.c_double
    vector.argtypes=[C.POINTER(Array)]*3+[C.c_double]
    for n in [0,1,17,257,100003]:
        x=(C.c_double*(n+1))(); y=(C.c_double*(n+1))(); out=(C.c_double*(n+1))()
        for i in range(n): x[i]=i*.125; y[i]=i*.25
        out[n]=-9876
        dx,dy,do=desc(x,n),desc(y,n),desc(out,n)
        vector(C.byref(dx),C.byref(dy),C.byref(do),2.25)
        for i in range(n): assert out[i]==2.25*x[i]+y[i],(n,i,out[i])
        assert out[n]==-9876
    # Conversion failure and translation failure are separate contracts.
    raw=d/"untranslated.mlir"
    raw.write_text('module { func.func @f() -> f64 { %0 = arith.constant 1.0 : f64 return %0 : f64 } }')
    run([compiler,"--input-mlir","--translate-only","--emit-llvm",raw],
        ok=False,contains="LLVM IR translation failed")
    raw.write_text('module { func.func @f(%x: i32) -> f64 { %0 = builtin.unrealized_conversion_cast %x : i32 to f64 return %0 : f64 } }')
    run([compiler,"--input-mlir","--emit-llvm",raw],ok=False,contains="failed to legalize")
    # Failed emission never replaces a previous artifact.
    artifact=d/"keep.o"; artifact.write_text("KEEP")
    bad=d/"bad.kale"
    bad.write_text("def bad(x: i32): i32 x+1;")
    run([compiler,"--emit-object",bad,"-o",artifact],ok=False)
    assert artifact.read_text()=="KEEP"
    run([compiler,"--emit-object",src,"-o",src],ok=False,contains="aliases input")
    assert src.read_text()==source
    run([compiler,"--emit-object",src,"-o",d/"missing"/"out.o"],ok=False,contains="Cannot create output")
    for program in ["def bad(x: array): array x;", "def bad(x: array) x+1;",
                    "def bad(x: array) load(x,1);", "def bad(x: array) store(x,0i32,true);",
                    "def bad(x: array) bool(x);", "global x: array;",
                    "def bad() var x: array in 0;"]:
        bad.write_text(program)
        run([compiler,bad],ok=False)
    # Native-only path rejects MLIR-only arrays through diagnostics, not a crash.
    bad.write_text("def bad(x: array) 0;")
    run([native,bad,"-o",artifact],ok=False,contains="Invalid parameter type")

    # Bounds checking: test in a child process because cf.assert terminates execution.
    bad.write_text("def access(x: array i: i32) load(x,i);")
    run([compiler,"--emit-object",bad,"-o",obj]); run([cxx,"-shared",obj,"-o",d/"bounds.so"])
    child = """
import ctypes as C, resource, sys
resource.setrlimit(resource.RLIMIT_CORE,(0,0))
class A(C.Structure):
 _fields_=[('p',C.POINTER(C.c_double)),('q',C.POINTER(C.c_double)),('off',C.c_int64),('n',C.c_int64),('s',C.c_int64)]
buf=(C.c_double*1)(1.)
desc=A(buf,buf,0,1,1)
lib=C.CDLL(sys.argv[1]); fn=lib._mlir_ciface_access
fn.argtypes=[C.POINTER(A),C.c_int32]; fn.restype=C.c_double
fn(C.byref(desc),int(sys.argv[2]))
"""
    p=subprocess.run([sys.executable,"-c",child,str(d/"bounds.so"),"0"],capture_output=True,text=True)
    assert p.returncode==0,(p.returncode,p.stderr)
    assert "Kaleidoscope array index out of bounds" in run([compiler,bad])
    for index in [-1,1]:
        p=subprocess.run([sys.executable,"-c",child,str(d/"bounds.so"),str(index)],capture_output=True,text=True)
        assert p.returncode == -signal.SIGABRT,(p.returncode,p.stderr)
    # PTX validates the requested kernel ABI before writing output.
    run([compiler,"--emit-ptx","--gpu-map","choose",src],ok=False,contains="GPU map")
    ptx=run([compiler,"--emit-ptx","--gpu-map","element",root/"examples/Vector.kale"])
    assert ".entry kale_map" in ptx and ".target sm_86" in ptx
print("MLIR execution, scalar parity, array ABI, and negative tests passed")
