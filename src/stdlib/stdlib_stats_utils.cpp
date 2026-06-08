#include "stdlib/stdlib.hpp"
#include <random>
#include <algorithm>

namespace cplang {

// MathMore, Statistics, Utils, StringCase, MoreUtils functions
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerMathMore(VM* vm) {
    registerFunction(vm, "asin", math_more::asin);
    registerFunction(vm, "acos", math_more::acos);
    registerFunction(vm, "atan", math_more::atan);
    registerFunction(vm, "atan2", math_more::atan2);
    registerFunction(vm, "sinh", math_more::sinh);
    registerFunction(vm, "cosh", math_more::cosh);
    registerFunction(vm, "tanh", math_more::tanh);
    registerFunction(vm, "asinh", math_more::asinh);
    registerFunction(vm, "acosh", math_more::acosh);
    registerFunction(vm, "atanh", math_more::atanh);
    registerFunction(vm, "log10", math_more::log10);
    registerFunction(vm, "log2", math_more::log2);
    registerFunction(vm, "exp", math_more::exp);
    registerFunction(vm, "exp2", math_more::exp2);
    registerFunction(vm, "expm1", math_more::expm1);
    registerFunction(vm, "cbrt", math_more::cbrt);
    registerFunction(vm, "hypot", math_more::hypot);
    registerFunction(vm, "ceil", math_more::ceil);
    registerFunction(vm, "floor", math_more::floor);
    registerFunction(vm, "round", math_more::round);
    registerFunction(vm, "trunc", math_more::trunc);
    registerFunction(vm, "fabs", math_more::fabs);
    registerFunction(vm, "fmod", math_more::fmod);
    registerFunction(vm, "remainder", math_more::remainder);
    registerFunction(vm, "copysign", math_more::copysign);
    registerFunction(vm, "fma", math_more::fma);
    registerFunction(vm, "ln", math_more::ln);
    registerFunction(vm, "factorial", math_more::factorial);
    registerFunction(vm, "sign", math_more::sign);
    registerFunction(vm, "deg2rad", math_more::deg2rad);
    registerFunction(vm, "rad2deg", math_more::rad2deg);

    registerAlias(vm, "反正弦", "asin");
    registerAlias(vm, "反余弦", "acos");
    registerAlias(vm, "反正切", "atan");
    registerAlias(vm, "反正切2", "atan2");
    registerAlias(vm, "双曲正弦", "sinh");
    registerAlias(vm, "双曲余弦", "cosh");
    registerAlias(vm, "双曲正切", "tanh");
    registerAlias(vm, "反双曲正弦", "asinh");
    registerAlias(vm, "反双曲余弦", "acosh");
    registerAlias(vm, "反双曲正切", "atanh");
    registerAlias(vm, "常用对数", "log10");
    registerAlias(vm, "二进制对数", "log2");
    registerAlias(vm, "自然指数", "exp");
    registerAlias(vm, "二进制指数", "exp2");
    registerAlias(vm, "指数减一", "expm1");
    registerAlias(vm, "立方根", "cbrt");
    registerAlias(vm, "斜边", "hypot");
    registerAlias(vm, "向上取整", "ceil");
    registerAlias(vm, "向下取整", "floor");
    registerAlias(vm, "四舍五入", "round");
    registerAlias(vm, "截断", "trunc");
    registerAlias(vm, "绝对值浮点", "fabs");
    registerAlias(vm, "浮点取余", "fmod");
    registerAlias(vm, "余数", "remainder");
    registerAlias(vm, "复制符号", "copysign");
    registerAlias(vm, "乘加", "fma");
    registerAlias(vm, "自然对数", "ln");
    registerAlias(vm, "阶乘", "factorial");
    registerAlias(vm, "符号", "sign");
    registerAlias(vm, "度转弧度", "deg2rad");
    registerAlias(vm, "弧度转度", "rad2deg");
}

namespace math_more {

static double getNum(const Value& v) {
    if (v.isInt()) return static_cast<double>(v.asInt());
    if (v.isFloat()) return v.asFloat();
    return 0.0;
}

Value asin(std::vector<Value>& args) { return Value::Float(std::asin(getNum(args.empty() ? Value::nil() : args[0]))); }
Value acos(std::vector<Value>& args) { return Value::Float(std::acos(getNum(args.empty() ? Value::nil() : args[0]))); }
Value atan(std::vector<Value>& args) { return Value::Float(std::atan(getNum(args.empty() ? Value::nil() : args[0]))); }
Value atan2(std::vector<Value>& args) { return Value::Float(std::atan2(getNum(args.size() < 1 ? Value::nil() : args[0]), getNum(args.size() < 2 ? Value::nil() : args[1]))); }
Value sinh(std::vector<Value>& args) { return Value::Float(std::sinh(getNum(args.empty() ? Value::nil() : args[0]))); }
Value cosh(std::vector<Value>& args) { return Value::Float(std::cosh(getNum(args.empty() ? Value::nil() : args[0]))); }
Value tanh(std::vector<Value>& args) { return Value::Float(std::tanh(getNum(args.empty() ? Value::nil() : args[0]))); }
Value asinh(std::vector<Value>& args) { return Value::Float(std::asinh(getNum(args.empty() ? Value::nil() : args[0]))); }
Value acosh(std::vector<Value>& args) { return Value::Float(std::acosh(getNum(args.empty() ? Value::nil() : args[0]))); }
Value atanh(std::vector<Value>& args) { return Value::Float(std::atanh(getNum(args.empty() ? Value::nil() : args[0]))); }
Value log10(std::vector<Value>& args) { return Value::Float(std::log10(getNum(args.empty() ? Value::nil() : args[0]))); }
Value log2(std::vector<Value>& args) { return Value::Float(std::log2(getNum(args.empty() ? Value::nil() : args[0]))); }
Value exp(std::vector<Value>& args) { return Value::Float(std::exp(getNum(args.empty() ? Value::nil() : args[0]))); }
Value exp2(std::vector<Value>& args) { return Value::Float(std::exp2(getNum(args.empty() ? Value::nil() : args[0]))); }
Value expm1(std::vector<Value>& args) { return Value::Float(std::expm1(getNum(args.empty() ? Value::nil() : args[0]))); }
Value cbrt(std::vector<Value>& args) { return Value::Float(std::cbrt(getNum(args.empty() ? Value::nil() : args[0]))); }
Value hypot(std::vector<Value>& args) { return Value::Float(std::hypot(getNum(args.size() < 1 ? Value::nil() : args[0]), getNum(args.size() < 2 ? Value::nil() : args[1]))); }
Value ceil(std::vector<Value>& args) { return Value::Float(std::ceil(getNum(args.empty() ? Value::nil() : args[0]))); }
Value floor(std::vector<Value>& args) { return Value::Float(std::floor(getNum(args.empty() ? Value::nil() : args[0]))); }
Value round(std::vector<Value>& args) { return Value::Float(std::round(getNum(args.empty() ? Value::nil() : args[0]))); }
Value trunc(std::vector<Value>& args) { return Value::Float(std::trunc(getNum(args.empty() ? Value::nil() : args[0]))); }
Value fabs(std::vector<Value>& args) { return Value::Float(std::fabs(getNum(args.empty() ? Value::nil() : args[0]))); }
Value fmod(std::vector<Value>& args) { return Value::Float(std::fmod(getNum(args.size() < 1 ? Value::nil() : args[0]), getNum(args.size() < 2 ? Value::nil() : args[1]))); }
Value remainder(std::vector<Value>& args) { return Value::Float(std::remainder(getNum(args.size() < 1 ? Value::nil() : args[0]), getNum(args.size() < 2 ? Value::nil() : args[1]))); }
Value copysign(std::vector<Value>& args) { return Value::Float(std::copysign(getNum(args.size() < 1 ? Value::nil() : args[0]), getNum(args.size() < 2 ? Value::nil() : args[1]))); }
Value fma(std::vector<Value>& args) { return Value::Float(std::fma(getNum(args.size() < 1 ? Value::nil() : args[0]), getNum(args.size() < 2 ? Value::nil() : args[1]), getNum(args.size() < 3 ? Value::nil() : args[2]))); }

Value ln(std::vector<Value>& args) {
    double x = getNum(args.size() < 1 ? Value::nil() : args[0]);
    if (x <= 0) return Value::Float(0);
    return Value::Float(std::log(x));
}

Value factorial(std::vector<Value>& args) {
    int n = static_cast<int>(getNum(args.size() < 1 ? Value::nil() : args[0]));
    if (n < 0) return Value::Float(0);
    double result = 1.0;
    for (int i = 2; i <= n; i++) result *= i;
    return Value::Float(result);
}

Value sign(std::vector<Value>& args) {
    double x = getNum(args.size() < 1 ? Value::nil() : args[0]);
    if (x > 0) return Value::Float(1);
    if (x < 0) return Value::Float(-1);
    return Value::Float(0);
}

Value deg2rad(std::vector<Value>& args) {
    double d = getNum(args.size() < 1 ? Value::nil() : args[0]);
    return Value::Float(d * 3.14159265358979323846 / 180.0);
}

Value rad2deg(std::vector<Value>& args) {
    double r = getNum(args.size() < 1 ? Value::nil() : args[0]);
    return Value::Float(r * 180.0 / 3.14159265358979323846);
}

} // namespace math_more

// ═══════════════════════════════════════════════════════════════════
//  统计函数实现
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerStatistics(VM* vm) {
    registerFunction(vm, "mean", statistics::mean);
    registerFunction(vm, "median", statistics::median);
    registerFunction(vm, "variance", statistics::variance);
    registerFunction(vm, "stddev", statistics::stddev);
    registerFunction(vm, "mode", statistics::mode);
    registerFunction(vm, "percentile", statistics::percentile);
    registerFunction(vm, "min", statistics::min);
    registerFunction(vm, "max", statistics::max);
    registerFunction(vm, "range", statistics::range);

    registerAlias(vm, "均值", "mean");
    registerAlias(vm, "中位数", "median");
    registerAlias(vm, "方差", "variance");
    registerAlias(vm, "标准差", "stddev");
    registerAlias(vm, "众数", "mode");
    registerAlias(vm, "百分位数", "percentile");
    registerAlias(vm, "最小值", "min");
    registerAlias(vm, "最大值", "max");
    registerAlias(vm, "极差", "range");
    
    registerFunction(vm, "correlation", statistics::correlation);
    registerFunction(vm, "covariance", statistics::covariance);
    registerFunction(vm, "linearRegression", statistics::linearRegression);
    
    registerAlias(vm, "相关系数", "correlation");
    registerAlias(vm, "协方差", "covariance");
    registerAlias(vm, "线性回归", "linearRegression");
}

namespace statistics {

static double getNum(const Value& v) {
    if (v.isInt()) return static_cast<double>(v.asInt());
    if (v.isFloat()) return v.asFloat();
    return 0.0;
}

static std::vector<double> extractNumbers(VMArray* arr) {
    std::vector<double> nums;
    for (size_t i = 0; i < arr->data.size(); i++) {
        if (arr->data[i].isNumber()) nums.push_back(getNum(arr->data[i]));
    }
    return nums;
}

Value mean(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Float(0);
    auto nums = extractNumbers(args[0].asArray());
    if (nums.empty()) return Value::Float(0);
    double sum = 0;
    for (double n : nums) sum += n;
    return Value::Float(sum / nums.size());
}

Value median(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Float(0);
    auto nums = extractNumbers(args[0].asArray());
    if (nums.empty()) return Value::Float(0);
    std::sort(nums.begin(), nums.end());
    size_t n = nums.size();
    if (n % 2 == 1) return Value::Float(nums[n / 2]);
    return Value::Float((nums[n / 2 - 1] + nums[n / 2]) / 2.0);
}

Value variance(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Float(0);
    auto nums = extractNumbers(args[0].asArray());
    if (nums.size() < 2) return Value::Float(0);
    double m = 0;
    for (double n : nums) m += n;
    m /= nums.size();
    double sumSq = 0;
    for (double n : nums) sumSq += (n - m) * (n - m);
    return Value::Float(sumSq / nums.size());
}

Value stddev(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Float(0);
    auto nums = extractNumbers(args[0].asArray());
    if (nums.size() < 2) return Value::Float(0);
    double m = 0;
    for (double n : nums) m += n;
    m /= nums.size();
    double sumSq = 0;
    for (double n : nums) sumSq += (n - m) * (n - m);
    return Value::Float(std::sqrt(sumSq / nums.size()));
}

Value mode(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    auto nums = extractNumbers(args[0].asArray());
    if (nums.empty()) return Value::nil();
    std::unordered_map<double, int> freq;
    for (double n : nums) freq[n]++;
    double maxVal = nums[0];
    int maxFreq = 0;
    for (auto& p : freq) {
        if (p.second > maxFreq) { maxFreq = p.second; maxVal = p.first; }
    }
    return Value::Float(maxVal);
}

Value percentile(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isNumber()) return Value::Float(0);
    auto nums = extractNumbers(args[0].asArray());
    if (nums.empty()) return Value::Float(0);
    double p = getNum(args[1]);
    if (p < 0) p = 0; if (p > 100) p = 100;
    std::sort(nums.begin(), nums.end());
    if (p == 0) return Value::Float(nums.front());
    if (p == 100) return Value::Float(nums.back());
    double idx = (p / 100.0) * (nums.size() - 1);
    size_t lower = static_cast<size_t>(idx);
    size_t upper = lower + 1;
    double frac = idx - lower;
    if (upper >= nums.size()) return Value::Float(nums[lower]);
    return Value::Float(nums[lower] * (1 - frac) + nums[upper] * frac);
}

Value min(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Float(0);
    auto nums = extractNumbers(args[0].asArray());
    if (nums.empty()) return Value::Float(0);
    return Value::Float(*std::min_element(nums.begin(), nums.end()));
}

Value max(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Float(0);
    auto nums = extractNumbers(args[0].asArray());
    if (nums.empty()) return Value::Float(0);
    return Value::Float(*std::max_element(nums.begin(), nums.end()));
}

Value range(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::Float(0);
    auto nums = extractNumbers(args[0].asArray());
    if (nums.empty()) return Value::Float(0);
    auto [mn, mx] = std::minmax_element(nums.begin(), nums.end());
    return Value::Float(*mx - *mn);
}

Value correlation(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::Float(0);
    auto x = extractNumbers(args[0].asArray());
    auto y = extractNumbers(args[1].asArray());
    if (x.empty() || y.empty() || x.size() != y.size()) return Value::Float(0);
    size_t n = x.size();
    double mx = 0, my = 0;
    for (size_t i = 0; i < n; i++) { mx += x[i]; my += y[i]; }
    mx /= n; my /= n;
    double sx = 0, sy = 0, cov = 0;
    for (size_t i = 0; i < n; i++) {
        double dx = x[i] - mx, dy = y[i] - my;
        sx += dx * dx; sy += dy * dy; cov += dx * dy;
    }
    if (sx == 0 || sy == 0) return Value::Float(0);
    return Value::Float(cov / std::sqrt(sx * sy));
}

Value covariance(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) return Value::Float(0);
    auto x = extractNumbers(args[0].asArray());
    auto y = extractNumbers(args[1].asArray());
    if (x.empty() || y.empty() || x.size() != y.size()) return Value::Float(0);
    size_t n = x.size();
    double mx = 0, my = 0;
    for (size_t i = 0; i < n; i++) { mx += x[i]; my += y[i]; }
    mx /= n; my /= n;
    double cov = 0;
    for (size_t i = 0; i < n; i++) cov += (x[i] - mx) * (y[i] - my);
    return Value::Float(cov / n);
}

Value linearRegression(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isArray() || !args[1].isArray()) {
        auto t = VMTable::create();
        return Value::Table(t);
    }
    auto xv = extractNumbers(args[0].asArray());
    auto yv = extractNumbers(args[1].asArray());
    if (xv.empty() || yv.empty() || xv.size() != yv.size()) {
        auto t = VMTable::create();
        return Value::Table(t);
    }
    size_t n = xv.size();
    double mx = 0, my = 0;
    for (size_t i = 0; i < n; i++) { mx += xv[i]; my += yv[i]; }
    mx /= n; my /= n;
    double sx = 0, sy = 0, sxy = 0;
    for (size_t i = 0; i < n; i++) {
        double dx = xv[i] - mx, dy = yv[i] - my;
        sx += dx * dx; sy += dy * dy; sxy += dx * dy;
    }
    if (sx == 0) {
        auto t = VMTable::create();
        return Value::Table(t);
    }
    double slope = sxy / sx;
    double intercept = my - slope * mx;
    double r = sxy / std::sqrt(sx * sy);
    auto t = VMTable::create();
    t->set(Value::String(VMString::create("slope")), Value::Float(slope));
    t->set(Value::String(VMString::create("intercept")), Value::Float(intercept));
    t->set(Value::String(VMString::create("r")), Value::Float(r));
    return Value::Table(t);
}
} // namespace statistics

// ═══════════════════════════════════════════════════════════════════
//  实用工具函数实现
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerUtils(VM* vm) {
    registerFunction(vm, "clamp", utils::clamp);
    registerFunction(vm, "lerp", utils::lerp);
    registerFunction(vm, "map", utils::map);
    registerFunction(vm, "randomInt", utils::randomInt);
    registerFunction(vm, "randomChoice", utils::randomChoice);
    registerFunction(vm, "isEmpty", utils::isEmpty);
    registerFunction(vm, "defaultVal", utils::defaultVal);

    registerAlias(vm, "限制", "clamp");
    registerAlias(vm, "插值", "lerp");
    registerAlias(vm, "映射", "map");
    registerAlias(vm, "随机整数", "randomInt");
    registerAlias(vm, "随机选择", "randomChoice");
    registerAlias(vm, "是否为空", "isEmpty");
    registerAlias(vm, "默认值", "defaultVal");
}

namespace utils {

static double getNum(const Value& v) {
    if (v.isInt()) return static_cast<double>(v.asInt());
    if (v.isFloat()) return v.asFloat();
    return 0.0;
}

Value clamp(std::vector<Value>& args) {
    if (args.size() < 3) return Value::Float(0);
    double v = getNum(args[0]), lo = getNum(args[1]), hi = getNum(args[2]);
    if (v < lo) return Value::Float(lo);
    if (v > hi) return Value::Float(hi);
    return Value::Float(v);
}

Value lerp(std::vector<Value>& args) {
    if (args.size() < 3) return Value::Float(0);
    double a = getNum(args[0]), b = getNum(args[1]), t = getNum(args[2]);
    return Value::Float(a + (b - a) * t);
}

Value map(std::vector<Value>& args) {
    if (args.size() < 5) return Value::Float(0);
    double v = getNum(args[0]), inMin = getNum(args[1]), inMax = getNum(args[2]);
    double outMin = getNum(args[3]), outMax = getNum(args[4]);
    if (inMax == inMin) return Value::Float(outMin);
    return Value::Float(outMin + (v - inMin) * (outMax - outMin) / (inMax - inMin));
}

Value randomInt(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isNumber() || !args[1].isNumber()) return Value::Float(0);
    int lo = static_cast<int>(getNum(args[0]));
    int hi = static_cast<int>(getNum(args[1]));
    if (lo > hi) std::swap(lo, hi);
    static std::mt19937 gen(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<> dis(lo, hi);
    return Value::Float(static_cast<double>(dis(gen)));
}

Value randomChoice(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    VMArray* arr = args[0].asArray();
    if (arr->data.empty()) return Value::nil();
    static std::mt19937 gen(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<> dis(0, static_cast<int>(arr->data.size()) - 1);
    return arr->data[dis(gen)];
}

Value isEmpty(std::vector<Value>& args) {
    if (args.empty()) return Value::Bool(true);
    if (args[0].isString()) return Value::Bool(args[0].asString()->length == 0);
    if (args[0].isArray()) return Value::Bool(args[0].asArray()->data.empty());
    if (args[0].isTable()) return Value::Bool(args[0].asTable()->size() == 0);
    if (args[0].isNil()) return Value::Bool(true);
    return Value::Bool(false);
}

Value defaultVal(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    if (args.size() >= 2) {
        if (args[0].isNil()) return args[1];
    }
    return args[0];
}
} // namespace utils

// ═══════════════════════════════════════════════════════════════════
//  字符串大小写与字符判断实现
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerStringCase(VM* vm) {
    registerFunction(vm, "strToUpper", string_case::strToUpper);
    registerFunction(vm, "strToLower", string_case::strToLower);
    registerFunction(vm, "strIsDigit", string_case::strIsDigit);
    registerFunction(vm, "strIsAlpha", string_case::strIsAlpha);
    registerFunction(vm, "strIsAlnum", string_case::strIsAlnum);
    registerFunction(vm, "strIsSpace", string_case::strIsSpace);
    registerFunction(vm, "strCapitalize", string_case::strCapitalize);
    registerFunction(vm, "strTitle", string_case::strTitle);

    registerAlias(vm, "转大写", "strToUpper");
    registerAlias(vm, "转小写", "strToLower");
    registerAlias(vm, "是否数字", "strIsDigit");
    registerAlias(vm, "是否字母", "strIsAlpha");
    registerAlias(vm, "是否字母数字", "strIsAlnum");
    registerAlias(vm, "是否空白", "strIsSpace");
    registerAlias(vm, "首字母大写", "strCapitalize");
    registerAlias(vm, "标题化", "strTitle");
}

namespace string_case {

static std::string getStr(const Value& v) {
    if (!v.isString()) return "";
    return std::string(v.asString()->data, v.asString()->length);
}

Value strToUpper(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string s = getStr(args[0]);
    std::string result;
    for (unsigned char c : s) result += static_cast<char>(std::toupper(c));
    return Value::String(VMString::create(result));
}

Value strToLower(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string s = getStr(args[0]);
    std::string result;
    for (unsigned char c : s) result += static_cast<char>(std::tolower(c));
    return Value::String(VMString::create(result));
}

Value strIsDigit(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    std::string s = getStr(args[0]);
    for (unsigned char c : s) if (!std::isdigit(c)) return Value::Bool(false);
    return Value::Bool(!s.empty());
}

Value strIsAlpha(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    std::string s = getStr(args[0]);
    for (unsigned char c : s) if (!std::isalpha(c)) return Value::Bool(false);
    return Value::Bool(!s.empty());
}

Value strIsAlnum(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    std::string s = getStr(args[0]);
    for (unsigned char c : s) if (!std::isalnum(c)) return Value::Bool(false);
    return Value::Bool(!s.empty());
}

Value strIsSpace(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    std::string s = getStr(args[0]);
    for (unsigned char c : s) if (!std::isspace(c)) return Value::Bool(false);
    return Value::Bool(!s.empty());
}

Value strCapitalize(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string s = getStr(args[0]);
    if (!s.empty()) s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    return Value::String(VMString::create(s));
}

Value strTitle(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string s = getStr(args[0]);
    bool newWord = true;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (std::isspace(c)) { newWord = true; }
        else if (newWord) { s[i] = static_cast<char>(std::toupper(c)); newWord = false; }
        else { s[i] = static_cast<char>(std::tolower(c)); }
    }
    return Value::String(VMString::create(s));
}
} // namespace string_case

// ═══════════════════════════════════════════════════════════════════
//  更多实用工具实现
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerMoreUtils(VM* vm) {
    registerFunction(vm, "arrShuffle", more_utils::arrShuffle);
    registerFunction(vm, "strRepeat", more_utils::strRepeat);
    registerFunction(vm, "deepEqual", more_utils::deepEqual);
    registerFunction(vm, "shallowClone", more_utils::shallowClone);
    registerFunction(vm, "typeOf", more_utils::typeOf);
    registerFunction(vm, "assert", more_utils::assertFn);
    registerFunction(vm, "benchmark", more_utils::benchmark);
    registerFunction(vm, "levenshtein", more_utils::levenshtein);
    registerFunction(vm, "uuid4", more_utils::uuid4);
    registerFunction(vm, "csvParse", more_utils::csvParse);
    registerFunction(vm, "strFormat", more_utils::strFormat);
    registerFunction(vm, "globMatch", more_utils::globMatch);
    // 移植自 stdlib_linux.cpp 的缺失函数
    registerFunction(vm, "toSnakeCase", more_utils::toSnakeCase);
    registerFunction(vm, "toCamelCase", more_utils::toCamelCase);
    registerFunction(vm, "uniq", more_utils::uniq);
    registerFunction(vm, "enumerate", more_utils::enumerate);
    registerFunction(vm, "arrSum", more_utils::arrSum);
    registerFunction(vm, "arrAvg", more_utils::arrAvg);
    registerFunction(vm, "arrTake", more_utils::arrTake);
    registerFunction(vm, "arrDrop", more_utils::arrDrop);
    registerFunction(vm, "intPow", more_utils::intPow);
    registerFunction(vm, "roundTo", more_utils::roundTo);
    registerFunction(vm, "merge", more_utils::merge);
    registerFunction(vm, "getOrDefault", more_utils::getOrDefault);
    registerFunction(vm, "swap", more_utils::swap);
    registerFunction(vm, "contains", more_utils::contains);
    registerFunction(vm, "intersection", more_utils::intersection);
    registerFunction(vm, "difference", more_utils::difference);
    registerFunction(vm, "strCount", more_utils::strCount);
    registerFunction(vm, "strCompareIC", more_utils::strCompareIC);
    registerFunction(vm, "strIsBlank", more_utils::strIsBlank);
    registerFunction(vm, "timestamp", more_utils::timestamp);
    registerFunction(vm, "clock", more_utils::clock);
    registerFunction(vm, "accumulate", more_utils::accumulate);
    registerFunction(vm, "product", more_utils::product);
    registerFunction(vm, "anyOf", more_utils::anyOf);
    registerFunction(vm, "allOf", more_utils::allOf);
    registerFunction(vm, "noneOf", more_utils::noneOf);
    registerFunction(vm, "erf", more_utils::erfFunc);
    registerFunction(vm, "tgamma", more_utils::tgammaFunc);

    registerAlias(vm, "数组洗牌", "arrShuffle");
    registerAlias(vm, "重复", "strRepeat");
    registerAlias(vm, "深度相等", "deepEqual");
    registerAlias(vm, "浅拷贝", "shallowClone");
    registerAlias(vm, "类型", "typeOf");
    registerAlias(vm, "断言", "assert");
    registerAlias(vm, "基准测试", "benchmark");
    registerAlias(vm, "编辑距离", "levenshtein");
    registerAlias(vm, "UUID生成", "uuid4");
    registerAlias(vm, "CSV解析", "csvParse");
    registerAlias(vm, "格式化", "strFormat");
    registerAlias(vm, "通配匹配", "globMatch");
}

namespace more_utils {

static std::string getStr(const Value& v) {
    if (!v.isString()) return "";
    return std::string(v.asString()->data, v.asString()->length);
}

Value arrShuffle(std::vector<Value>& args) {
    if (args.empty() || !args[0].isArray()) return Value::nil();
    VMArray* arr = args[0].asArray();
    static std::mt19937 gen(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::shuffle(arr->data.begin(), arr->data.end(), gen);
    return args[0];
}

Value strRepeat(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isNumber()) return Value::nil();
    std::string s = getStr(args[0]);
    int count = static_cast<int>(args[1].asFloat());
    if (count < 0) count = 0;
    std::string result;
    for (int i = 0; i < count; i++) result += s;
    return Value::String(VMString::create(result));
}

static bool deepEqualValue(const Value& a, const Value& b) {
    if (a.tag != b.tag) return false;
    if (a.isNil()) return true;
    if (a.isBool()) return a.asInt() == b.asInt();
    if (a.isInt()) return a.asInt() == b.asInt();
    if (a.isFloat()) return a.asFloat() == b.asFloat();
    if (a.isString()) {
        if (a.asString()->length != b.asString()->length) return false;
        return std::memcmp(a.asString()->data, b.asString()->data, a.asString()->length) == 0;
    }
    if (a.isArray()) {
        auto* aa = a.asArray(); auto* ba = b.asArray();
        if (aa->data.size() != ba->data.size()) return false;
        for (size_t i = 0; i < aa->data.size(); i++) {
            if (!deepEqualValue(aa->data[i], ba->data[i])) return false;
        }
        return true;
    }
    if (a.isTable()) {
        auto* ta = a.asTable(); auto* tb = b.asTable();
        if (ta->size() != tb->size()) return false;
        for (auto& pa : ta->data) {
            bool found = false;
            for (auto& pb : tb->data) {
                if (deepEqualValue(pa.first, pb.first) && deepEqualValue(pa.second, pb.second)) { found = true; break; }
            }
            if (!found) return false;
        }
        return true;
    }
    return a.equals(b);
}

Value deepEqual(std::vector<Value>& args) {
    if (args.size() < 2) return Value::Bool(false);
    return Value::Bool(deepEqualValue(args[0], args[1]));
}

Value shallowClone(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    if (args[0].isArray()) {
        VMArray* src = args[0].asArray();
        VMArray* dst = VMArray::create(0);
        dst->data = src->data;
        return Value::Array(dst);
    }
    if (args[0].isTable()) {
        VMTable* src = args[0].asTable();
        VMTable* dst = VMTable::create();
        dst->data = src->data;
        dst->buckets = src->buckets;
        dst->count = src->count;
        return Value::Table(dst);
    }
    return args[0];
}

Value typeOf(std::vector<Value>& args) {
    if (args.empty()) return Value::String(VMString::create("nil"));
    auto& v = args[0];
    if (v.isNil()) return Value::String(VMString::create("nil"));
    if (v.isBool()) return Value::String(VMString::create("bool"));
    if (v.isInt()) return Value::String(VMString::create("int"));
    if (v.isFloat()) return Value::String(VMString::create("float"));
    if (v.isString()) return Value::String(VMString::create("string"));
    if (v.isArray()) return Value::String(VMString::create("array"));
    if (v.isTable()) return Value::String(VMString::create("table"));
    if (v.isSet()) return Value::String(VMString::create("set"));
    if (v.isStack()) return Value::String(VMString::create("stack"));
    if (v.isQueue()) return Value::String(VMString::create("queue"));
    if (v.isFunction()) return Value::String(VMString::create("function"));
    return Value::String(VMString::create("object"));
}

Value assertFn(std::vector<Value>& args) {
    if (args.empty() || !args[0].isTrue()) {
        std::string msg = (args.size() >= 2 && args[1].isString()) ? getStr(args[1]) : "Assertion failed";
        std::cerr << "ASSERT: " << msg << std::endl;
        return Value::Bool(false);
    }
    return Value::Bool(true);
}

Value benchmark(std::vector<Value>& args) {
    if (args.empty() || !args[0].isFunction()) return Value::nil();
    auto start = std::chrono::steady_clock::now();
    // 无法直接调用 CP 函数，返回时间戳让用户自行比较
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count();
    return Value::Float(static_cast<double>(ms));
}

Value levenshtein(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Int(0);
    std::string a(args[0].asString()->data, args[0].asString()->length);
    std::string b(args[1].asString()->data, args[1].asString()->length);
    size_t m = a.size(), n = b.size();
    std::vector<size_t> prev(n + 1), cur(n + 1);
    for (size_t j = 0; j <= n; j++) prev[j] = j;
    for (size_t i = 1; i <= m; i++) {
        cur[0] = i;
        for (size_t j = 1; j <= n; j++) {
            size_t cost = (a[i-1] == b[j-1]) ? 0 : 1;
            cur[j] = std::min({prev[j] + 1, cur[j-1] + 1, prev[j-1] + cost});
        }
        prev.swap(cur);
    }
    return Value::Int(static_cast<Int64>(prev[n]));
}

Value uuid4(std::vector<Value>& args) {
    static std::mt19937 gen(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
    static std::uniform_int_distribution<unsigned> dist(0, 15);
    const char* hex = "0123456789abcdef";
    char buf[37];
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) buf[i] = '-';
        else if (i == 14) buf[i] = '4';
        else if (i == 19) buf[i] = hex[(8 + dist(gen)) % 16];
        else buf[i] = hex[dist(gen)];
    }
    buf[36] = '\0';
    return Value::String(VMString::create(std::string(buf)));
}

Value csvParse(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Array(VMArray::create());
    std::string input(args[0].asString()->data, args[0].asString()->length);
    VMArray* rows = VMArray::create();
    VMArray* row = VMArray::create();
    std::string field;
    bool inQuote = false;
    for (size_t i = 0; i <= input.size(); i++) {
        char c = (i < input.size()) ? input[i] : '\n';
        if (inQuote) {
            if (c == '"') {
                if (i + 1 < input.size() && input[i + 1] == '"') { field += '"'; i++; }
                else inQuote = false;
            } else { field += c; }
        } else {
            if (c == '"') { inQuote = true; }
            else if (c == ',') { row->data.push_back(Value::String(VMString::create(field))); field.clear(); }
            else if (c == '\n' || c == '\r') {
                if (!field.empty() || !row->data.empty()) {
                    row->data.push_back(Value::String(VMString::create(field)));
                    rows->data.push_back(Value::Array(row));
                    row = VMArray::create();
                    field.clear();
                }
                if (c == '\r' && i + 1 < input.size() && input[i + 1] == '\n') i++;
            }
            else { field += c; }
        }
    }
    return Value::Array(rows);
}

Value strFormat(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString()) return args.empty() ? Value::nil() : args[0];
    std::string tmpl(args[0].asString()->data, args[0].asString()->length);
    std::string out;
    size_t i = 0;
    while (i < tmpl.size()) {
        if (tmpl[i] == '{' && i + 1 < tmpl.size()) {
            size_t end = tmpl.find('}', i + 1);
            if (end != std::string::npos) {
                std::string key = tmpl.substr(i + 1, end - i - 1);
                if (args[1].isTable()) {
                    auto v = args[1].asTable()->get(Value::String(VMString::create(key)));
                    out += v.toString();
                }
                i = end + 1;
                continue;
            }
        }
        out += tmpl[i];
        i++;
    }
    return Value::String(VMString::create(out));
}

Value globMatch(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
    std::string s(args[0].asString()->data, args[0].asString()->length);
    std::string p(args[1].asString()->data, args[1].asString()->length);
    size_t si = 0, pi = 0, star = std::string::npos, match = 0;
    while (si < s.size()) {
        if (pi < p.size() && (p[pi] == '?' || p[pi] == s[si])) { si++; pi++; }
        else if (pi < p.size() && p[pi] == '*') { star = pi++; match = si; }
        else if (star != std::string::npos) { pi = star + 1; match++; si = match; }
        else return Value::Bool(false);
    }
    while (pi < p.size() && p[pi] == '*') pi++;
    return Value::Bool(pi == p.size());
}
} // namespace more_utils

// ═══════════════════════════════════════════════════════════════════
//  移植自 stdlib_linux.cpp 的辅助函数实现
// ═══════════════════════════════════════════════════════════════════

static std::string mu_getStr(const Value& v) {
    if (v.isString()) return std::string(v.asString()->data, v.asString()->length);
    if (v.isInt()) return std::to_string(v.asInt());
    if (v.isFloat()) return std::to_string(v.asFloat());
    if (v.isBool()) return v.asBool() ? "true" : "false";
    if (v.isNil()) return "nil";
    return "";
}

static int mu_getInt(const Value& v) {
    if (v.isInt()) return (int)v.asInt();
    if (v.isFloat()) return (int)v.asFloat();
    return 0;
}

static double mu_getNum(const Value& v) {
    if (v.isInt()) return (double)v.asInt();
    if (v.isFloat()) return v.asFloat();
    return 0.0;
}
// ═══════════════════════════════════════════════════════════════════

} // namespace cplang
