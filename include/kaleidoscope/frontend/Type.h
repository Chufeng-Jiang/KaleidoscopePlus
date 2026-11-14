#pragma once
#include <string_view>
#include <vector>
namespace kaleidoscope {
// Unknown means not analyzed; Error propagates a diagnosed failure.
// Value types are independent of LLVM representations.
enum class Type { Unknown, Double, I32, Bool, ArrayDouble, Error };
constexpr std::string_view typeName(Type type) {
    switch (type) {
    case Type::Double:
        return "double";
    case Type::I32:
        return "i32";
    case Type::Bool:
        return "bool";
    case Type::ArrayDouble:
        return "array";
    case Type::Error:
        return "<error>";
    case Type::Unknown:
        return "<unknown>";
    }
    return "<unknown>";
}
constexpr bool isValueType(Type type) {
    return type == Type::Double || type == Type::I32 || type == Type::Bool;
}
constexpr bool isAssignable(Type destination, Type source) {
    return (isValueType(destination) || destination == Type::ArrayDouble) && destination == source;
}
constexpr Type namedType(std::string_view name) {
    return name == "double" ? Type::Double
           : name == "i32"  ? Type::I32
           : name == "bool" ? Type::Bool
                            : name == "array" ? Type::ArrayDouble
                            : Type::Unknown;
}
struct FunctionType {
    std::vector<Type> Parameters;
    Type Result = Type::Double;
    bool operator==(const FunctionType &other) const {
        return Parameters == other.Parameters && Result == other.Result;
    }
    bool operator!=(const FunctionType &other) const { return !(*this == other); }
};
} // namespace kaleidoscope
