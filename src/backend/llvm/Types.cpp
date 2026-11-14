#include "CodegenContext.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Intrinsics.h"
namespace kaleidoscope {
llvm::Type *CodegenContext::lowerType(Type type) {
    switch (type) {
    case Type::Double:
        return llvm::Type::getDoubleTy(*TheContext);
    case Type::I32:
        return llvm::Type::getInt32Ty(*TheContext);
    case Type::Bool:
        return llvm::Type::getInt1Ty(*TheContext);
    default:
        llvm_unreachable("Cannot lower an unchecked type");
    }
}
Constant *CodegenContext::scalarConstant(double value, Type type) {
    if (type == Type::Double)
        return ConstantFP::get(lowerType(type), value);
    return ConstantInt::get(lowerType(type), static_cast<int64_t>(value), true);
}
Value *CodegenContext::condition(Value *value) {
    if (value->getType()->isIntegerTy(1))
        return value;
    auto *zero = Constant::getNullValue(value->getType());
    if (value->getType()->isDoubleTy())
        return Builder->CreateFCmpONE(value, zero, "condition");
    return Builder->CreateICmpNE(value, zero, "condition");
}
Value *CodegenContext::convert(Value *value, Type destination) {
    auto *target = lowerType(destination);
    if (value->getType() == target)
        return value;
    if (destination == Type::Bool)
        return condition(value);
    if (destination == Type::Double)
        return value->getType()->isIntegerTy(1) ? Builder->CreateUIToFP(value, target, "convert")
                                                : Builder->CreateSIToFP(value, target, "convert");
    if (value->getType()->isIntegerTy(1))
        return Builder->CreateZExt(value, target, "convert");
    // Defined for NaN/infinity/out-of-range input: LLVM's saturating conversion.
    return Builder->CreateIntrinsic(Intrinsic::fptosi_sat, {target, value->getType()}, {value});
}
Value *CodegenContext::call(Function *function, ArrayRef<Value *> arguments, const Twine &name) {
    auto *result = Builder->CreateCall(function, arguments, name);
    result->setAttributes(function->getAttributes());
    return result;
}
} // namespace kaleidoscope
