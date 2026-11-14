#include "AccDirective.h"
#include <regex>
#include <set>
namespace kaleidoscope::mlir_backend {
bool parseAccDirective(const std::string &text, SourceLocation location, Diagnostics &diag,
                       AccDirective &directive) {
    static const std::regex syntax(
        R"(^\s*parallel\s+loop\s+copyin\s*\(\s*([A-Za-z][A-Za-z0-9]*)\s*,\s*([A-Za-z][A-Za-z0-9]*)\s*\)\s+copyout\s*\(\s*([A-Za-z][A-Za-z0-9]*)\s*\)\s*$)");
    std::smatch match;
    if (!std::regex_match(text, match, syntax)) {
        diag.error("Teaching #acc supports only: parallel loop copyin(x,y) copyout(out); "
                   "other clauses and constructs are unsupported",
                   location);
        return false;
    }
    directive = {match[1], match[2], match[3], location};
    return true;
}
namespace {
bool variable(const ExprAST &e, const std::string &name) {
    auto *v = dynamic_cast<const VariableExprAST *>(&e);
    return v && v->getName() == name;
}
bool number(const ExprAST &e, Type type, double value) {
    auto *n = dynamic_cast<const NumberExprAST *>(&e);
    return n && n->type() == type && n->value() == value;
}
const CallExprAST *call(const ExprAST &e, const char *name, size_t arity) {
    auto *c = dynamic_cast<const CallExprAST *>(&e);
    return c && c->callee() == name && c->arguments().size() == arity ? c : nullptr;
}
bool sizeOf(const ExprAST &e, const std::string &name) {
    auto *c = call(e, "size", 1);
    return c && variable(*c->arguments()[0], name);
}
bool loadAt(const ExprAST &e, const std::string &array, const std::string &index) {
    auto *c = call(e, "load", 2);
    return c && variable(*c->arguments()[0], array) && variable(*c->arguments()[1], index);
}
bool pure(const ExprAST &e, const std::set<std::string> &parameters) {
    if (auto *n = dynamic_cast<const NumberExprAST *>(&e))
        return n->type() == Type::Double;
    if (auto *v = dynamic_cast<const VariableExprAST *>(&e))
        return parameters.count(v->getName());
    if (auto *b = dynamic_cast<const BinaryExprAST *>(&e))
        return (b->opcode() == '+' || b->opcode() == '-' || b->opcode() == '*') &&
               pure(b->lhs(), parameters) && pure(b->rhs(), parameters);
    return false;
}
} // namespace
bool validateAccLoop(const AccDirective &d, const FunctionAST &fn,
                     const std::map<std::string, std::unique_ptr<FunctionAST>> &definitions,
                     Diagnostics &diag, std::string &element) {
    auto fail = [&](const char *message) {
        diag.error(message, d.location);
        return false;
    };
    const auto &p = fn.prototype();
    const auto &names = p.arguments();
    if (p.signature().Parameters != std::vector<Type>{Type::ArrayDouble, Type::ArrayDouble,
                                                      Type::ArrayDouble, Type::Double} ||
        p.signature().Result != Type::Double || names.size() != 4)
        return fail("#acc function must have (x: array y: array out: array a) -> double");
    if (d.x != names[0] || d.y != names[1] || d.out != names[2] ||
        std::set<std::string>{d.x, d.y, d.out}.size() != 3)
        return fail("#acc copyin must name the first two array parameters; copyout the third, all "
                    "distinct");
    auto *guard = dynamic_cast<const IfExprAST *>(&fn.body());
    auto *cond = guard ? dynamic_cast<const BinaryExprAST *>(&guard->condition()) : nullptr;
    if (!guard || !cond || cond->opcode() != '<' || !number(cond->lhs(), Type::I32, 0) ||
        !sizeOf(cond->rhs(), d.x) || !number(guard->elseExpr(), Type::Double, 0))
        return fail("#acc loop requires an outer 'if 0i32 < size(x) then ... else 0' guard");
    auto *loop = dynamic_cast<const ForExprAST *>(&guard->thenExpr());
    if (!loop || loop->bindingType() != Type::I32 || !number(loop->start(), Type::I32, 0) ||
        (loop->step() && !number(*loop->step(), Type::I32, 1)))
        return fail("#acc supports only an i32 loop starting at zero with unit step");
    auto *end = dynamic_cast<const BinaryExprAST *>(&loop->end());
    auto *bound = end ? dynamic_cast<const BinaryExprAST *>(&end->rhs()) : nullptr;
    if (!end || end->opcode() != '<' || !variable(end->lhs(), loop->variable()) || !bound ||
        bound->opcode() != '-' || !sizeOf(bound->lhs(), d.x) || !number(bound->rhs(), Type::I32, 1))
        return fail("#acc loop condition must be i < size(x)-1i32 (legacy post-test semantics)");
    auto *store = call(loop->body(), "store", 3);
    if (!store || !variable(*store->arguments()[0], d.out) ||
        !variable(*store->arguments()[1], loop->variable()))
        return fail("#acc body must store exactly one result to out[i]");
    auto *compute = dynamic_cast<const CallExprAST *>(store->arguments()[2].get());
    if (!compute || compute->arguments().size() != 3 ||
        !loadAt(*compute->arguments()[0], d.x, loop->variable()) ||
        !loadAt(*compute->arguments()[1], d.y, loop->variable()) ||
        !variable(*compute->arguments()[2], names[3]))
        return fail("#acc body must be store(out,i,element(load(x,i),load(y,i),a)); cross-element "
                    "accesses are unsupported");
    auto it = definitions.find(compute->callee());
    if (it == definitions.end())
        return fail("#acc element must be defined earlier in this module");
    auto &ep = it->second->prototype();
    if (ep.signature().Parameters != std::vector<Type>(3, Type::Double) ||
        ep.signature().Result != Type::Double ||
        !pure(it->second->body(),
              std::set<std::string>(ep.arguments().begin(), ep.arguments().end())))
        return fail(
            "#acc element must be pure double arithmetic (+,-,*) over its three parameters");
    element = compute->callee();
    return true;
}
} // namespace kaleidoscope::mlir_backend
