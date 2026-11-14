#include "MLIRGen.h"
#include "AccDirective.h"
#include <optional>
#include "KaleDialect.h"
#include "kaleidoscope/frontend/Parser.h"
#include "kaleidoscope/frontend/SemanticAnalyzer.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Verifier.h"
#include <map>

namespace kaleidoscope::mlir_backend {
namespace {
class Generator : private ConstExprVisitor {
    mlir::OpBuilder b;
    Diagnostics &diag;
    const std::string &filename;
    mlir::ModuleOp module;
    mlir::Block *entry = nullptr;
    struct Binding {
        mlir::Value value;
        bool array;
    };
    std::map<std::string, Binding> bindings;
    mlir::Value result;

    mlir::Location loc(SourceLocation p) {
        return mlir::FileLineColLoc::get(b.getContext(), filename, p.Line, p.Column);
    }
    mlir::Type lowerType(Type t) {
        switch (t) {
        case Type::Double:
            return b.getF64Type();
        case Type::I32:
            return b.getI32Type();
        case Type::Bool:
            return b.getI1Type();
        case Type::ArrayDouble:
            return mlir::MemRefType::get({mlir::ShapedType::kDynamic}, b.getF64Type());
        default:
            llvm_unreachable("unchecked type");
        }
    }
    mlir::Value constant(mlir::Location l, Type t, double v) {
        auto type = lowerType(t);
        mlir::TypedAttr attr =
            t == Type::Double ? mlir::TypedAttr(b.getFloatAttr(type, v))
                              : mlir::TypedAttr(b.getIntegerAttr(type, static_cast<int64_t>(v)));
        return ConstantOp::create(b, l, type, attr);
    }
    mlir::Value emit(const ExprAST &expr) {
        result = {};
        expr.accept(*this);
        return result;
    }
    mlir::Value slot(mlir::Location l, mlir::Value initial) {
        mlir::Value storage;
        {
            mlir::OpBuilder::InsertionGuard guard(b);
            b.setInsertionPointToStart(entry);
            storage =
                mlir::memref::AllocaOp::create(b, l, mlir::MemRefType::get({}, initial.getType()));
        }
        mlir::memref::StoreOp::create(b, l, initial, storage);
        return storage;
    }
    mlir::Value condition(mlir::Location l, mlir::Value v) {
        if (v.getType().isInteger(1))
            return v;
        if (v.getType().isF64())
            return mlir::arith::CmpFOp::create(b, l, mlir::arith::CmpFPredicate::ONE, v,
                                               constant(l, Type::Double, 0));
        return mlir::arith::CmpIOp::create(b, l, mlir::arith::CmpIPredicate::ne, v,
                                           constant(l, Type::I32, 0));
    }
    mlir::Value convert(mlir::Location l, mlir::Value v, Type dest) {
        auto type = lowerType(dest);
        if (type == v.getType())
            return v;
        if (dest == Type::Bool)
            return condition(l, v);
        if (dest == Type::Double) {
            if (v.getType().isInteger(1))
                return mlir::arith::UIToFPOp::create(b, l, type, v);
            return mlir::arith::SIToFPOp::create(b, l, type, v);
        }
        if (v.getType().isInteger(1))
            return mlir::arith::ExtUIOp::create(b, l, type, v);
        // Match the original backend's fptosi.sat: clamp, and map NaN to zero.
        auto zero = constant(l, Type::Double, 0);
        auto lo = constant(l, Type::Double, -2147483648.0);
        auto hi = constant(l, Type::Double, 2147483647.0);
        auto nan = mlir::arith::CmpFOp::create(b, l, mlir::arith::CmpFPredicate::UNO, v, v);
        v = mlir::arith::SelectOp::create(b, l, nan, zero, v);
        auto below = mlir::arith::CmpFOp::create(b, l, mlir::arith::CmpFPredicate::OLT, v, lo);
        v = mlir::arith::SelectOp::create(b, l, below, lo, v);
        auto above = mlir::arith::CmpFOp::create(b, l, mlir::arith::CmpFPredicate::OGT, v, hi);
        v = mlir::arith::SelectOp::create(b, l, above, hi, v);
        return mlir::arith::FPToSIOp::create(b, l, type, v);
    }
    void error(const ExprAST &node, const char *message) {
        diag.error(message, node.location());
        result = {};
    }
    void visit(const NumberExprAST &n) override {
        result = constant(loc(n.location()), n.type(), n.value());
    }
    void visit(const VariableExprAST &n) override {
        auto it = bindings.find(n.getName());
        if (it == bindings.end()) {
            error(n, "MLIR backend does not support globals");
            return;
        }
        result =
            it->second.array
                ? it->second.value
                : mlir::Value(mlir::memref::LoadOp::create(b, loc(n.location()), it->second.value));
    }
    void visit(const BinaryExprAST &n) override {
        auto l = loc(n.location());
        if (n.opcode() == '=') {
            auto v = emit(n.rhs());
            if (!v)
                return;
            auto it = bindings.find(*n.lhs().variableName());
            if (it == bindings.end() || it->second.array) {
                error(n, "Invalid scalar assignment");
                return;
            }
            mlir::memref::StoreOp::create(b, l, v, it->second.value);
            result = v;
            return;
        }
        auto lhs = emit(n.lhs());
        if (!lhs)
            return;
        auto rhs = emit(n.rhs());
        if (!rhs)
            return;
        switch (n.opcode()) {
        case '+':
            result = AddOp::create(b, l, lhs.getType(), lhs, rhs);
            break;
        case '-':
            result = SubOp::create(b, l, lhs.getType(), lhs, rhs);
            break;
        case '*':
            result = MulOp::create(b, l, lhs.getType(), lhs, rhs);
            break;
        case '<':
            if (lhs.getType().isF64()) {
                auto cmp =
                    mlir::arith::CmpFOp::create(b, l, mlir::arith::CmpFPredicate::ULT, lhs, rhs);
                result = mlir::arith::UIToFPOp::create(b, l, b.getF64Type(), cmp);
            } else
                result =
                    mlir::arith::CmpIOp::create(b, l, mlir::arith::CmpIPredicate::slt, lhs, rhs);
            break;
        default:
            error(n, "Custom operators are not supported by the MLIR backend");
        }
    }
    void visit(const UnaryExprAST &n) override {
        auto v = emit(n.operand());
        if (!v)
            return;
        auto l = loc(n.location());
        if (n.type() == Type::I32 && n.opcode() == '+')
            result = v;
        else if (n.type() == Type::I32 && n.opcode() == '-')
            result = SubOp::create(b, l, v.getType(), constant(l, Type::I32, 0), v);
        else if (n.type() == Type::Bool && n.opcode() == '!')
            result = mlir::arith::XOrIOp::create(b, l, v, constant(l, Type::Bool, 1));
        else
            error(n, "Custom unary operators are not supported by the MLIR backend");
    }
    void visit(const CallExprAST &n) override {
        llvm::SmallVector<mlir::Value> args;
        for (auto &arg : n.arguments()) {
            auto v = emit(*arg);
            if (!v)
                return;
            args.push_back(v);
        }
        auto l = loc(n.location());
        auto dest = namedType(n.callee());
        if (isValueType(dest)) {
            result = convert(l, args[0], dest);
            return;
        }
        if (n.callee() == "size") {
            auto length = mlir::memref::DimOp::create(b, l, args[0], 0);
            result = mlir::arith::IndexCastOp::create(b, l, b.getI32Type(), length);
            return;
        }
        if (n.callee() == "load" || n.callee() == "store") {
            auto index = mlir::arith::IndexCastOp::create(b, l, b.getIndexType(), args[1]);
            auto length = mlir::memref::DimOp::create(b, l, args[0], 0);
            auto nonnegative = mlir::arith::CmpIOp::create(b, l, mlir::arith::CmpIPredicate::sge,
                                                           args[1], constant(l, Type::I32, 0));
            auto inrange =
                mlir::arith::CmpIOp::create(b, l, mlir::arith::CmpIPredicate::slt, index, length);
            auto valid = mlir::arith::AndIOp::create(b, l, nonnegative, inrange);
            mlir::cf::AssertOp::create(b, l, valid, "Kaleidoscope array index out of bounds");
            if (n.callee() == "load")
                result = mlir::memref::LoadOp::create(b, l, args[0], mlir::ValueRange{index});
            else {
                mlir::memref::StoreOp::create(b, l, args[2], args[0], mlir::ValueRange{index});
                result = args[2];
            }
            return;
        }
        auto fn = module.lookupSymbol<mlir::func::FuncOp>(n.callee());
        if (!fn) {
            error(n, "Missing lowered callee");
            return;
        }
        result = mlir::func::CallOp::create(b, l, fn, args).getResult(0);
    }
    void visit(const IfExprAST &n) override {
        auto cond = emit(n.condition());
        if (!cond)
            return;
        auto l = loc(n.location());
        auto op = mlir::scf::IfOp::create(b, l, mlir::TypeRange{lowerType(n.type())},
                                          condition(l, cond), true);
        bool ok = true;
        {
            mlir::OpBuilder::InsertionGuard guard(b);
            b.setInsertionPointToStart(&op.getThenRegion().front());
            auto v = emit(n.thenExpr());
            if (v)
                mlir::scf::YieldOp::create(b, l, v);
            else
                ok = false;
            b.setInsertionPointToStart(&op.getElseRegion().front());
            v = emit(n.elseExpr());
            if (v)
                mlir::scf::YieldOp::create(b, l, v);
            else
                ok = false;
        }
        result = ok ? op.getResult(0) : mlir::Value{};
    }
    void visit(const VarExprAST &n) override {
        auto saved = bindings;
        for (size_t i = 0; i < n.bindings().size(); ++i) {
            const auto &binding = n.bindings()[i];
            auto v = binding.second ? emit(*binding.second)
                                    : constant(loc(n.location()), n.bindingTypes()[i], 0);
            if (!v) {
                bindings = std::move(saved);
                return;
            }
            bindings.insert_or_assign(binding.first, Binding{slot(loc(n.location()), v), false});
        }
        auto value = emit(n.body());
        bindings = std::move(saved);
        result = value;
    }
    void visit(const ForExprAST &n) override {
        auto initial = emit(n.start());
        if (!initial)
            return;
        auto saved = bindings;
        auto l = loc(n.location());
        auto storage = slot(l, initial);
        bindings.insert_or_assign(n.variable(), Binding{storage, false});
        auto loop = mlir::scf::WhileOp::create(b, l, mlir::TypeRange{}, mlir::ValueRange{});
        loop.getBefore().push_back(new mlir::Block());
        loop.getAfter().push_back(new mlir::Block());
        bool ok = false;
        {
            mlir::OpBuilder::InsertionGuard guard(b);
            // Kale's loop is post-tested. Its body belongs in the BEFORE region.
            b.setInsertionPointToStart(&loop.getBefore().front());
            auto body = emit(n.body());
            auto step = body ? (n.step() ? emit(*n.step()) : constant(l, n.bindingType(), 1))
                             : mlir::Value{};
            auto end = step ? emit(n.end()) : mlir::Value{};
            if (end) {
                auto current = mlir::memref::LoadOp::create(b, l, storage);
                auto next = AddOp::create(b, l, initial.getType(), current, step);
                mlir::memref::StoreOp::create(b, l, next, storage);
                mlir::scf::ConditionOp::create(b, l, condition(l, end), mlir::ValueRange{});
                ok = true;
            }
            b.setInsertionPointToStart(&loop.getAfter().front());
            mlir::scf::YieldOp::create(b, l);
        }
        bindings = std::move(saved);
        result = ok ? constant(l, Type::Double, 0) : mlir::Value{};
    }

  public:
    Generator(mlir::MLIRContext &ctx, Diagnostics &d, const std::string &file, mlir::ModuleOp m)
        : b(&ctx), diag(d), filename(file), module(m) {}
    mlir::func::FuncOp declare(const PrototypeAST &p) {
        if (p.isUnaryOp() || p.isBinaryOp()) {
            diag.error("Custom operator definitions are outside the MLIR backend", p.location());
            return {};
        }
        if (auto old = module.lookupSymbol<mlir::func::FuncOp>(p.getName()))
            return old;
        llvm::SmallVector<mlir::Type> types;
        bool arrays = false;
        for (auto t : p.signature().Parameters) {
            types.push_back(lowerType(t));
            arrays |= t == Type::ArrayDouble;
        }
        b.setInsertionPointToEnd(module.getBody());
        auto fn =
            mlir::func::FuncOp::create(b, loc(p.location()), p.getName(),
                                       b.getFunctionType(types, {lowerType(p.signature().Result)}));
        fn.setPrivate(); // External declarations must have private symbol visibility in MLIR.
        if (arrays)
            fn->setAttr("llvm.emit_c_interface", b.getUnitAttr());
        if (p.signature().Result == Type::Bool)
            fn.setResultAttr(0, "llvm.zeroext", b.getUnitAttr());
        for (unsigned i = 0; i < types.size(); ++i)
            if (types[i].isInteger(1))
                fn.setArgAttr(i, "llvm.zeroext", b.getUnitAttr());
        return fn;
    }
    bool generate(const FunctionAST &ast) {
        auto fn = declare(ast.prototype());
        if (!fn)
            return false;
        fn.setPublic();
        entry = fn.addEntryBlock();
        b.setInsertionPointToStart(entry);
        bindings.clear();
        for (size_t i = 0; i < ast.prototype().arguments().size(); ++i) {
            auto arg = entry->getArgument(i);
            bool array = ast.prototype().signature().Parameters[i] == Type::ArrayDouble;
            bindings.emplace(
                ast.prototype().arguments()[i],
                Binding{array ? mlir::Value(arg) : slot(loc(ast.prototype().location()), arg),
                        array});
        }
        auto value = emit(ast.body());
        if (!value) {
            fn.erase();
            return false;
        }
        mlir::func::ReturnOp::create(b, loc(ast.body().location()), value);
        return true;
    }
};
} // namespace

mlir::OwningOpRef<mlir::ModuleOp> generateMLIR(std::istream &input, const std::string &filename,
                                               mlir::MLIRContext &context, Diagnostics &diagnostics,
                                               SourceManager &source) {
    context.loadDialect<KaleDialect, mlir::func::FuncDialect, mlir::arith::ArithDialect,
                        mlir::scf::SCFDialect, mlir::memref::MemRefDialect,
                        mlir::cf::ControlFlowDialect>();
    auto module =
        mlir::OwningOpRef<mlir::ModuleOp>(mlir::ModuleOp::create(mlir::UnknownLoc::get(&context)));
    OperatorTable operators{{'=', 2}, {'<', 10}, {'+', 20}, {'-', 20}, {'*', 40}};
    Lexer lexer(input, &source);
    Parser parser(lexer, diagnostics, operators);
    SemanticAnalyzer semantics(diagnostics, operators, true);
    Generator generator(context, diagnostics, filename, *module);
    std::optional<AccDirective> pending;
    std::map<std::string, std::unique_ptr<FunctionAST>> definitions;
    bool hasAcc = false;
    parser.getNextToken();
    while (parser.currentToken() != tok_eof) {
        if (parser.currentToken() == tok_acc) {
            AccDirective directive;
            if (pending || hasAcc) {
                diagnostics.error("Only one #acc region is supported per module", lexer.location());
                return {};
            }
            if (!parseAccDirective(lexer.identifier(), lexer.location(), diagnostics, directive))
                return {};
            pending = directive;
            parser.getNextToken();
            if (parser.currentToken() != tok_def) {
                diagnostics.error("#acc must immediately precede a function definition",
                                  directive.location);
                return {};
            }
            continue;
        }
        if (parser.currentToken() == ';') {
            parser.getNextToken();
            continue;
        }
        if (parser.currentToken() == tok_extern) {
            auto ast = parser.ParseExtern();
            if (!ast || !semantics.analyze(*ast) || !generator.declare(*ast))
                return {};
            semantics.commit(*ast);
        } else if (parser.currentToken() == tok_def) {
            auto ast = parser.ParseDefinition();
            if (!ast || !semantics.analyze(*ast))
                return {};
            std::string element;
            if (pending && !validateAccLoop(*pending, *ast, definitions, diagnostics, element))
                return {};
            if (!generator.generate(*ast))
                return {};
            if (pending) {
                mlir::OpBuilder attributes(&context);
                module->getOperation()->setAttr(
                    "kale.acc_plan",
                    attributes.getDictionaryAttr(
                        {attributes.getNamedAttr(
                             "function", attributes.getStringAttr(ast->prototype().getName())),
                         attributes.getNamedAttr("element", attributes.getStringAttr(element)),
                         attributes.getNamedAttr(
                             "copyin", attributes.getStrArrayAttr({pending->x, pending->y})),
                         attributes.getNamedAttr("copyout",
                                                 attributes.getStrArrayAttr({pending->out}))}));
                pending.reset();
                hasAcc = true;
            }
            semantics.commit(*ast);
            definitions.emplace(ast->prototype().getName(), std::move(ast));
        } else {
            diagnostics.error(
                "MLIR backend accepts function definitions and extern declarations only",
                lexer.location());
            return {};
        }
    }
    if (diagnostics.hasError() || mlir::failed(mlir::verify(*module)))
        return {};
    return module;
}
} // namespace kaleidoscope::mlir_backend
