#include "kaleidoscope/SemanticAnalyzer.h"
namespace kaleidoscope {
void SemanticAnalyzer::previous(const Symbol &symbol) {
    Diag.note("Previous declaration is here", symbol.Location);
}
bool SemanticAnalyzer::checkPrototype(const PrototypeAST &prototype, bool definition,
                                      bool temporary) {
    auto count = Diag.errorCount();
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
        if (symbol.Type != Kind::Function) {
            Diag.error("Function name conflicts with a global variable '" + prototype.Name + "'",
                       prototype.location());
            previous(symbol);
        } else if (symbol.Arity != prototype.Args.size()) {
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
    auto existing = Symbols.find(global.Name);
    if (existing == Symbols.end())
        return true;
    const auto &symbol = existing->second;
    if (symbol.Type != Kind::Global) {
        Diag.error("Global name conflicts with a function '" + global.Name + "'",
                   global.location());
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
    Scopes.emplace_back(function.Proto->Args.begin(), function.Proto->Args.end());
    function.Body->accept(*this);
    Scopes.pop_back();
    CurrentFunction = nullptr;
    return count == Diag.errorCount();
}
void SemanticAnalyzer::commit(const PrototypeAST &prototype) {
    Symbols.try_emplace(prototype.Name,
                        Symbol{Kind::Function, prototype.Args.size(), false, prototype.location()});
}
void SemanticAnalyzer::commit(const GlobalAST &global) {
    auto [entry, inserted] = Symbols.try_emplace(
        global.Name, Symbol{Kind::Global, 0, !global.External, global.location()});
    if (!global.External) {
        entry->second.Defined = true;
        entry->second.Location = global.location();
    }
}
void SemanticAnalyzer::commit(const FunctionAST &function, bool temporary) {
    if (temporary)
        return;
    const auto &p = *function.Proto;
    Symbols.insert_or_assign(p.Name, Symbol{Kind::Function, p.Args.size(), true, p.location()});
    if (p.isBinaryOp())
        Operators[p.getOperatorName()] = p.getBinaryPrecedence();
}
bool SemanticAnalyzer::variableExists(const std::string &name) const {
    for (auto scope = Scopes.rbegin(); scope != Scopes.rend(); ++scope)
        if (scope->count(name))
            return true;
    auto symbol = Symbols.find(name);
    return symbol != Symbols.end() && symbol->second.Type == Kind::Global;
}
void SemanticAnalyzer::checkCall(const std::string &name, std::size_t arity,
                                 SourceLocation location) {
    std::size_t expected;
    if (CurrentFunction && CurrentFunction->Name == name)
        expected = CurrentFunction->Args.size();
    else {
        auto symbol = Symbols.find(name);
        if (symbol == Symbols.end() || symbol->second.Type != Kind::Function) {
            Diag.error("Unknown function referenced '" + name + "'", location);
            return;
        }
        expected = symbol->second.Arity;
    }
    if (arity != expected)
        Diag.error("Incorrect # arguments passed to '" + name + "': expected " +
                       std::to_string(expected) + ", got " + std::to_string(arity),
                   location);
}
void SemanticAnalyzer::visit(const NumberExprAST &) {}
void SemanticAnalyzer::visit(const VariableExprAST &node) {
    if (!variableExists(node.Name))
        Diag.error("Unknown variable name '" + node.Name + "'", node.location());
}
void SemanticAnalyzer::visit(const UnaryExprAST &node) {
    checkCall(std::string("unary") + node.Opcode, 1, node.location());
    node.Operand->accept(*this);
}
void SemanticAnalyzer::visit(const BinaryExprAST &node) {
    if (node.Op == '=') {
        if (!node.LHS->variableName())
            Diag.error("destination of '=' must be a variable", node.LHS->location());
        else
            node.LHS->accept(*this);
    } else {
        node.LHS->accept(*this);
        if (node.Op != '+' && node.Op != '-' && node.Op != '*' && node.Op != '<')
            checkCall(std::string("binary") + node.Op, 2, node.location());
    }
    node.RHS->accept(*this);
}
void SemanticAnalyzer::visit(const CallExprAST &node) {
    checkCall(node.Callee, node.Args.size(), node.location());
    for (const auto &arg : node.Args)
        arg->accept(*this);
}
void SemanticAnalyzer::visit(const IfExprAST &node) {
    node.Cond->accept(*this);
    node.Then->accept(*this);
    node.Else->accept(*this);
}
void SemanticAnalyzer::visit(const ForExprAST &node) {
    node.Start->accept(*this); // The loop binding is not visible in its initializer.
    Scopes.push_back({node.VarName});
    node.Body->accept(*this);
    if (node.Step)
        node.Step->accept(*this);
    node.End->accept(*this);
    Scopes.pop_back();
}
void SemanticAnalyzer::visit(const VarExprAST &node) {
    Scopes.emplace_back();
    for (const auto &binding : node.VarNames) {
        if (binding.second)
            binding.second->accept(*this);
        Scopes.back().insert(binding.first);
    }
    node.Body->accept(*this);
    Scopes.pop_back();
}
} // namespace kaleidoscope
