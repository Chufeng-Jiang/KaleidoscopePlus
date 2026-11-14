#!/usr/bin/env python3
"""Run the full locally supported benchmark suite. No synthetic/fabricated timings."""
import argparse
import csv
import datetime
import hashlib
import json
import os
import pathlib
import platform
import random
import shutil
import subprocess
import time

ROOT=pathlib.Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument("--build",default="build/mlir")
parser.add_argument("--out",required=True)
parser.add_argument("--samples",type=int,default=21)
parser.add_argument("--warmup",type=int,default=5)
parser.add_argument("--compile-samples",type=int,default=7)
parser.add_argument("--cxx",default="/usr/bin/clang++-23")
args=parser.parse_args()
if args.samples<3 or args.compile_samples<3 or args.warmup<1:
    parser.error("Require >=3 samples and >=1 warmup")
build=(ROOT/args.build).resolve()
out=(ROOT/args.out).resolve()
if out.exists(): parser.error("Output already exists; choose a fresh --out directory")
out.mkdir(parents=True)
work=build/"benchmark-artifacts"/out.name
work.mkdir(parents=True,exist_ok=True)
(out/"sources").mkdir()
(out/"ir").mkdir()
affinity=sorted(os.sched_getaffinity(0))
cpu=affinity[0]
commands=[]
def execute(command,capture=False,pin=False):
    command=[str(x) for x in command]
    if pin:command=["taskset","-c",str(cpu),*command]
    commands.append(command)
    return subprocess.run(command,check=True,text=True,capture_output=capture)
def info(command):
    try:return subprocess.check_output(command,text=True,stderr=subprocess.STDOUT).strip()
    except (OSError,subprocess.CalledProcessError) as e:return str(e)
def gpu():
    return info(["nvidia-smi","--query-gpu=name,driver_version,pstate,temperature.gpu,power.draw,clocks.sm,clocks.mem,utilization.gpu,memory.used",
                 "--format=csv,noheader"])
fingerprints={}
for directory in ("src","include","benchmarks"):
    for p in sorted((ROOT/directory).rglob("*")):
        if p.is_file() and p.suffix in (".cpp",".h",".td",".py",".txt"):
            fingerprints[str(p.relative_to(ROOT))]=hashlib.sha256(p.read_bytes()).hexdigest()
meta={"complete":False,"timestamp_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),
      "platform":platform.platform(),"cpu":info(["lscpu"]),"affinity_available":affinity,
      "benchmark_cpu":cpu,"gpu_before":gpu(),"cxx":info([args.cxx,"--version"]),
      "llvm":info(["/usr/lib/llvm-23/bin/llvm-config","--version"]),
      "cuda":info(["/usr/local/cuda-12.8/bin/nvcc","--version"]),
      "git_head":info(["git","-C",str(ROOT),"rev-parse","HEAD"]),
      "git_status":info(["git","-C",str(ROOT),"status","--short"]),
      "source_sha256":fingerprints,"parameters":vars(args),
      "notes":["FP64 only; CUDA synchronous, pageable host buffers, block size 256.",
               "CPU direct baseline is single-threaded; CPU simulation is not a speedup baseline.",
               "Repeated warm same-buffer measurements; effective traffic is not physical DRAM bandwidth.",
               "All timing samples retained; no outlier removal.",
               "Startup observations are process-first-use, not cold hardware/OS-cache measurements."]}
def save():
    meta["commands"]=commands
    (out/"environment.json").write_text(json.dumps(meta,indent=2))
save()
execute(["cmake","-S",ROOT,"-B",build,"-DKALEIDOSCOPE_BUILD_BENCHMARKS=ON"])
execute(["cmake","--build",build,"--target","kale-benchmark","kale-mlir","kale","-j4"])
runner=build/"benchmarks/kale-benchmark"
mlir=build/"src/backend/mlir/kale-mlir"
native=build/"kale"
flags=["-O2","-ffp-contract=off","-fPIC","-shared","-I",ROOT/"include"]
scalar="""def scalar(x y) (x+y)*(x-y);
def integer(x: i32 y: i32): i32 (x+y)*(x-y);
def predicate(x): bool bool(x);
def loops(n: i32): i32 var s: i32=0i32, ignored=for i: i32=0i32, i<n in s=s+i in s;
"""
src=out/"sources/scalar.kale";src.write_text(scalar)
for tool,name,extra in ((native,"llvm",[]),(mlir,"mlir",["--emit-object"])):
    execute([tool,*extra,src,"-o",work/(name+".o")])
    execute([args.cxx,*flags,work/(name+".o"),"-o",work/(name+".so")])
execute([runner,"scalar",work/"llvm.so",work/"mlir.so",out/"runtime.csv",
         args.samples,args.warmup],pin=True)
template=(ROOT/"examples/AccVector.kale").read_text()
cpp_template="""#include "kaleidoscope/runtime/ArrayABI.h"
#include <cstdlib>
static inline double element(double x,double y,double a) { return EXPR; }
extern "C" void checked(KaleArray *x,KaleArray *y,KaleArray *out,double a) {
  for(int i=0;i<x->sizes[0];++i) {
    if(i>=y->sizes[0] || i>=out->sizes[0]) std::abort();
    out->aligned[out->offset+i]=element(x->aligned[x->offset+i],y->aligned[y->offset+i],a);
  }
}
extern "C" void unchecked(KaleArray *x,KaleArray *y,KaleArray *out,double a) {
  for(int i=0;i<x->sizes[0];++i)
    out->aligned[out->offset+i]=element(x->aligned[x->offset+i],y->aligned[y->offset+i],a);
}
"""
variants={}
for count in (1,8,32,128):
    expression="x"
    for _ in range(count): expression="("+expression+"*a+y)"
    source=out/"sources"/f"vector{count}.kale"
    source.write_text(template.replace("a*x+y",expression))
    cpp=out/"sources"/f"baseline{count}.cpp";cpp.write_text(cpp_template.replace("EXPR",expression))
    obj=work/f"vector{count}.o";so=work/f"vector{count}.so";baseline=work/f"baseline{count}.so"
    ptx=out/"ir"/f"vector{count}.ptx"
    execute([mlir,"--emit-object",source,"-o",obj])
    execute([mlir,"--emit-ptx",source,"-o",ptx])
    execute([mlir,"--emit-llvm",source,"-o",out/"ir"/f"vector{count}.ll"])
    execute([args.cxx,*flags,obj,"-o",so])
    execute([args.cxx,*flags,cpp,"-o",baseline])
    execute([args.cxx,"-O2","-ffp-contract=off","-I",ROOT/"include","-S",cpp,
             "-o",out/"ir"/f"baseline{count}.s"])
    # Audit surviving PTX arithmetic rather than trusting source operation count.
    text=ptx.read_text()
    meta.setdefault("ptx_arithmetic",{})[str(count)]={
        "mul_f64":text.count("mul.rn.f64"),"add_f64":text.count("add.rn.f64"),
        "fma_f64":text.count("fma.rn.f64"),"bytes":ptx.stat().st_size}
    variants[count]=(so,ptx,baseline,source)
lengths="0,1,17,257,4096,65536,262144,1048576,4194304"
so,ptx,baseline,_=variants[1]
execute([runner,"vector",so,ptx,baseline,1,out/"runtime.csv",args.samples,args.warmup,
         lengths,"length"],pin=True)
for count in (1,8,32,128):
    so,ptx,baseline,_=variants[count]
    execute([runner,"vector",so,ptx,baseline,count,out/"runtime.csv",args.samples,args.warmup,
             "65536,1048576","intensity"],pin=True)
so,ptx,_,_=variants[1]
execute([runner,"residency",so,ptx,out/"runtime.csv",max(7,args.samples//3),2,
         "65536,1048576"],pin=True)
# Independent fresh compiler processes, warm filesystem; no compilation overlaps GPU work.
with (out/"compile.csv").open("w",newline="") as f:
    writer=csv.DictWriter(f,fieldnames=["group","functions","mode","sample","wall_ms","peak_rss_kib","output_bytes"])
    writer.writeheader()
    def measure(command,path,group,size,mode,sample):
        rss=work/"rss.txt"
        t=time.perf_counter()
        execute(["/usr/bin/time","-f","%M","-o",rss,*command],capture=True,pin=True)
        elapsed=(time.perf_counter()-t)*1000
        if sample>=0:
            writer.writerow(dict(group=group,functions=size,mode=mode,sample=sample,
                wall_ms=elapsed,peak_rss_kib=int(rss.read_text()),output_bytes=path.stat().st_size))
            f.flush()
    for count in (10,100,500):
        source=out/"sources"/f"compile{count}.kale"
        expression="x"
        for _ in range(16):expression="("+expression+"*y+x)"
        source.write_text("\n".join(f"def f{i}(x y) {expression};" for i in range(count))+"\n")
        for sample in range(-1,args.compile_samples):
            choices=[("llvm",native,[]),("mlir",mlir,["--emit-object"])]
            if sample%2:choices.reverse()
            for mode,tool,options in choices:
                path=work/(mode+"-compile.o")
                measure([tool,*options,source,"-o",path],path,"compiler",count,mode,sample)
        print(f"Compiler size={count} finished",flush=True)
    source=variants[1][3]
    stages=[("kale",[]),("arith",["--lower-to-arith"]),("llvm_dialect",["--emit-llvm-dialect"]),
            ("llvm_ir",["--emit-llvm"]),("object",["--emit-object"]),("ptx",["--emit-ptx"])]
    rng=random.Random(20261114)
    for sample in range(-1,args.compile_samples):
        order=stages.copy();rng.shuffle(order)
        for mode,options in order:
            path=work/(mode+".stage")
            measure([mlir,*options,source,"-o",path],path,"stage",2,mode,sample)
meta["gpu_after"]=gpu()
meta["complete"]=True
meta["finished_utc"]=datetime.datetime.now(datetime.timezone.utc).isoformat()
save()
print(f"Complete: {out}",flush=True)
