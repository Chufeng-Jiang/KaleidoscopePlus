include "mlir/IR/OpBase.td"
include "mlir/Interfaces/InferTypeOpInterface.td"
include "mlir/IR/BuiltinAttributeInterfaces.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

def Kale_Dialect : Dialect {
  let name = "kale";
  let cppNamespace = "::kaleidoscope::mlir_backend";
}
def Kale_Scalar : AnyTypeOf<[F64, I32, I1]>;
def Kale_Number : AnyTypeOf<[F64, I32]>;

def Kale_ConstantOp : Op<Kale_Dialect, "constant", [Pure]> {
  let summary = "A typed scalar literal";
  let arguments = (ins TypedAttrInterface:$value);
  let results = (outs Kale_Scalar:$result);
  let hasVerifier = 1;
  let assemblyFormat = "$value attr-dict `:` type($result)";
}
class Kale_BinaryOp<string mnemonic> :
    Op<Kale_Dialect, mnemonic, [Pure, SameOperandsAndResultType]> {
  let arguments = (ins Kale_Number:$lhs, Kale_Number:$rhs);
  let results = (outs Kale_Number:$result);
  let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($result)";
}
def Kale_AddOp : Kale_BinaryOp<"add">;
def Kale_SubOp : Kale_BinaryOp<"sub">;
def Kale_MulOp : Kale_BinaryOp<"mul">;
