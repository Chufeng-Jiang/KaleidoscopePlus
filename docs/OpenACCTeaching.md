# OpenACC 风格教学子集

这是当前 Kaleidoscope 项目中的受限指令实现，**不是 OpenACC 标准实现**。
不使用 Flang、Fortran 语法或上游 MLIR acc dialect，不宣称通过 OpenACC V&V。
它把“解析指令 → 检查并行合法性 → 生成宿主调用 → 自动数据迁移 → GPU 执行”
串成可读、可测试的教学闭环。

## 可以直接运行

沿用 [MLIR/CUDA 构建配置](MLIRExecutionAndGPU.md)，在 WSL 项目根目录：

```bash
cmake -S . -B build/mlir \
  -DLLVM_DIR=/usr/lib/llvm-23/lib/cmake/llvm \
  -DMLIR_DIR=/usr/lib/llvm-23/lib/cmake/mlir \
  -DKALEIDOSCOPE_ENABLE_MLIR=ON -DKALEIDOSCOPE_ENABLE_CUDA=ON \
  -DCUDAToolkit_ROOT=/usr/local/cuda-12.8 -DCMAKE_BUILD_TYPE=Release
cmake --build build/mlir -j4
ctest --test-dir build/mlir -R '^acc_' --output-on-failure

build/mlir/src/runtime/kale-acc-demo cpu
build/mlir/src/runtime/kale-acc-demo cuda build/mlir/src/runtime/acc-vector.ptx
```

CUDA 测试实际加载 PTX 并启动设备内核，初始化没有设备时会显示 Skipped，
不会伪装为 GPU 通过。关闭 CUDA 选项仍可运行 CPU 后端。

## 指令与支持范围

examples/AccVector.kale：

```text
def element(x y a) a*x+y;

#acc parallel loop copyin(x,y) copyout(out)
def vector(x: array y: array out: array a)
  if 0i32 < size(x) then
    for i: i32 = 0i32, i < size(x)-1i32 in
      store(out,i,element(load(x,i),load(y,i),a))
  else 0;
```

- 标记必须精确为 #acc，位于普通函数定义之前；不是 # acc，也不是 #pragma acc。
- 只支持上述 parallel loop、copyin、copyout 的组合和子句顺序。
- copyin 对应前两个数组参数，copyout 对应第三个；第四个参数是 double 系数。
- 名称可以改变，但参数类型和角色必须符合这一契约。
- 每个模块只支持一个指令区域；不支持嵌套区域。
- 循环必须是零起点、单位步长的 i32 循环，并具有示例中的外层零长度保护和结束条件。
- body 只能向 out[i] 写一次，读取 x[i]、y[i]，调用已定义的 element。
- element 必须只含 double 参数/字面量及 +、-、*；不允许调用、副作用、共享状态或跨元素访问。
- 不支持 async、wait 子句、reduction、gang/worker/vector、present、独立 data 区域、切片和多设备。
- 不认识的构造、子句或循环形状直接报错，不忽略指令，也不静默退回串行实现。

这是保守的“已知独立循环形状”检查，不是一般的循环依赖分析。
原有 for 为后测试循环，所以保留外层 if 与 i < size(x)-1i32；
不能把它误解成 Fortran DO 或 C 风格前测试循环。

## 编译与运行时自动化

```text
#acc 文本
  → Lexer 的 tok_acc
  → AccDirective.cpp：子句解析、参数绑定、循环形状和纯函数检查
  → MLIR module 上的 kale.acc_plan 字典属性
  → 原有 MLIR lowering 和 LLVM IR translation
  → Export.cpp 生成 kacc_vector 宿主入口
  → krt_acc_map 自动管理映射、复制、启动、同步与释放
```

这版使用显式的 **kale.acc_plan 元数据**，没有创建假的 acc.* 操作：
原有 CPU 循环仍通过 scf/memref 编译；导出的额外宿主入口调用教学运行时。
函数签名、element 和 copyin/copyout 信息可以直接在 MLIR 中检查。
输入手写 --input-mlir 时，调用者负责 IR 语义；宿主入口生成还会检查必要函数的类型。

```bash
build/mlir/src/backend/mlir/kale-mlir examples/AccVector.kale
build/mlir/src/backend/mlir/kale-mlir --emit-llvm examples/AccVector.kale
build/mlir/src/backend/mlir/kale-mlir --emit-object examples/AccVector.kale -o build/mlir/acc.o
build/mlir/src/backend/mlir/kale-mlir --emit-ptx examples/AccVector.kale -o build/mlir/acc.ptx
```

对于有指令的源码，PTX entry 从已验证的 element 推导，不必再写 --gpu-map。
如果显式 --gpu-map 与指令不一致，编译失败。
无指令的原 GPU map 模式仍可使用。

原有 LLVM-only kale 不支持 #acc，会报告语法错误；该标记不再被当作普通注释忽略。
其他注释（例如 #account）仍是注释。

## 宿主 ABI 与调用方式

编译器保留 _mlir_ciface_vector 作为串行参考，另生成：

```cpp
extern "C" int kacc_vector(
    KrtSession *session,
    const KaleArray *x, const KaleArray *y, KaleArray *out,
    double a);
```

宿主代码初始化一个 CPU 或 CUDA session。CUDA session 加载由**同一源码版本**
生成的 PTX，然后只调用 kacc_vector。CPU element 函数指针由生成代码自动提供。
调用者负责 session 的创建、PTX 部署与加载、最终销毁；
调用者无需手写 enter/copyin/launch/wait/copyout/exit。

PTX 模块与宿主对象是配套编译产物；当前没有二进制内容指纹校验，
不能混用来自不同源码版本的 PTX。CMake 对源码和编译器建立了重建依赖。

函数返回 KrtStatus；错误详情通过 krt_error(session) 读取。
原有源函数的成功结果是 0.0，新入口改为状态码，避免把运行错误藏在数值结果中。

每次调用自动执行：

```text
验证描述符和范围
→ enter(x/y/out)
→ copyin(x), copyin(y)
→ launch, wait
→ copyout(out)
→ exit(out/y/x)
```

- 要求连续一维 double 数组、offset=0、stride=1、长度相等且不超过 INT32_MAX。
- 三个底层数组范围必须互不重叠；参数名不同不代表运行时地址一定不同。
- 数组存储由调用者拥有；描述符和数据必须在调用期间有效。
- 不支持嵌套数据环境，参与本次调用的起始地址不能已经被 session 映射。
- 中途失败会清理本次取得的映射并保留首个错误信息，不释放调用者原先的其他映射。
- 空数组仍需有效、互不相同的身份指针；不分配设备存储、不启动 kernel。
- 所有操作同步；重复调用重新建立独立数据区域，不做跨调用的数据驻留优化。

## 测试与阅读顺序

acc_directives 覆盖解析、源码定位、IR 计划、宿主调用、自动 PTX 入口，
以及未知子句、错误数据参数、非单位步长、错误边界、跨元素访问、
有副作用的 element、重复/悬空指令、原编译器拒绝及失败时文件保护。

acc_cpu 和 acc_cuda 对 0、1、17、257、100003 个元素比较：
生成指令入口、原生串行入口和独立数值参考。
还测试重复调用、描述符错误、别名、已有映射以及进入区域中途失败的清理。
CUDA 测试使用本机 RTX 3080 Ti。

建议依次阅读：

1. src/frontend/Lexer.cpp：tok_acc 如何区别于注释；
2. src/backend/mlir/AccDirective.cpp：语法与保守并行合法性检查；
3. src/backend/mlir/MLIRGen.cpp：把指令与 AST 绑定并记录 IR 计划；
4. src/backend/mlir/Export.cpp：生成宿主入口与 PTX kernel；
5. src/runtime/DeviceRuntime.cpp 的 krt_acc_map：数据区域的生命周期；
6. examples/AccDriver.cpp 与 tests/acc_directive_test.py：运行和错误验证。

这项练习提供指令编译与运行时衔接的实践，但不能替代真实 Flang/OpenACC
开发经历。以后迁移时，需要把教学语法、元数据和运行时契约分别替换为
Flang 的解析/语义检查、真实 acc dialect 和目标 OpenACC 运行时。

## 本次本地验收

2026-11-14：MLIR/CUDA 完整测试 20/20 通过，含 acc_cuda 的真实 GPU 执行；
LLVM 21 独立构建 14/14 通过。新增 acc_directives、acc_cpu、acc_cuda 均通过，
GPU 测试没有跳过。日志位于 build/mlir/acc-final-tests.log 与 build/acc-tests.log。
