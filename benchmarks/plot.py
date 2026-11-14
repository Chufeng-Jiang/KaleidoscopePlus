#!/usr/bin/env python3
"""Summarize raw samples and render publication-style static PNG/SVG charts."""
import argparse
import collections
import csv
import json
import pathlib
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT=pathlib.Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument("results")
parser.add_argument("--update-readme",action="store_true")
args=parser.parse_args()
folder=(ROOT/args.results).resolve()
metadata=json.loads((folder/"environment.json").read_text())
if not metadata["complete"]:raise SystemExit("Refuse to report an incomplete run")
with (folder/"runtime.csv").open() as f:raw=list(csv.DictReader(f))
with (folder/"compile.csv").open() as f:comp=list(csv.DictReader(f))
groups=collections.defaultdict(list)
for row in raw:groups[(row["group"],int(row["variant"]),int(row["n"]),int(row["iterations"]),row["mode"])].append(row)
cg=collections.defaultdict(list)
for row in comp:cg[(row["group"],int(row["functions"]),row["mode"])].append(row)
def values(rows,metric):return np.array([float(r[metric]) for r in rows])
def median(rows,metric="wall_ms"):return float(np.median(values(rows,metric)))
def quantile(rows,metric="wall_ms",q=.95):return float(np.quantile(values(rows,metric),q))
def rows(group,mode,n,variant=1,k=1):return groups[(group,variant,n,k,mode)]
def stat(group,mode,n,metric="wall_ms",variant=1,k=1):return median(rows(group,mode,n,variant,k),metric)
def summarize(source,keys,metrics,path):
    result=[]
    for key,items in sorted(source.items()):
        row=dict(zip(keys,key));row["samples"]=len(items)
        for metric in metrics:
            x=values(items,metric)
            for label,value in (("median",np.median(x)),("p05",np.quantile(x,.05)),
                                ("p95",np.quantile(x,.95)),("mean",np.mean(x)),("min",np.min(x)),("max",np.max(x))):
                row[metric+"_"+label]=float(value)
        result.append(row)
    with path.open("w",newline="") as f:
        writer=csv.DictWriter(f,fieldnames=list(result[0]));writer.writeheader();writer.writerows(result)
summarize(groups,["group","variant","n","iterations","mode"],
          ["wall_ms","h2d_ms","kernel_ms","d2h_ms","allocations"],folder/"runtime_summary.csv")
summarize(cg,["group","functions","mode"],["wall_ms","peak_rss_kib","output_bytes"],folder/"compile_summary.csv")
figures=folder/"figures";figures.mkdir(exist_ok=True)
plt.rcParams.update({"font.family":"DejaVu Sans","font.size":10,"axes.titlesize":12,
 "axes.spines.top":False,"axes.spines.right":False,"axes.grid":True,"grid.alpha":.18,
 "figure.facecolor":"white","savefig.facecolor":"white","legend.frameon":False})
colors={"cpu_mlir":"#2364aa","cpp_checked":"#379683","cpp_unchecked":"#88b04b",
 "gpu_event":"#e77728","gpu_resident":"#b16c20","gpu_scope_manual":"#b33f62",
 "gpu_scope_directive":"#7e57c2","cpu_sim_scope":"#888888"}
def save(fig,name):
    fig.savefig(figures/(name+".png"),dpi=180,bbox_inches="tight")
    fig.savefig(figures/(name+".svg"),bbox_inches="tight")
    plt.close(fig)
def line(ax,x,series,label,color,metric="wall_ms",scale=1):
    med=np.array([median(r,metric)*scale for r in series])
    hi=np.array([quantile(r,metric)*scale for r in series])
    ax.plot(x,med,"o-",label=label,color=color,markersize=4)
    ax.fill_between(x,med,hi,color=color,alpha=.12)
    return med
# Compilation end-to-end, fresh processes with warmed filesystem.
fig,axes=plt.subplots(1,3,figsize=(13,3.8),layout="constrained")
for mode,color in (("llvm","#2364aa"),("mlir","#e77728")):
    series=[cg[("compiler",n,mode)] for n in (10,100,500)]
    for ax,metric,scale,label in zip(axes,["wall_ms","peak_rss_kib","output_bytes"],[1,1/1024,1/1024],
                                    ["Compile wall time (ms)","Peak child RSS (MiB)","Object size (KiB)"]):
        line(ax,[10,100,500],series,mode.upper()+" backend",color,metric,scale)
        ax.set(xscale="log",xlabel="Function count",ylabel=label)
axes[0].legend();fig.suptitle("Compilation: same scalar sources, LLVM 23, -O2 | median to P95")
save(fig,"01_compilation")
# Scalar calls; black-box functions include call/loop/checksum cost.
fig,ax=plt.subplots(figsize=(9,4),layout="constrained")
names=["scalar","integer","predicate","loops"];x=np.arange(4)
for offset,mode,color in ((-.18,"llvm","#2364aa"),(.18,"mlir","#e77728")):
    series=[next(v for k,v in groups.items() if k[0]=="scalar" and k[4]==name+"_"+mode) for name in names]
    med=np.array([median(r)*1e6 for r in series]);hi=np.array([quantile(r)*1e6 for r in series])
    ax.bar(x+offset,med,.34,color=color,label=mode.upper(),yerr=[np.zeros(4),hi-med],capsize=3)
ax.set(xticks=x,xticklabels=["FP64 arithmetic","i32 arithmetic","bool predicate","counted-loop source"],
       ylabel="Amortized ns/call",title="Scalar execution | median + P95, batched black-box calls")
ax.legend();save(fig,"02_scalar")
lengths=sorted({k[2] for k in groups if k[0]=="length" and k[2]>0})
specs=[("cpu_mlir","CPU MLIR","wall_ms"),("cpp_checked","C++ checked","wall_ms"),
       ("cpp_unchecked","C++ unchecked","wall_ms"),("gpu_event","GPU event interval","kernel_ms"),
       ("gpu_resident","GPU resident host call","wall_ms"),("gpu_scope_manual","GPU full scope","wall_ms")]
fig,axes=plt.subplots(1,2,figsize=(13,4.8),layout="constrained")
for mode,label,metric in specs:
    actual="gpu_resident" if mode=="gpu_event" else mode
    series=[rows("length",actual,n) for n in lengths]
    med=line(axes[0],lengths,series,label,colors[mode],metric)
    if mode in ("cpu_mlir","cpp_checked","cpp_unchecked","gpu_event"):
        bandwidth=np.array(lengths)*24/(med*1e6)
        axes[1].plot(lengths,bandwidth,"o-",label=label,color=colors[mode],markersize=4)
axes[0].set(xscale="log",yscale="log",xlabel="Elements (FP64)",ylabel="Milliseconds",
            title="Length sweep | median to P95")
axes[1].set(xscale="log",yscale="log",xlabel="Elements (FP64)",ylabel="Effective GB/s = 24N / time",
            title="Logical traffic rate; not physical DRAM bandwidth")
axes[0].legend(fontsize=8);axes[1].legend(fontsize=8)
save(fig,"03_vector_length")
# Stage breakdown uses means of the SAME samples, preserving additivity.
fig,axes=plt.subplots(1,2,figsize=(13,4.5),layout="constrained")
large=[n for n in lengths if n>=65536];bottom=np.zeros(len(large));x=np.arange(len(large))
for metric,label,color in [("h2d_ms","H2D","#379683"),("kernel_ms","Event interval","#e77728"),
                          ("d2h_ms","D2H","#2364aa"),("other","Allocation / API / sync / free","#b6bac5")]:
    data=[]
    for n in large:
        r=rows("length","gpu_scope_manual",n)
        value=float(np.mean(values(r,metric))) if metric!="other" else float(np.mean(
            values(r,"wall_ms")-values(r,"h2d_ms")-values(r,"kernel_ms")-values(r,"d2h_ms")))
        if value<0:raise ValueError("Invalid negative residual")
        data.append(value)
    axes[0].bar(x,data,bottom=bottom,label=label,color=color);bottom+=data
axes[0].set(xticks=x,xticklabels=[f"{n:,}" for n in large],ylabel="Mean milliseconds",
            title="GPU scope breakdown | additive sample means")
axes[0].tick_params(axis="x",rotation=20);axes[0].legend(fontsize=8)
cpu=np.array([stat("length","cpu_mlir",n) for n in lengths])
for mode,metric,label in [("gpu_resident","kernel_ms","Event-only ratio"),
                         ("gpu_resident","wall_ms","Resident host-call ratio"),
                         ("gpu_scope_manual","wall_ms","Full-scope ratio")]:
    gpu=np.array([stat("length",mode,n,metric) for n in lengths])
    axes[1].plot(lengths,cpu/gpu,"o-",label=label)
axes[1].axhline(1,color="black",ls="--",lw=1)
axes[1].set(xscale="log",yscale="log",xlabel="Elements",ylabel="CPU MLIR median / GPU median",
            title="Ratios use different GPU timing boundaries")
axes[1].legend(fontsize=8);save(fig,"04_gpu_costs")
fig,axes=plt.subplots(1,2,figsize=(11,4.2),layout="constrained")
for ax,n in zip(axes,[65536,1048576]):
    for mode,color,label in [("fresh","#b33f62","Copy/allocate every iteration"),
                             ("resident","#2364aa","Copy/allocate once")]:
        line(ax,[1,10,100],[rows("residency",mode,n,k=k) for k in [1,10,100]],label,color)
    ax.set(xscale="log",yscale="log",xlabel="Repeated launches K",ylabel="Total milliseconds",
           title=f"Data residency | N={n:,}")
axes[0].legend(fontsize=8);save(fig,"05_residency")
fig,axes=plt.subplots(1,2,figsize=(12,4.4),layout="constrained")
for ax,n in zip(axes,[65536,1048576]):
    for mode,label,metric in [("cpu_mlir","CPU MLIR","wall_ms"),("cpp_checked","C++ checked","wall_ms"),
                              ("gpu_event","GPU event interval","kernel_ms"),("gpu_scope_manual","GPU full scope","wall_ms")]:
        actual="gpu_resident" if mode=="gpu_event" else mode
        line(ax,[1,8,32,128],[rows("intensity",actual,n,variant=v) for v in [1,8,32,128]],
             label,colors[mode],metric)
    ax.set(xscale="log",yscale="log",xlabel="Dependent multiply/add pairs per element",
           ylabel="Milliseconds",title=f"FP64 computation sweep | N={n:,}")
axes[0].legend(fontsize=8);save(fig,"06_intensity")
fig,axes=plt.subplots(1,2,figsize=(12,4.3),layout="constrained")
ratio_rows=[];med=[];lo=[];hi=[]
for n in lengths:
    manual={int(r["sample"]):float(r["wall_ms"]) for r in rows("length","gpu_scope_manual",n)}
    directive={int(r["sample"]):float(r["wall_ms"]) for r in rows("length","gpu_scope_directive",n)}
    ratios=np.array([100*(directive[i]/manual[i]-1) for i in sorted(manual)])
    med.append(np.median(ratios));lo.append(np.quantile(ratios,.05));hi.append(np.quantile(ratios,.95))
    ratio_rows.append(dict(n=n,median_percent=med[-1],p05_percent=lo[-1],p95_percent=hi[-1]))
axes[0].plot(lengths,med,"o-",color="#7e57c2")
axes[0].fill_between(lengths,lo,hi,color="#7e57c2",alpha=.15)
axes[0].axhline(0,color="black",ls="--",lw=1)
axes[0].set(xscale="log",xlabel="Elements",ylabel="Paired (directive / manual - 1), %",
            title="Directive cost | median, P05-P95 band")
for mode,label in [("gpu_scope_manual","Manual scope"),("gpu_scope_directive","Generated directive")]:
    line(axes[1],lengths,[rows("length",mode,n) for n in lengths],label,colors[mode])
axes[1].set(xscale="log",yscale="log",xlabel="Elements",ylabel="Milliseconds",title="Same transfers and synchronous execution")
axes[1].legend(fontsize=8);save(fig,"07_directives")
with (folder/"directive_ratios.csv").open("w",newline="") as f:
    w=csv.DictWriter(f,fieldnames=list(ratio_rows[0]));w.writeheader();w.writerows(ratio_rows)
fig,axes=plt.subplots(1,2,figsize=(12,4.2),layout="constrained")
stages=["kale","arith","llvm_dialect","llvm_ir","object","ptx"]
for ax,metric,scale,label in [(axes[0],"wall_ms",1,"Wall time (ms)"),(axes[1],"peak_rss_kib",1/1024,"Peak child RSS (MiB)")]:
    med=np.array([median(cg[("stage",2,s)],metric)*scale for s in stages])
    hi=np.array([quantile(cg[("stage",2,s)],metric)*scale for s in stages])
    ax.bar(np.arange(6),med,color=["#379683","#379683","#2364aa","#2364aa","#e77728","#b33f62"],
           yerr=[np.zeros(6),hi-med],capsize=3)
    ax.set(xticks=np.arange(6),xticklabels=["kale","arith","LLVM dialect","LLVM IR","Object","PTX"],ylabel=label)
    ax.tick_params(axis="x",rotation=20)
fig.suptitle("Separate process measurements to each output endpoint; NOT additive pass costs")
save(fig,"08_stages")
# Data-driven bilingual report fragments.
n=1048576
cpu=stat("length","cpu_mlir",n);kernel=stat("length","gpu_resident",n,"kernel_ms")
resident=stat("length","gpu_resident",n);scope=stat("length","gpu_scope_manual",n)
fresh=stat("residency","fresh",n,k=100);reuse=stat("residency","resident",n,k=100)
old=median(cg[("compiler",500,"llvm")]);new=median(cg[("compiler",500,"mlir")])
params=metadata["parameters"]
import re
run_date=metadata["timestamp_utc"][:10]
cpu_name=next((line.split(":",1)[1].strip() for line in metadata["cpu"].splitlines() if line.startswith("Model name:")), "CPU: see environment.json")
gpu_name=metadata["gpu_before"].split(",")[0]
cuda_version=re.search(r"release ([0-9.]+)",metadata["cuda"])
cuda_version=cuda_version.group(1) if cuda_version else "see environment.json"
environment_label=f"{run_date} · {cpu_name} / {gpu_name} · LLVM {metadata['llvm']} · CUDA {cuda_version}"
heavy_cpu=stat("intensity","cpu_mlir",1048576,variant=128)
heavy_gpu=stat("intensity","gpu_scope_manual",1048576,variant=128)
relative=folder.relative_to(ROOT).as_posix()
caption=[
("01_compilation","Compilation time, memory and object size","编译耗时、内存与目标文件大小"),
("02_scalar","Scalar execution","标量执行"),
("03_vector_length","Vector length and effective traffic rate","数组长度与有效数据吞吐"),
("04_gpu_costs","GPU cost breakdown and timing boundaries","GPU 阶段占比与计时边界"),
("05_residency","Device data residency","设备数据驻留"),
("06_intensity","Increasing FP64 computation","增加 FP64 计算量"),
("07_directives","Teaching-directive overhead","教学指令开销"),
("08_stages","Compiler output endpoints","编译输出阶段")]
def report(zh):
    if zh:
        text=f"""## Benchmark：本地实测

**{environment_label}。**
本次完成编译、标量、向量、GPU 内核/端到端、数据驻留、计算量及教学指令开销测试。
保留 {len(raw)} 条运行时观测（含首次使用记录）和 {len(comp)} 条编译样本；不删除异常慢样本。

| 代表性结果（中位数） | 实测 |
| --- | ---: |
| 500 个函数，原 LLVM 后端编译 | {old:.3f} ms |
| 500 个函数，MLIR 后端编译 | {new:.3f} ms（{new/old:.2f}×） |
| 1,048,576 元素，CPU MLIR 直接执行 | {cpu:.3f} ms |
| 同规模，GPU 驻留数据的 event 区间 | {kernel:.3f} ms |
| 同规模，GPU 驻留数据的同步宿主调用 | {resident:.3f} ms |
| 同规模，GPU 完整数据区域 | {scope:.3f} ms |
| 同规模，每元素 128 对乘加：CPU / GPU 完整区域 | {heavy_cpu:.3f} / {heavy_gpu:.3f} ms（{heavy_cpu/heavy_gpu:.2f}×） |
| 同规模重复 100 次：每次迁移 / 驻留一次 | {fresh:.3f} / {reuse:.3f} ms（{fresh/reuse:.2f}×） |

**结论：本机这次测量中，GPU 的计算阶段更短，但简单 FP64 向量运算的完整迁移流程更慢。**
不能把 event-only 的比值当作端到端加速比。复用设备数据显著降低了重复分配和传输成本。
教学指令相对手写调用的差异存在噪声，图中保留配对样本的 P05–P95，不宣称固定百分比开销。

### 测量口径

- CPU/GPU 使用相同 FP64 输入及运算；向量每个样本后检查全部结果和主机尾部哨兵，检查不计时。标量计时前验证双后端一致性。
- 运行时每项预热 {params['warmup']} 次、测量 {params['samples']} 次；驻留实验预热 2 次、每项 {max(7,params['samples']//3)} 次；编译每项预热 1 次、测量 {params['compile_samples']} 次。
- CPU 直接执行使用批量计时；小于等于 257 元素批量 1000 次。标量报告包含调用、循环和校验和累加成本，不能解释为单条运算延迟。
- 单线程测量进程固定到 WSL 逻辑 CPU {metadata['benchmark_cpu']}；C++ 基线为 Clang 23 -O2、关闭 FP contraction，未用 -march=native 或 fast-math。
- C++ checked 保留数组长度检查；unchecked 是较弱安全语义的参考。CPU 模拟运行时含额外复制，不作为 GPU 加速比基线。
- CUDA 使用同步 API、pageable 主机内存、256 线程块。驻留 event 时间与同步宿主调用时间分别保存；进程首次使用单独记录且不混入稳定样本，不代表硬件/系统缓存冷启动。
- 图中通常是中位数，淡色带/误差线延伸至 P95；阶段分解使用同一组样本的均值以保证可加和。
- 有效带宽按逻辑流量 24N/time 计算；重复访问可能命中缓存，不能据此称为实际 DRAM 带宽。
- 计算量采用 1/8/32/128 对有依赖的乘加；保存的 PTX 分别保留相同数量的 mul/add，未融合为 FMA、未折叠掉。
- 各编译输出阶段都是独立进程的累计端点耗时，不能相加或直接相减推断单个 Pass 成本。
- 数据驻留实验中只有最后一次结果被消费，两方案可观察结果相同；它不代表每轮都必须回传结果的工作负载。
- 未锁定 CPU/GPU 频率；WSL、温度、显示负载和后台程序会影响结果。本报告不是通用硬件排名或 OpenACC V&V 性能结论。

"""
    else:
        text=f"""## Benchmarks: measured locally

**{environment_label}.**
The suite covers compilation, scalar/vector execution, GPU event and end-to-end timing,
data residency, FP64 computation density, and teaching-directive overhead.
It retains {len(raw)} runtime observations (including first-use records) and {len(comp)} compiler samples; no slow outliers are discarded.

| Representative result (median) | Measured |
| --- | ---: |
| 500 functions: original LLVM backend compilation | {old:.3f} ms |
| 500 functions: MLIR backend compilation | {new:.3f} ms ({new/old:.2f}×) |
| 1,048,576 elements: direct CPU MLIR execution | {cpu:.3f} ms |
| Same size: resident GPU event interval | {kernel:.3f} ms |
| Same size: resident GPU synchronous host call | {resident:.3f} ms |
| Same size: complete GPU data scope | {scope:.3f} ms |
| Same size, 128 multiply/add pairs: CPU / complete GPU scope | {heavy_cpu:.3f} / {heavy_gpu:.3f} ms ({heavy_cpu/heavy_gpu:.2f}×) |
| Same size, 100 launches: fresh scopes / one resident scope | {fresh:.3f} / {reuse:.3f} ms ({fresh/reuse:.2f}×) |

**In this local run, GPU computation is shorter, but full transfers make the simple FP64 vector operation slower end to end.**
Event-only ratios are not application speedups. Device-data reuse substantially reduces repeated allocation/copy cost.
Directive-versus-manual differences are noisy: paired P05–P95 ranges are retained rather than claiming a fixed overhead percentage.

### Methodology

- Identical FP64 inputs and arithmetic; vector samples verify all outputs and a host-side tail guard outside timing. Scalar backend parity is checked before timing.
- Runtime configurations: {params['warmup']} warmups and {params['samples']} samples. Residency: 2 warmups and {max(7,params['samples']//3)} samples. Compilation: 1 warmup and {params['compile_samples']} samples.
- CPU direct calls are batched (1000 calls for N ≤ 257). Scalar ns/call includes call, loop and checksum accumulation; it is not instruction latency.
- Single-threaded measurement processes are pinned to WSL logical CPU {metadata['benchmark_cpu']}. C++ baselines use Clang 23 -O2 with FP contraction disabled, without -march=native or fast-math.
- C++ checked retains array length checks; unchecked has weaker safety semantics. The CPU simulation includes extra copies and is not the GPU speedup baseline.
- CUDA uses synchronous APIs, pageable host memory and 256-thread blocks. Resident event intervals and synchronous host calls are recorded separately; first-use observations are excluded from steady-state summaries.
- Most plots show medians with bands/error bars up to P95. Cost decomposition uses means of the same samples to preserve additivity.
- Effective bandwidth uses logical traffic 24N/time. Repeated data can be cached; this is not a measurement of physical DRAM bandwidth.
- Computation density is 1/8/32/128 dependent multiply/add pairs; saved PTX retains the corresponding mul/add counts with no FMA or arithmetic elimination.
- Compiler stages are cumulative output endpoints measured in separate processes, not additive individual-pass costs.
- Residency consumes only the final result in both variants; it does not model an application that needs every intermediate result on the host.
- Frequencies are not locked. WSL scheduling, temperature, display activity and background load affect results. These are not universal hardware rankings or OpenACC V&V results.

"""
    for name,en,cn in caption:
        text+=f"### {cn if zh else en}\n\n![{cn if zh else en}]({relative}/figures/{name}.png)\n\n"
    if zh:
        text+=f"""### 原始记录与复现

[运行时原始 CSV]({relative}/runtime.csv) · [编译原始 CSV]({relative}/compile.csv) ·
[运行时汇总]({relative}/runtime_summary.csv) · [编译汇总]({relative}/compile_summary.csv) ·
[环境、命令与源码哈希]({relative}/environment.json) · [生成源码]({relative}/sources/) · [LLVM IR/PTX]({relative}/ir/)

驻留实验调用底层运行时；当前教学指令每次仍重新分配和传输，不自动驻留。
图表同时提供 PNG 和 SVG。以下命令要求已配置好的 MLIR/CUDA 构建及 Python matplotlib/numpy；
--out 必须是新的目录，以免覆盖原始记录。

"""
    else:
        text+=f"""### Raw records and reproduction

[Runtime samples]({relative}/runtime.csv) · [Compiler samples]({relative}/compile.csv) ·
[Runtime summary]({relative}/runtime_summary.csv) · [Compiler summary]({relative}/compile_summary.csv) ·
[Environment, commands and source hashes]({relative}/environment.json) · [Generated sources]({relative}/sources/) · [LLVM IR/PTX]({relative}/ir/)

Residency uses the lower-level runtime; current teaching directives still allocate and transfer on every call.
Charts are saved as PNG and SVG. The commands require an already configured MLIR/CUDA build
and Python matplotlib/numpy. Choose a fresh --out directory to preserve existing records.

"""
    if (folder/"logs/regression-tests.log").exists():
        if zh:
            text+=f"[测量日志]({relative}/logs/benchmark-run.log) · [回归测试日志]({relative}/logs/regression-tests.log)\n\n"
        else:
            text+=f"[Measurement log]({relative}/logs/benchmark-run.log) · [Regression test log]({relative}/logs/regression-tests.log)\n\n"
    text+="```bash\npython3 benchmarks/run.py --out benchmarks/results/my-run\npython3 benchmarks/plot.py benchmarks/results/my-run --update-readme\n```\n"
    return text
en=report(False);zh=report(True)
# Standalone reports use paths relative to their own result folder.
(folder/"REPORT.md").write_text(en.replace(relative+"/",""))
(folder/"REPORT.zh-CN.md").write_text(zh.replace(relative+"/",""))
if args.update_readme:
    for name,section in (("README.md",en),("README.zh-CN.md",zh)):
        path=ROOT/name
        if not path.exists():raise SystemExit(f"Missing {name}; create the language version first")
        text=path.read_text();start="<!-- BENCHMARKS:START -->";end="<!-- BENCHMARKS:END -->"
        block=start+"\n"+section+"\n"+end
        if start in text:
            a=text.index(start);b=text.index(end,a)+len(end);text=text[:a]+block+text[b:]
        else:text+="\n"+block+"\n"
        path.write_text(text)
print(f"Rendered 8 PNG + 8 SVG figures; {len(raw)} runtime / {len(comp)} compiler observations")
print(f"N=1048576 CPU={cpu:.6f}ms event={kernel:.6f}ms resident={resident:.6f}ms scope={scope:.6f}ms")
