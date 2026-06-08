#include "stdlib/stdlib.hpp"
#include <random>

namespace cplang {

// Math functions (abs, sqrt, pow, floor, ceil, round, random, etc.)
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerMath(VM* vm) {
    registerFunction(vm, "abs", math::abs);
    registerFunction(vm, "sqrt", math::sqrt);
    registerFunction(vm, "pow", math::pow);
    registerFunction(vm, "floor", math::floor);
    registerFunction(vm, "ceil", math::ceil);
    registerFunction(vm, "round", math::round);
    registerFunction(vm, "random", math::random);
    registerFunction(vm, "max", math::max);
    registerFunction(vm, "min", math::min);
    registerFunction(vm, "sin", math::sin);
    registerFunction(vm, "cos", math::cos);
    registerFunction(vm, "tan", math::tan);
    registerFunction(vm, "log", math::log);
    registerFunction(vm, "log10", math::log10);
    registerFunction(vm, "exp", math::exp);
    registerFunction(vm, "pi", math::pi);
    registerFunction(vm, "e", math::e);
    
    // 中文别名
    registerAlias(vm, "绝对值", "abs");
    registerAlias(vm, "平方根", "sqrt");
    registerAlias(vm, "幂", "pow");
    registerAlias(vm, "向下取整", "floor");
    registerAlias(vm, "向上取整", "ceil");
    registerAlias(vm, "四舍五入", "round");
    registerAlias(vm, "随机", "random");
    registerAlias(vm, "正弦", "sin");
    registerAlias(vm, "余弦", "cos");
    registerAlias(vm, "正切", "tan");
    registerAlias(vm, "自然对数", "log");
    registerAlias(vm, "圆周率", "pi");
    registerAlias(vm, "自然常数", "e");
    registerFunction(vm, "asin", math::asin);
    registerAlias(vm, "反正弦", "asin");
    registerFunction(vm, "acos", math::acos);
    registerAlias(vm, "反余弦", "acos");
    registerFunction(vm, "atan", math::atan);
    registerAlias(vm, "反正切", "atan");
    registerFunction(vm, "atan2", math::atan2);
    registerAlias(vm, "反正切2", "atan2");
}

Value math::abs(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    if (args[0].isInt()) return Value::Int(std::llabs(args[0].asInt()));
    if (args[0].isFloat()) return Value::Float(std::fabs(args[0].asFloat()));
    return Value::nil();
}

Value math::sqrt(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    double x = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    return Value::Float(std::sqrt(x));
}

Value math::pow(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    double base = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    double exp = args[1].isInt() ? static_cast<double>(args[1].asInt()) : args[1].asFloat();
    return Value::Float(std::pow(base, exp));
}

Value math::floor(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    double x = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    return Value::Int(static_cast<Int64>(std::floor(x)));
}

Value math::ceil(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    double x = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    return Value::Int(static_cast<Int64>(std::ceil(x)));
}

Value math::round(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    double x = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    return Value::Int(static_cast<Int64>(std::round(x)));
}

Value math::random(std::vector<Value>& args) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    if (args.empty()) {
        // 返回 0-1 之间的随机浮点数
        std::uniform_real_distribution<> dis(0.0, 1.0);
        return Value::Float(dis(gen));
    }
    
    if (args.size() == 1) {
        // 返回 0-n 之间的随机整数
        Int64 n = args[0].asInt();
        std::uniform_int_distribution<Int64> dis(0, n - 1);
        return Value::Int(dis(gen));
    }
    
    // 返回 min-max 之间的随机整数
    Int64 min = args[0].asInt();
    Int64 max = args[1].asInt();
    std::uniform_int_distribution<Int64> dis(min, max);
    return Value::Int(dis(gen));
}

Value math::max(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    Value maxVal = args[0];
    for (size_t i = 1; i < args.size(); i++) {
        if (args[i].isInt() && maxVal.isInt()) {
            if (args[i].asInt() > maxVal.asInt()) maxVal = args[i];
        } else {
            double a = args[i].isInt() ? static_cast<double>(args[i].asInt()) : args[i].asFloat();
            double b = maxVal.isInt() ? static_cast<double>(maxVal.asInt()) : maxVal.asFloat();
            if (a > b) maxVal = args[i];
        }
    }
    return maxVal;
}

Value math::min(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    Value minVal = args[0];
    for (size_t i = 1; i < args.size(); i++) {
        if (args[i].isInt() && minVal.isInt()) {
            if (args[i].asInt() < minVal.asInt()) minVal = args[i];
        } else {
            double a = args[i].isInt() ? static_cast<double>(args[i].asInt()) : args[i].asFloat();
            double b = minVal.isInt() ? static_cast<double>(minVal.asInt()) : minVal.asFloat();
            if (a < b) minVal = args[i];
        }
    }
    return minVal;
}

Value math::sin(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    double x = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    return Value::Float(std::sin(x));
}

Value math::cos(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    double x = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    return Value::Float(std::cos(x));
}

Value math::tan(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    double x = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    return Value::Float(std::tan(x));
}

Value math::log(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    double x = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    return Value::Float(std::log(x));
}

Value math::log10(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    double x = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    return Value::Float(std::log10(x));
}

Value math::exp(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    double x = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    return Value::Float(std::exp(x));
}

Value math::pi(std::vector<Value>& /*args*/) {
    return Value::Float(3.14159265358979323846);
}

Value math::e(std::vector<Value>& /*args*/) {
    return Value::Float(2.71828182845904523536);
}

Value math::asin(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    double x = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    return Value::Float(std::asin(x));
}

Value math::acos(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    double x = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    return Value::Float(std::acos(x));
}

Value math::atan(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    double x = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    return Value::Float(std::atan(x));
}

Value math::atan2(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    double y = args[0].isInt() ? static_cast<double>(args[0].asInt()) : args[0].asFloat();
    double x = args[1].isInt() ? static_cast<double>(args[1].asInt()) : args[1].asFloat();
    return Value::Float(std::atan2(y, x));
}

} // namespace cplang
