# MLIR 执行、数组与同步 GPU 运行时

## 已实现的路径与边界

本项目现在有两条可执行编译路径：

```text
原有：Kaleidoscope AST → LLVM IR → 优化 → JIT / 目标文件

新增：Kaleidoscope AST → kale + arith + func + scf + memref + cf
                     → 消除 kale
                     → scf 转 cf
                     → LLVM dialect（仍是 MLIR）
                     → LLVM IR translation
                     → LLVM 优化 → 原生目标文件 / PTX
```

GPU 示例是**显式的一维逐元素映射**，不是任意循环自动并行化：
`out[i] = element(x[i], y[i], a)`。element 的实际计算来自 .kale 源码，
不是运行时中硬编码的 SAXPY，也不是另外手写的 CUDA C++ 内核。
CPU 向量函数和 GPU 包装内核复用同一个 element 定义。

这是一套教学编译器/运行时，不实现 Flang、FIR/HLFIR、acc dialect、
OpenACC ABI、OpenACC 数据环境规范或 V&V 兼容性。

已验证环境：WSL Ubuntu 22.04、LLVM/MLIR 23.1.6、CUDA Toolkit 12.8、
NVIDIA GeForce RTX 3080 Ti Laptop GPU（compute capability 8.6）。
原 LLVM-only 构建仍使用 LLVM 21。LLVM/MLIR 必须为相同版本。

## 一次构建并运行全部验收

在项目根目录的 WSL shell 中：

```bash
cmake -S . -B build/mlir \
  -DLLVM_DIR=/usr/lib/llvm-23/lib/cmake/llvm \
  -DMLIR_DIR=/usr/lib/llvm-23/lib/cmake/mlir \
  -DKALEIDOSCOPE_ENABLE_MLIR=ON \
  -DKALEIDOSCOPE_ENABLE_CUDA=ON \
  -DCUDAToolkit_ROOT=/usr/local/cuda-12.8 \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/mlir -j4
ctest --test-dir build/mlir --output-on-failure

build/mlir/src/runtime/kale-vector-demo cpu
build/mlir/src/runtime/kale-vector-demo cuda build/mlir/src/runtime/vector.ptx
```

构建自动从 examples/Vector.kale 生成 vector.o 与 vector.ptx，并链接 CPU demo。
运行时通过 CUDA Driver API 加载 PTX，在设备上 JIT 编译并启动内核；
本实现不用 nvcc 编译内核，但 CUDA 开发头文件和 Driver API 链接目标仍是构建依赖。

无 CUDA 时，设置 KALEIDOSCOPE_ENABLE_CUDA=OFF，CPU 模拟和原生执行仍可使用。
CUDA 测试在初始化无法获得设备时以 77 返回并显示 Skipped，绝不算作 GPU 通过。
本地验收应确认 vector_cuda 和 device_runtime_cuda 实际为 Passed。

## 分阶段观察 IR

```bash
# 高层混合 dialect IR
build/mlir/src/backend/mlir/kale-mlir examples/Vector.kale

# 消除 kale，保留 scf/memref/func 等操作
build/mlir/src/backend/mlir/kale-mlir --lower-to-arith examples/Vector.kale

# LLVM dialect：输出 llvm.func、llvm.load 等 MLIR 操作
build/mlir/src/backend/mlir/kale-mlir --emit-llvm-dialect examples/Vector.kale

# 真正的 LLVM IR：输出 define、getelementptr 等
build/mlir/src/backend/mlir/kale-mlir --emit-llvm examples/Vector.kale -o build/mlir/vector.ll

# 原生目标文件
build/mlir/src/backend/mlir/kale-mlir --emit-object examples/Vector.kale -o build/mlir/vector.o

# GPU PTX，使用源码中 element 函数作为映射函数
build/mlir/src/backend/mlir/kale-mlir --emit-ptx --gpu-map element \
  examples/Vector.kale -o build/mlir/vector.ptx

# 在标准 MLIR Pass 驱动器中观察完整转换
build/mlir/src/backend/mlir/kale-mlir examples/Vector.kale |
  build/mlir/src/backend/mlir/kale-opt \
  --pass-pipeline='builtin.module(kale-lower-to-llvm)' \
  --mlir-print-ir-after-all
```

--input-mlir 可直接读取 .mlir。--translate-only --input-mlir --emit-llvm
只调用 translation，用来区分“无法合法化”与“缺少 LLVM translation 接口”的失败。
不能把任意 arith/func IR 直接交给 LLVM translator。

对象输出必须指定 -o。所有文件输出先完整生成，再写临时文件并原子替换；
错误不覆盖已有结果，输出不能与输入为同一路径、符号链接或硬链接。
默认文本输出写 stdout；诊断写 stderr。

## 数组语言与 ABI

```text
def element(x y a) a*x+y;
def vector(x: array y: array out: array a)
  if 0i32 < size(x) then
    for i: i32 = 0i32, i < size(x)-1i32 in
      store(out, i, element(load(x,i),load(y,i),a))
  else 0;
```

array 是借用的一维连续 double 数组参数，映射为 memref<?xf64>：

| 操作 | 类型与行为 |
| --- | --- |
| load(array, i32) | 返回 double，访问前检查 0 ≤ index < size |
| store(array, i32, double) | 写入元素，返回被写入的 double |
| size(array) | 返回 i32 长度 |

当前只允许数组参数，不允许数组返回、局部数组分配、数组赋值或全局数组。
load/store/size 在 MLIR 前端模式下是保留的内建函数名。
原有 LLVM-only 前端模式仍不支持数组参数，遇到数组会给出语义诊断。

C/C++ 调用数组函数应使用生成的 _mlir_ciface_ 包装器。
include/kaleidoscope/runtime/ArrayABI.h 定义与 ranked memref 对应的描述符：

```cpp
KaleArray array = {data, data, 0, {length}, {1}};
extern "C" double _mlir_ciface_vector(
    KaleArray *x, KaleArray *y, KaleArray *out, double a);
```

这是明确的 64 位 ABI：
allocated pointer、aligned pointer、i64 offset、i64 sizes[1]、i64 strides[1]。
调用者保证描述符及底层存储有效、offset=0、stride=1、
0≤length≤INT32_MAX，并在调用结束前保持存储存活。编译器不取得所有权。
边界检查保护索引，不负责验证调用者伪造的指针或描述符。
越界经 cf.assert 降级后终止进程；测试在子进程验证 SIGABRT。
原生 ABI 与 GPU kernel ABI 不同，运行时不会把主机 memref 描述符直接传给设备。

## 循环语义：为什么用 scf.while

现有 for 是后测试循环，精确执行顺序是：

1. 计算初值并绑定循环变量；
2. 执行 body；
3. 计算 step（允许有副作用）；
4. 计算 end 条件表达式（也允许有副作用）；
5. 读取当前循环变量，加 step，写回；
6. 根据第 4 步的值决定是否继续。

因此循环体至少执行一次；不能直接替换成前测试的 scf.for。
这里把 body、step、end、increment 放入 scf.while 的 before Region，
由 scf.condition 决定是否进入 after Region，再通过 scf.yield 回到 before。
零长度向量由外部 if 保护。示例条件使用 i < size(x)-1i32，
因为条件读取的是自动递增之前的 i。

为保留变量赋值、遮蔽和条件副作用，本版用函数入口的 rank-0 memref.alloca
保存可变标量，跨迭代状态通过 load/store 传递；没有强行把可变变量改成纯 SSA
循环携带参数。后续可以单独练习 mem2reg 或 scf 的 iter_args。
所有标量临时存储都在函数入口分配，循环中反复初始化，不会每次迭代继续分配栈空间。

if 生成带结果的 scf.if，两个 Region 用 scf.yield 交出结果。
分支赋值通过共享存储生效，离开词法作用域恢复名称绑定。
extern、普通调用和递归已接入。全局变量和自定义运算符仍只在旧后端支持。

## 标量一致性与 LLVMTypeConverter

保留原语言的细节：

- i32 加减乘与负号使用回绕语义，无 nsw/nuw；
- 浮点 < 使用 unordered 比较，结果保持 double 0/1；
- 浮点条件使用 ordered-not-equal，NaN 为 false；
- double → i32 先处理 NaN 并夹紧到 i32 范围，再截断，匹配旧后端的饱和转换；
- bool 函数参数和返回值使用 zeroext，匹配现有 x86-64 C++ bool ABI；
- 浮点操作不增加 fast-math flags。

LowerToLLVM.cpp 共享一个 LLVMTypeConverter，收集上游的 arith、func、memref、
cf 转换规则后执行完整转换。memref 参数转成描述符，函数签名、调用与
Block arguments 一起转换。数组函数通过 llvm.emit_c_interface 生成 C 包装器。
这次没有手写标准 dialect 到 LLVM 的全部规则。

LLVM dialect → LLVM IR 需要注册 builtin 与 LLVM translation interfaces。
Export.cpp 随后设置 native 或 NVPTX target triple/data layout，
复用原有 optimizeModule，验证 IR，再生成目标文件或 PTX。

## GPU 入口与限制

--gpu-map 指定的函数必须是已定义的
(double x, double y, double a) → double。
编译器生成导出的 kale_map kernel，计算：

```text
i = blockIdx.x * blockDim.x + threadIdx.x
if i < n:
    out[i] = element(x[i], y[i], a)
```

线程块固定为 256，尾部检查避免非整齐长度越界。n=0 时运行时不启动 kernel。
目标固定 sm_86、PTX 7.5，适配本次 3080 Ti。GPU 版本不链接任意主机 extern；
优化后仍有活跃非 intrinsic 外部依赖时拒绝导出。
只有 kernel 及可达 helper 留在 PTX，源码中仅用于 CPU 的数组遍历函数被移除。

这是显式 map 包装，不是对 vector 函数做一般的循环依赖分析。
正确使用要求 element 没有跨元素共享状态；同一源码修改 element 后，
原生对象和 PTX 都必须重建。没有加入归约、设备 printf、动态数组分配、
多 GPU 或异步执行支持。

## 教学运行时的生命周期

头文件：include/kaleidoscope/runtime/DeviceRuntime.h；
实现：src/runtime/DeviceRuntime.cpp。

```text
create(cpu/cuda)
  → set_cpu_kernel / load_ptx
  → enter(x), enter(y), enter(out)
  → copyin(x), copyin(y)
  → launch → wait
  → copyout(out)
  → exit(x), exit(y), exit(out)
  → destroy
```

- 每个 session 管理一张主机地址到缓冲区的映射表；
- enter 相同地址和长度是幂等操作，不增加引用计数或重复分配；
- 同一地址改长度、不同地址的重叠范围均报错；
- 空范围也要求非空的主机身份指针，但不分配设备内存；
- copyin 后输入才被标记初始化；未初始化输出不能 copyout；
- launch 要求三个映射长度都等于 n*sizeof(double)，避免未写区域被误认为有效；
- 允许 out 与某个输入使用完全相同的映射，支持逐元素原地写回；
- 调用都是同步的；wait 显式表达同步契约；
- session 是线程限定的单设备对象，使用设备 0；CUDA 上通过 context push/pop 保留调用者上下文；
- exit 显式释放，destroy 也会清理尚未释放的映射；
- 接口返回状态码，krt_error 提供具体操作错误，C++ 异常不会跨出常规操作接口；
- CPU 后端使用独立字节缓冲区模拟设备内存，直接在原数组上计算不算模拟数据迁移。

没有嵌套数据区域引用计数、异步 stream/queue、数组切片或 OpenACC present 语义。

## 验收和计时解释

mlir_execution：
整数/浮点/bool 的双后端执行对照、NaN/Infinity/饱和边界、回绕、
递归与 extern、遮蔽、循环副作用、LLVM IR 重新汇编、
数组边界、错误类型、转换失败、translation 失败、输出保护、GPU 入口检查。

mlir_conversion / mlir_missing_pattern：
已注册操作的验证、SSA 替换、完整转换合法性以及缺失 pattern 必须失败。

device_runtime_cpu / device_runtime_cuda：
重复 enter、重叠范围、尺寸不匹配、未初始化输入、未设置内核、
无效 PTX 路径、原地写回、重复释放和析构清理。

vector_cpu / vector_cuda：
n=0、1、17、257、100003；比较 CPU 数组函数、运行时执行结果及独立数值参考，
检查末尾哨兵。GPU 测试真的加载 PTX、启动 CUDA kernel 并读回结果。

demo 每种长度输出 h2d_ms、kernel_ms、d2h_ms、total_ms：

- CUDA kernel_ms 用 CUDA events，CPU kernel_ms 用单调时钟；
- 传输时间是同步复制 API 的主机墙钟时间；
- total 包含映射/分配、传输、启动/同步、释放以及少量 API 管理开销；
- total 不包括 session 初始化、PTX 加载/JIT、参考计算和结果验证；
- 第一次 kernel 启动仍可能包含驱动延迟加载开销；
- 这是正确性示例的单次观测，不是稳定性能基准，不据此声称 GPU 加速比。

## 后续与岗位的关系

这次补齐了 MLIR 到执行、数组内存 ABI、GPU 线程映射、数据迁移与运行时错误处理。
下一步可以在真实 Flang 中选择一个 OpenACC 子句，跟踪其语义检查、
acc/FIR 表示与 lowering，再复用这些经验补一个小功能。
不需要先给本项目增加 Fortran 解析器。

参考：
[LLVM IR Target](https://mlir.llvm.org/docs/TargetLLVMIR/)、
[Dialect Conversion](https://mlir.llvm.org/docs/DialectConversion/)、
[CUDA Driver API](https://docs.nvidia.com/cuda/cuda-driver-api/index.html)。
