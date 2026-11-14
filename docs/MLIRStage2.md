# 第二阶段：理解 Dialect Conversion

当前工程已从最初的标量练习扩展到执行、控制流、数组和 GPU。
完整构建命令及功能范围见 [执行与 GPU 指南](MLIRExecutionAndGPU.md)。
这里保留核心概念和建议的阅读顺序。

## 最小概念路径

```text
源码 → AST → SemanticAnalyzer
     → func + kale（可变变量另用 memref，控制流另用 scf）
     → kale-to-arith
     → func + arith + 其他合法 dialect
```

kale.constant/add/sub/mul 在 KaleOps.td 中用 ODS 定义，
KaleDialect.cpp 注册操作并验证 constant 的属性类型与结果类型。
算术只接受 f64/i32；bool 常量使用 i1，bool 加法会被拒绝。

## 六个重点

| 概念 | 对应实现 |
| --- | --- |
| AST → IR | MLIRGen.cpp 的 ConstExprVisitor，通过只读 getter 获取源码结构 |
| Operation | kale.add 有两个 operands 和一个 result；module/func 也是 Operation |
| Region / Block | func 持有函数体 Region，Region 的入口 Block 持有参数和操作 |
| SSA value | Block argument 和操作 result 都是 Value；可变源变量另有 memref 存储 |
| Pass / ConversionPattern | LowerToArith.cpp 组织规则，通过 adaptor 读取重映射后的 operands |
| ConversionTarget | 显式允许目标 dialect，禁止 kale，applyFullConversion 保证全部合法化 |

没有类型改变时也可以做真正的 Dialect Conversion。
本阶段的 kale → arith 不需要 TypeConverter；下一阶段的 memref/func → LLVM
才引入 LLVMTypeConverter 和函数签名转换。

## 观察一个例子

```text
def sum(a b) a+b;
```

概念上的 SSA 算术是：

```mlir
%0 = kale.add %a, %b : f64
return %0 : f64
```

当前生成器为可变参数建立 rank-0 memref 存储，所以真实输出还会包含
alloca/store/load；它们让后续赋值、循环与遮蔽语义保持一致。
转换后 kale.add 被 arith.addf 替换，所有使用者通过 rewriter 更新。

constant 的格式同时显示属性类型与结果类型：
`kale.constant 2.000000e+00 : f64 : f64`。
第一个 f64 属于属性，第二个属于 result。

## 构建、打印与测试

```bash
cmake -S . -B build/mlir \
  -DLLVM_DIR=/usr/lib/llvm-23/lib/cmake/llvm \
  -DMLIR_DIR=/usr/lib/llvm-23/lib/cmake/mlir \
  -DKALEIDOSCOPE_ENABLE_MLIR=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/mlir -j4

build/mlir/src/backend/mlir/kale-mlir examples/MLIRScalars.kale
build/mlir/src/backend/mlir/kale-mlir --lower-to-arith examples/MLIRScalars.kale
build/mlir/src/backend/mlir/kale-mlir examples/MLIRScalars.kale |
  build/mlir/src/backend/mlir/kale-opt \
  --pass-pipeline='builtin.module(kale-to-arith)' --mlir-print-ir-after-all

ctest --test-dir build/mlir -R '^mlir_(conversion|missing_pattern)$' --output-on-failure
```

不要将 module/func 标记为 recursively legal，否则内部 kale 操作可能被豁免。
“某个操作自身合法”与“它所包含的全部操作递归合法”是不同的约束。

成功测试检查目标运算、SSA 引用、无残留 kale、可重新解析及 Pass 幂等性。
失败测试分别覆盖 ODS/自定义 verifier、非法目标操作，以及故意缺少 AddOp pattern。
合法输入没有重写规则时也必须转换失败；完整规则集处理相同输入则成功。

推荐阅读顺序：AST.h → KaleOps.td → KaleDialect.cpp → MLIRGen.cpp
→ LowerToArith.cpp → kale-opt.cpp → 两个转换测试。

参考：[Dialect Conversion](https://mlir.llvm.org/docs/DialectConversion/)、
[ODS](https://mlir.llvm.org/docs/DefiningDialects/Operations/)。
