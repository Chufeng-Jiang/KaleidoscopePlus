# Benchmark harness / 性能测试

[English project README](../README.md) · [中文项目 README](../README.zh-CN.md)

This Linux/WSL harness measures the implemented compiler and synchronous CUDA paths.
It does not implement additional language features, compare full OpenACC compilers,
or run OpenACC V&V. All recorded GPU samples execute the generated PTX on a real GPU.
The current PTX target is sm_86.

本套件在 Linux/WSL 中测量已有的编译器与同步 CUDA 路径，不是完整 OpenACC 编译器横评，
也不是 OpenACC V&V。GPU 样本均执行实际生成的 PTX；当前目标为 sm_86。

## Reproduce / 复现

First configure and build MLIR/CUDA as described in the project README.
The suite needs a working CUDA device, Clang 23, GNU time, taskset, Python 3,
and matplotlib/numpy for plotting. Benchmark binaries are optional:
KALEIDOSCOPE_BUILD_BENCHMARKS defaults to OFF; run.py enables it in the chosen build.

先按项目 README 配置 MLIR/CUDA。需要可用的 CUDA 设备、Clang 23、GNU time、taskset、
Python 3；绘图另需 matplotlib/numpy。可选 benchmark 构建默认关闭，
run.py 会在指定的构建目录中开启该选项。

```bash
python3 benchmarks/run.py --out benchmarks/results/my-run
python3 benchmarks/plot.py benchmarks/results/my-run --update-readme
```

On the measured machine, plotting used:
本机绘图使用已有的 Python 环境：

```bash
/home/chufeng/SoftwareInstalled/anaconda3/bin/python benchmarks/plot.py benchmarks/results/2026-11-14 --update-readme
```

--out must not exist. A failed run leaves complete=false in environment.json;
plot.py refuses incomplete runs. The scripts preserve raw measurements;
plot.py regenerates derived CSVs, reports and figures only.
--update-readme updates marked benchmark sections in both README languages,
preserving the rest of each document. The standard full suite is expected;
arbitrary partial CSV datasets are not supported.

--out 必须不存在；失败时 environment.json 的 complete 保持 false，绘图拒绝不完整记录。
原始样本不被覆盖；绘图仅重新生成统计、报告和图片。
--update-readme 只替换两种 README 的 Benchmark 标记区块，保留其余内容。
绘图脚本要求完整套件数据，不支持任意截取的 CSV。

## Coverage / 覆盖范围

| Experiment / 实验 | Scope / 范围 |
| --- | --- |
| Compiler / 编译器 | Same 10/100/500-function sources, original LLVM vs MLIR, wall time, peak child RSS, object size / 同源码双后端耗时、内存、产物大小 |
| Scalar / 标量 | FP64, i32, bool, counted-loop source, both backends / 两后端执行对比 |
| Vector length / 数组长度 | 0, 1, 17, 257, 4096, 65536, 262144, 1048576, 4194304 |
| Execution modes / 执行模式 | MLIR CPU, C++ checked/unchecked, CPU simulation, CUDA resident/manual scope/generated directive |
| FP64 computation / 计算量 | 1, 8, 32, 128 dependent multiply/add pairs; N=65536 and 1048576 |
| Residency / 数据驻留 | 1, 10, 100 launches; N=65536 and 1048576; transfer every time vs once |
| Compiler endpoints / 编译端点 | kale, arith, LLVM dialect, LLVM IR, object, PTX |
| First use / 首次使用 | Context creation, module loading, first manual scope; separately recorded, not a hardware cold-start benchmark |

The scalar loop source can be optimized algebraically. Its ns/call measures compiled
function-call throughput including the harness loop/checksum, not per-iteration latency.
Vector outputs are checked after each sample against an independent numeric reference;
scalar backends undergo a parity check before timing, and per-sample checksums are saved.
Array tail guards are host-side checks, not a GPU memory sanitizer.

标量循环源码可能被代数优化，其 ns/call 包含调用、驱动循环及累加，并非循环单步延迟。
向量每个样本后做独立参考值检查；标量在计时前做双后端一致性检查，逐样本保存校验和。
数组尾部哨兵位于主机侧，不能替代 GPU 内存检查工具。

No reduction, matrix multiplication, async overlap, multi-GPU, Flang, or standard
OpenACC comparison is claimed: those paths are not implemented here.
JIT startup and DWARF-debug performance are also outside this native/GPU suite.

本次没有声称覆盖尚未实现的归约、矩阵乘、异步重叠、多 GPU、Flang 或标准 OpenACC 横评。
JIT 启动与 DWARF 调试性能也不属于本次原生/GPU 套件范围。

## Timing and records / 计时与记录

- runtime.csv: group, variant (multiply/add pairs), n, iterations (launches), mode,
  sample, batch, wall_ms, h2d_ms, kernel_ms, d2h_ms, allocations, checksum.
  Timing and allocation fields are normalized per batch call; residency batch=1
  reports the entire K-launch workflow.
- compile.csv: experiment group, function count, backend/endpoint, sample,
  wall_ms, peak_rss_kib, output_bytes. Wall time includes fresh-process launch;
  filesystem caches are warmed. Peak RSS is GNU time's child-process peak.
- environment.json: versions, affinity, GPU snapshots, command arrays, Git state,
  source SHA256, parameters and completion status. Git state can be dirty;
  hashes record the sources that existed when the measurement began.
- sources/ and ir/: generated workloads and saved LLVM IR/PTX/C++ assembly.
- *_summary.csv: median, P05, P95, mean, minimum, maximum; all samples retained.
- figures/: eight PNGs and matching SVGs. REPORT.md / REPORT.zh-CN.md:
  standalone bilingual reports. Figures use English labels for portable fonts.

runtime.csv 的时间与分配字段已按 batch 归一化；驻留实验 batch=1，
wall_ms 为 K 次启动完整流程。compile.csv 的时间包含新进程启动，
peak_rss_kib 是 GNU time 记录的子进程峰值内存。
environment.json 记录版本、CPU 绑定、GPU 快照、命令、Git 状态和测量开始时的源码哈希。
结果目录保存生成源码与 IR，统计包含中位数、P05/P95、均值、最小/最大值。
8 张图均提供 PNG/SVG；图内使用英文标签，报告及 README 均有中英文版。

Resident event intervals exclude transfers; resident host wall time includes the
synchronous runtime call. Full-scope wall time includes allocation, transfers,
execution and release, but excludes context/module initialization and validation.
Transfer statistics use host wall time; kernel statistics use CUDA events.
Their subtraction from wall time is an unattributed residual, not an isolated
measurement of allocator cost. No frequency locks or background-load isolation
were imposed; interpret distributions and raw environmental snapshots together.

驻留 event 区间不含传输；驻留宿主墙钟时间包含同步运行时调用。
完整数据区域包含分配、传输、执行、释放，不含 context/模块初始化与结果验证。
传输统计采用主机墙钟，内核统计采用 CUDA event；与总时间的差额是未归因余项，
不是单独测出的分配器耗时。没有锁定频率或隔离后台负载，请结合分布与环境快照解释。

The data-residency experiment repeatedly computes from unchanged inputs and consumes
only the last result. Its speedup compares two GPU data-management strategies,
not GPU against CPU. The directive implementation itself still creates a fresh
data region on every call; resident measurements use the lower-level runtime API.

驻留实验从不变输入重复计算，只消费最后结果；加速比比较的是两种 GPU 数据管理方式，
不是 GPU 对 CPU。教学指令本身仍每次创建独立数据区域；驻留实验使用底层运行时接口。
