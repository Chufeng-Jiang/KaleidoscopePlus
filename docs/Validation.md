# 本地验收记录

更新：OpenACC 风格教学子集加入后，完整 MLIR/CUDA 回归为 **20/20**，
LLVM 21 独立回归为 **14/14**；三项 GPU 测试均实际通过。
见 [教学子集验收](OpenACCTeaching.md)。下方 17 项及计时是此前执行/GPU 阶段的记录。

日期：2026-11-14。环境：WSL Ubuntu 22.04，LLVM/MLIR 23.1.6，
CUDA Toolkit 12.8，RTX 3080 Ti Laptop GPU（sm_86）。

## 构建与测试

- MLIR + CUDA 构建：17/17 测试通过；vector_cuda 和 device_runtime_cuda 均实际运行并 Passed，没有跳过。
- LLVM 21、MLIR/CUDA 关闭：14/14 测试通过，包含原有调试器集成测试和 CPU 运行时测试。
- LLVM 23 配置没有对应 LLDB，因此其 17 项不包含三项 LLDB 集成测试；这些在 LLVM 21 配置中已通过。
- 验证了文档中的 kale-opt 完整 lowering pipeline。
- git diff --check 通过。
- 未安装新工具链或驱动，使用本机已有 LLVM/MLIR 和 CUDA。

具体覆盖见 [执行与 GPU 指南](MLIRExecutionAndGPU.md)。
构建目录保存 final-tests.log、vector-cpu.log、vector-cuda.log；
LLVM-only 测试日志为 build/stage3-tests.log。

## 单次计时观测

单位均为毫秒。CPU 行是带独立缓冲区和复制的模拟运行时，
不是整个 CPU 数组函数的直接调用时间。
CUDA kernel 时间使用 events；传输与总时间使用主机单调时钟。
所有行均与原生 MLIR 数组函数和独立数值参考比较并通过。

| 后端 | 元素数 | H2D | Kernel | D2H | Total | 结果 |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| cpu | 0 | 0.000086 | 0.000042 | 0.000041 | 0.002996 | PASS |
| cpu | 1 | 0.000107 | 0.000055 | 0.000074 | 0.001411 | PASS |
| cpu | 17 | 0.000100 | 0.000093 | 0.000063 | 0.000974 | PASS |
| cpu | 257 | 0.000147 | 0.000441 | 0.000098 | 0.002602 | PASS |
| cpu | 100003 | 0.113929 | 0.127843 | 0.059203 | 1.322597 | PASS |
| cuda | 0 | 0.000058 | 0.000000 | 0.000038 | 0.737897 | PASS |
| cuda | 1 | 1.071588 | 1.006592 | 0.096783 | 3.930220 | PASS |
| cuda | 17 | 0.310369 | 0.006144 | 0.053063 | 0.863737 | PASS |
| cuda | 257 | 0.235314 | 0.006144 | 0.039546 | 0.688667 | PASS |
| cuda | 100003 | 0.340961 | 0.065184 | 0.208546 | 1.645954 | PASS |

Total 包含分配、映射管理、复制、启动/同步和释放；
不含 session/PTX 初始化、参考计算和结果验证。第一轮实际启动仍有驱动延迟加载成本。
零长度 CUDA 行不分配设备缓冲区、不启动 kernel。

这是一组正确性测试的单次运行记录，不是稳定性能基准。
本次 100003 元素时 CUDA kernel 比 CPU 模拟内核短，但 CUDA 总时间更长；
不能据此声称端到端加速。后续性能实验应增加预热、重复次数、
设备数据复用和统计分布，并固定精度与问题规模。

## 复现

在项目根目录执行：

```bash
ctest --test-dir build/mlir --output-on-failure
build/mlir/src/runtime/kale-vector-demo cpu
build/mlir/src/runtime/kale-vector-demo cuda build/mlir/src/runtime/vector.ptx
```

GPU 是显式 element-map 包装，算术来自 examples/Vector.kale。
这份记录不代表 OpenACC、Flang 或 V&V 支持。

## 稳定性能测量

上述单次 demo 计时保留为历史正确性记录。2026-11-14 已另行完成带预热、重复采样、
原始 CSV 和图表的 [完整 Benchmark 报告](../benchmarks/results/2026-11-14/REPORT.zh-CN.md)。
性能分析请使用该报告；两种测试的计时边界与样本口径不同。
