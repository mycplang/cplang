// 常量折叠优化器实现

#include "optimizer/constant_folder.hpp"
#include <cmath>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  表达式求值
// ═══════════════════════════════════════════════════════════════════

ConstValue ConstantFolder::tryEval(Shared<Expr> expr) {
    if (!expr) return ConstValue::nil();
    
    // 字面量
    if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
        // Variant<Int64, Float64, String, bool> - 使用 std::get 访问
        if (auto* pInt = std::get_if<Int64>(&lit->value)) {
            return ConstValue::fromInt(*pInt);
        } else if (auto* pFloat = std::get_if<Float64>(&lit->value)) {
            return ConstValue::fromFloat(*pFloat);
        } else if (auto* pBool = std::get_if<bool>(&lit->value)) {
            return ConstValue::fromBool(*pBool);
        } else if (auto* pStr = std::get_if<String>(&lit->value)) {
            return ConstValue::fromString(*pStr);
        }
    }
    
    // 标识符（查找已知常量）
    if (auto id = std::dynamic_pointer_cast<IdentifierExpr>(expr)) {
        auto it = constVars_.find(id->name);
        if (it != constVars_.end()) {
            return it->second;
        }
    }
    
    // 二元表达式
    if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        ConstValue left = tryEval(bin->left);
        ConstValue right = tryEval(bin->right);
        
        if (left.isConst() && right.isConst()) {
            return evalBinary(bin->op, left, right);
        }
    }
    
    // 一元表达式
    if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        ConstValue operand = tryEval(unary->operand);
        if (operand.isConst()) {
            return evalUnary(unary->op, operand);
        }
    }
    
    return ConstValue::nil();
}

// ═══════════════════════════════════════════════════════════════════
//  二元运算
// ═══════════════════════════════════════════════════════════════════

ConstValue ConstantFolder::evalBinary(TokenType op, const ConstValue& left, const ConstValue& right) {
    // 整数运算
    if (left.isInt() && right.isInt()) {
        Int64 a = left.intVal;
        Int64 b = right.intVal;
        
        switch (op) {
            case TokenType::OP_PLUS:  return ConstValue::fromInt(a + b);
            case TokenType::OP_MINUS: return ConstValue::fromInt(a - b);
            case TokenType::OP_MUL:   return ConstValue::fromInt(a * b);
            case TokenType::OP_DIV: 
                if (b != 0) return ConstValue::fromInt(a / b);
                break;
            case TokenType::OP_MOD:
                if (b != 0) return ConstValue::fromInt(a % b);
                break;
            case TokenType::OP_LT:    return ConstValue::fromBool(a < b);
            case TokenType::OP_GT:    return ConstValue::fromBool(a > b);
            case TokenType::OP_LE:    return ConstValue::fromBool(a <= b);
            case TokenType::OP_GE:    return ConstValue::fromBool(a >= b);
            case TokenType::OP_EQ:    return ConstValue::fromBool(a == b);
            case TokenType::OP_NE:    return ConstValue::fromBool(a != b);
            case TokenType::OP_AND:   return ConstValue::fromBool(a && b);
            case TokenType::OP_OR:    return ConstValue::fromBool(a || b);
            case TokenType::OP_BIT_AND:  return ConstValue::fromInt(a & b);
            case TokenType::OP_BIT_OR:   return ConstValue::fromInt(a | b);
            case TokenType::OP_BIT_XOR:  return ConstValue::fromInt(a ^ b);
            case TokenType::OP_LSHIFT:   return ConstValue::fromInt(a << b);
            case TokenType::OP_RSHIFT:   return ConstValue::fromInt(a >> b);
            default: break;
        }
    }
    
    // 浮点运算
    if (left.isNumber() && right.isNumber()) {
        double a = left.toNumber();
        double b = right.toNumber();
        
        switch (op) {
            case TokenType::OP_PLUS:  return ConstValue::fromFloat(a + b);
            case TokenType::OP_MINUS: return ConstValue::fromFloat(a - b);
            case TokenType::OP_MUL:  return ConstValue::fromFloat(a * b);
            case TokenType::OP_DIV:
                if (b != 0.0) return ConstValue::fromFloat(a / b);
                break;
            case TokenType::OP_LT:    return ConstValue::fromBool(a < b);
            case TokenType::OP_GT:    return ConstValue::fromBool(a > b);
            case TokenType::OP_LE:    return ConstValue::fromBool(a <= b);
            case TokenType::OP_GE:    return ConstValue::fromBool(a >= b);
            case TokenType::OP_EQ:    return ConstValue::fromBool(a == b);
            case TokenType::OP_NE:    return ConstValue::fromBool(a != b);
            default: break;
        }
    }
    
    // 布尔运算
    if (left.isBool() && right.isBool()) {
        bool a = left.boolVal;
        bool b = right.boolVal;
        
        switch (op) {
            case TokenType::OP_AND: return ConstValue::fromBool(a && b);
            case TokenType::OP_OR:  return ConstValue::fromBool(a || b);
            case TokenType::OP_EQ:  return ConstValue::fromBool(a == b);
            case TokenType::OP_NE:  return ConstValue::fromBool(a != b);
            default: break;
        }
    }
    
    // 字符串拼接
    if (left.isString() && right.isString()) {
        if (op == TokenType::OP_PLUS) {
            return ConstValue::fromString(left.stringVal + right.stringVal);
        }
    }
    
    return ConstValue::nil();
}

// ═══════════════════════════════════════════════════════════════════
//  一元运算
// ═══════════════════════════════════════════════════════════════════

ConstValue ConstantFolder::evalUnary(TokenType op, const ConstValue& operand) {
    if (operand.isInt()) {
        Int64 v = operand.intVal;
        switch (op) {
            case TokenType::OP_MINUS: return ConstValue::fromInt(-v);
            case TokenType::OP_NOT:   return ConstValue::fromBool(!v);
            default: break;
        }
    }
    
    if (operand.isFloat()) {
        double v = operand.floatVal;
        switch (op) {
            case TokenType::OP_MINUS: return ConstValue::fromFloat(-v);
            case TokenType::OP_NOT:   return ConstValue::fromBool(!v);
            default: break;
        }
    }
    
    if (operand.isBool()) {
        bool v = operand.boolVal;
        switch (op) {
            case TokenType::OP_NOT: return ConstValue::fromBool(!v);
            default: break;
        }
    }
    
    return ConstValue::nil();
}

// ═══════════════════════════════════════════════════════════════════
//  创建字面量
// ═══════════════════════════════════════════════════════════════════

Shared<Expr> ConstantFolder::makeLiteral(const ConstValue& val) {
    auto lit = std::make_shared<LiteralExpr>();
    
    switch (val.type) {
        case ConstValue::INT:
            lit->value = val.intVal;
            break;
        case ConstValue::FLOAT:
            lit->value = val.floatVal;
            break;
        case ConstValue::BOOL:
            lit->value = val.boolVal;
            break;
        case ConstValue::STRING:
            lit->value = val.stringVal;
            break;
        default:
            return nullptr;
    }
    
    return lit;
}

// ═══════════════════════════════════════════════════════════════════
//  折叠入口
// ═══════════════════════════════════════════════════════════════════

Shared<Expr> ConstantFolder::fold(Shared<Expr> expr) {
    if (!expr) return expr;
    
    // 尝试求值
    ConstValue val = tryEval(expr);
    
    if (val.isConst()) {
        // 可以折叠！
        Shared<Expr> lit = makeLiteral(val);
        if (lit) {
            foldCount_++;
            return lit;
        }
    }
    
    // 递归处理子表达式
    if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        bin->left = fold(bin->left);
        bin->right = fold(bin->right);
    } else if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        unary->operand = fold(unary->operand);
    }
    
    return expr;
}

Shared<Stmt> ConstantFolder::foldStmt(Shared<Stmt> stmt) {
    if (!stmt) return stmt;
    
    // 变量声明
    if (auto var = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) {
        if (var->init) {
            var->init = fold(var->init);
            
            // 如果是常量，记录下来
            if (var->isConst) {
                ConstValue val = tryEval(var->init);
                if (val.isConst()) {
                    constVars_[var->name] = val;
                }
            }
        }
    }
    
    // 表达式语句
    if (auto exprStmt = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        exprStmt->expr = fold(exprStmt->expr);
    }
    
    // if 语句
    if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        ifStmt->condition = fold(ifStmt->condition);
    }
    
    // while 语句
    if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        whileStmt->condition = fold(whileStmt->condition);
    }
    
    // return 语句
    if (auto ret = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        if (ret->value) {
            ret->value = fold(ret->value);
        }
    }
    
    return stmt;
}

} // namespace cplang
