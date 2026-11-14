#include "kaleidoscope/frontend/SemanticAnalyzer.h"
#include <cmath>
namespace kaleidoscope {
void SemanticAnalyzer::previous(const Symbol &symbol) {
    Diag.note("Previous declaration is here", symbol.Location);
}
bool SemanticAnalyzer::checkPrototype(const PrototypeAST &prototype, bool definition,
                                      bool temporary) {
    auto count = Diag.errorCount();
    if (ArraysEnabled && (prototype.getName() == "load" || prototype.getName() == "store" ||
                          prototype.getName() == "size")) {
        Diag.error("Array intrinsic name is reserved", prototype.location());
        return false;
    }
    if (prototype.signature().Parameters.size() != prototype.Args.size() ||
        !isValueType(prototype.signature().Result)) {
        Diag.error("Invalid function type", prototype.location());
        return false;
    }
    for (auto type : prototype.signature().Parameters)
        if (!isValueType(type) && !(ArraysEnabled && type == Type::ArrayDouble)) {
            Diag.error("Invalid parameter type", prototype.location());
            return false;
        }
    std::map<std::string, SourceLocation> arguments;
    for (std::size_t i = 0; i < prototype.Args.size(); ++i) {
        auto loc = i < prototype.ArgumentLocations.size() ? prototype.ArgumentLocations[i]
                                                          : prototype.location();
        auto [entry, inserted] = arguments.emplace(prototype.Args[i], loc);
        if (!inserted) {
            Diag.error("Duplicate parameter name '" + prototype.Args[i] + "'", loc);
            Diag.note("Previous parameter is here", entry->second);
        }
    }
    auto existing = Symbols.find(prototype.Name);
    if (!temporary && existing != Symbols.end()) {
        const auto &symbol = existing->second;
        if (symbol.Category != Kind::Function) {
            Diag.error("Function name conflicts with a global variable '" + prototype.Name + "'",
                       prototype.location());
            previous(symbol);
        } else if (symbol.Signature != prototype.signature()) {
            Diag.error("Function signature does not match previous declaration of '" +
                           prototype.Name + "'",
                       prototype.location());
            previous(symbol);
        } else if (definition && symbol.Defined) {
            Diag.error(prototype.Name == "__anon_expr"
                           ? "Only one top-level expression is supported in file compilation"
                           : "Function is already defined: '" + prototype.Name + "'",
                       prototype.location());
            previous(symbol);
        }
    }
    return count == Diag.errorCount();
}
bool SemanticAnalyzer::analyze(const PrototypeAST &prototype) {
    return checkPrototype(prototype, false);
}
bool SemanticAnalyzer::analyze(const GlobalAST &global) {
    if (global.type() == Type::ArrayDouble) {
        Diag.error("Arrays are supported only as borrowed function parameters", global.location());
        return false;
    }
    bool valid =
        requireType(global.InitializerType, global.type(), global.location(), "Global initializer");
    valid = checkLiteral(global.Initializer, global.InitializerType, global.location()) && valid;
    if (!valid)
        return false;
    auto existing = Symbols.find(global.Name);
    if (existing == Symbols.end())
        return true;
    const auto &symbol = existing->second;
    if (symbol.Category != Kind::Global) {
        Diag.error("Global name conflicts with a function '" + global.Name + "'",
                   global.location());
        previous(symbol);
        return false;
    }
    if (symbol.ValueType != global.type()) {
        Diag.error("Global type does not match previous declaration", global.location());
        previous(symbol);
        return false;
    }
    if (!global.External && symbol.Defined) {
        Diag.error("Global variable is already defined: '" + global.Name + "'", global.location());
        previous(symbol);
        return false;
    }
    return true;
}
bool SemanticAnalyzer::analyze(const FunctionAST &function, bool temporary) {
    if (!checkPrototype(*function.Proto, true, temporary))
        return false;
    auto count = Diag.errorCount();
    CurrentFunction = function.Proto.get(); // Recursion is valid before committing the definition.
    Scopes.emplace_back();
    auto signature = function.Proto->signature();
    for (std::size_t i = 0; i < signature.Parameters.size(); ++i)
        Scopes.back().emplace(function.Proto->Args[i], signature.Parameters[i]);
    auto bodyType = infer(*function.Body);
    if (function.Proto->Name != "__anon_expr")
        requireType(bodyType, signature.Result, function.Body->location(), "Function return value");
    Scopes.pop_back();
    CurrentFunction = nullptr;
    return count == Diag.errorCount();
}
void SemanticAnalyzer::commit(const PrototypeAST &prototype) {
    Symbols.try_emplace(prototype.Name, Symbol{Kind::Function, prototype.signature(), Type::Unknown,
                                               false, prototype.location()});
}
void SemanticAnalyzer::commit(const GlobalAST &global) {
    auto [entry, inserted] = Symbols.try_emplace(
        global.Name, Symbol{Kind::Global, {}, global.type(), !global.External, global.location()});
    if (!global.External) {
        entry->second.Defined = true;
        entry->second.Location = global.location();
    }
}
void SemanticAnalyzer::commit(const FunctionAST &function, bool temporary) {
    if (temporary)
        return;
    const auto &p = *function.Proto;
    Symbols.insert_or_assign(
        p.Name, Symbol{Kind::Function, p.signature(), Type::Unknown, true, p.location()});
    if (p.isBinaryOp())
        Operators[p.getOperatorName()] = p.getBinaryPrecedence();
}
Type SemanticAnalyzer::variableType(const std::string &name) const {
    for (auto scope = Scopes.rbegin(); scope != Scopes.rend(); ++scope) {
        auto entry = scope->find(name);
        if (entry != scope->end())
            return entry->second;
    }
    auto symbol = Symbols.find(name);
    return symbol != Symbols.end() && symbol->second.Category == Kind::Global
               ? symbol->second.ValueType
               : Type::Unknown;
}
Type SemanticAnalyzer::infer(const ExprAST &expression) {
    auto errors = Diag.errorCount();
    expression.InferredType = Type::Unknown;
    expression.accept(*this);
    if (errors != Diag.errorCount())
        expression.InferredType = Type::Error;
    return expression.type();
}
bool SemanticAnalyzer::requireType(Type actual, Type expected, SourceLocation location,
                                   const std::string &context) {
    if (actual == Type::Error)
        return false; // The child already supplied a diagnostic.
    if (isAssignable(expected, actual))
        return true;
    Diag.error(context + ": expected " + std::string(typeName(expected)) + ", got " +
                   std::string(typeName(actual)),
               location);
    return false;
}
bool SemanticAnalyzer::checkCondition(Type type, SourceLocation location) {
    if (type == Type::Error)
        return false;
    if (isValueType(type))
        return true;
    Diag.error("Condition must have a scalar value", location);
    return false;
}
bool SemanticAnalyzer::checkLiteral(double value, Type type, SourceLocation location) {
    if (type == Type::I32 && (!std::isfinite(value) || std::trunc(value) != value ||
                              value < -2147483648.0 || value > 2147483647.0)) {
        Diag.error("Integer literal is outside the i32 range", location);
        return false;
    }
    return true;
}
Type SemanticAnalyzer::checkCall(const std::string &name, const std::vector<Type> &arguments,
                                 SourceLocation location) {
    if (ArraysEnabled && (name == "load" || name == "store" || name == "size")) {
        std::vector<Type> expected{Type::ArrayDouble};
        if (name != "size") expected.push_back(Type::I32);
        if (name == "store") expected.push_back(Type::Double);
        if (arguments.size() != expected.size()) {
            Diag.error("Incorrect array intrinsic argument count", location);
            return Type::Error;
        }
        bool valid = true;
        for (size_t i = 0; i < expected.size(); ++i)
            valid = requireType(arguments[i], expected[i], location, "Array intrinsic argument") && valid;
        return valid ? (name == "size" ? Type::I32 : Type::Double) : Type::Error;
    }
    auto conversion = namedType(name);
    if (conversion == Type::ArrayDouble) {
        Diag.error("array is a borrowed parameter type, not a conversion", location);
        return Type::Error;
    }
    if (conversion != Type::Unknown) {
        if (arguments.size() != 1) {
            Diag.error("Conversion requires exactly one argument", location);
            return Type::Error;
        }
        if (!isValueType(arguments[0]) && arguments[0] != Type::Error)
            Diag.error("Conversion requires a scalar argument", location);
        return isValueType(arguments[0]) ? conversion : Type::Error;
    }
    FunctionType signature;
    if (CurrentFunction && CurrentFunction->Name == name)
        signature = CurrentFunction->signature();
    else {
        auto symbol = Symbols.find(name);
        if (symbol == Symbols.end() || symbol->second.Category != Kind::Function) {
            Diag.error("Unknown function referenced '" + name + "'", location);
            return Type::Error;
        }
        signature = symbol->second.Signature;
    }
    if (arguments.size() != signature.Parameters.size()) {
        Diag.error("Incorrect # arguments passed to '" + name + "': expected " +
                       std::to_string(signature.Parameters.size()) + ", got " +
                       std::to_string(arguments.size()),
                   location);
        return Type::Error;
    }
    bool valid = true;
    for (std::size_t i = 0; i < arguments.size(); ++i)
        valid = requireType(arguments[i], signature.Parameters[i], location,
                            "Argument " + std::to_string(i + 1) + " of '" + name + "'") &&
                valid;
    return valid ? signature.Result : Type::Error;
}
void SemanticAnalyzer::visit(const NumberExprAST &node) {
    node.InferredType =
        checkLiteral(node.Val, node.LiteralType, node.location()) ? node.LiteralType : Type::Error;
}
void SemanticAnalyzer::visit(const VariableExprAST &node) {
    node.InferredType = variableType(node.Name);
    if (node.type() == Type::Unknown)
        Diag.error("Unknown variable name '" + node.Name + "'", node.location());
}
void SemanticAnalyzer::visit(const UnaryExprAST &node) {
    auto operand = infer(*node.Operand);
    if ((operand == Type::I32 && (node.Opcode == '-' || node.Opcode == '+')) ||
        (operand == Type::Bool && node.Opcode == '!'))
        node.InferredType = operand;
    else if (operand == Type::Error)
        node.InferredType = Type::Error;
    else
        node.InferredType =
            checkCall(std::string("unary") + node.Opcode, {operand}, node.location());
}
void SemanticAnalyzer::visit(const BinaryExprAST &node) {
    auto left = infer(*node.LHS);
    auto right = infer(*node.RHS);
    if (node.Op == '=') {
        if (left == Type::ArrayDouble) {
            Diag.error("Array parameters cannot be reassigned; use store", node.location());
            return;
        }
        if (!node.LHS->variableName()) {
            Diag.error("destination of '=' must be a variable", node.LHS->location());
            return;
        }
        node.InferredType =
            left != Type::Error && requireType(right, left, node.RHS->location(), "Assignment")
                ? left
                : Type::Error;
    } else if (node.Op == '+' || node.Op == '-' || node.Op == '*' || node.Op == '<') {
        bool valid = true;
        if (left != Type::Error && left != Type::Double && left != Type::I32) {
            Diag.error("Arithmetic and ordering require double or i32 operands", node.location());
            valid = false;
        }
        valid = left != Type::Error &&
                requireType(right, left, node.RHS->location(), "Right operand") && valid;
        node.InferredType =
            valid ? (node.Op == '<' && left == Type::I32 ? Type::Bool : left) : Type::Error;
    } else
        node.InferredType =
            checkCall(std::string("binary") + node.Op, {left, right}, node.location());
}
void SemanticAnalyzer::visit(const CallExprAST &node) {
    std::vector<Type> arguments;
    for (const auto &arg : node.Args)
        arguments.push_back(infer(*arg));
    node.InferredType = checkCall(node.Callee, arguments, node.location());
}
void SemanticAnalyzer::visit(const IfExprAST &node) {
    bool valid = checkCondition(infer(*node.Cond), node.Cond->location());
    auto thenType = infer(*node.Then);
    auto elseType = infer(*node.Else);
    valid = thenType != Type::Error &&
            requireType(elseType, thenType, node.Else->location(), "Conditional branch") && valid;
    node.InferredType = valid ? thenType : Type::Error;
}
void SemanticAnalyzer::visit(const ForExprAST &node) {
    bool valid = requireType(infer(*node.Start), node.BindingType, node.Start->location(),
                             "Loop initializer");
    if (node.BindingType != Type::Double && node.BindingType != Type::I32) {
        Diag.error("Loop variable must have type double or i32", node.VariableLocation);
        valid = false;
    }
    Scopes.push_back({{node.VarName, node.BindingType}});
    valid = infer(*node.Body) != Type::Error && valid;
    if (node.Step)
        valid =
            requireType(infer(*node.Step), node.BindingType, node.Step->location(), "Loop step") &&
            valid;
    valid = checkCondition(infer(*node.End), node.End->location()) && valid;
    Scopes.pop_back();
    node.InferredType = valid ? Type::Double : Type::Error; // Existing loop result is 0.0.
}
void SemanticAnalyzer::visit(const VarExprAST &node) {
    Scopes.emplace_back();
    bool valid = true;
    for (std::size_t i = 0; i < node.VarNames.size(); ++i) {
        const auto &binding = node.VarNames[i];
        auto declared = node.BindingTypes[i];
        if (declared == Type::ArrayDouble) {
            Diag.error("Arrays are supported only as borrowed function parameters", node.location());
            valid = false;
        }
        auto type = binding.second ? infer(*binding.second) : declared;
        if (binding.second)
            valid =
                requireType(type, declared, binding.second->location(), "Variable initializer") &&
                valid;
        Scopes.back().insert_or_assign(binding.first,
                                       isAssignable(declared, type) ? declared : Type::Error);
    }
    auto bodyType = infer(*node.Body);
    Scopes.pop_back();
    node.InferredType = valid ? bodyType : Type::Error;
}
} // namespace kaleidoscope
