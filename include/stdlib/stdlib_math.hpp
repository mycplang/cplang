#pragma once
#include "vm/value.hpp"
#include <vector>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  数学函数
// ═══════════════════════════════════════════════════════════════════

namespace math {
    Value abs(std::vector<Value>& args);
    Value sqrt(std::vector<Value>& args);
    Value pow(std::vector<Value>& args);
    Value floor(std::vector<Value>& args);
    Value ceil(std::vector<Value>& args);
    Value round(std::vector<Value>& args);
    Value random(std::vector<Value>& args);
    Value max(std::vector<Value>& args);
    Value min(std::vector<Value>& args);
    Value sin(std::vector<Value>& args);
    Value cos(std::vector<Value>& args);
    Value tan(std::vector<Value>& args);
    Value asin(std::vector<Value>& args);
    Value acos(std::vector<Value>& args);
    Value atan(std::vector<Value>& args);
    Value atan2(std::vector<Value>& args);
    Value log(std::vector<Value>& args);
    Value log10(std::vector<Value>& args);
    Value exp(std::vector<Value>& args);
    Value pi(std::vector<Value>& args);
    Value e(std::vector<Value>& args);
}

// ═══════════════════════════════════════════════════════════════════
//  数学扩展函数
// ═══════════════════════════════════════════════════════════════════

namespace math_more {
    Value asin(std::vector<Value>& args);
    Value acos(std::vector<Value>& args);
    Value atan(std::vector<Value>& args);
    Value atan2(std::vector<Value>& args);
    Value sinh(std::vector<Value>& args);
    Value cosh(std::vector<Value>& args);
    Value tanh(std::vector<Value>& args);
    Value asinh(std::vector<Value>& args);
    Value acosh(std::vector<Value>& args);
    Value atanh(std::vector<Value>& args);
    Value log10(std::vector<Value>& args);
    Value log2(std::vector<Value>& args);
    Value exp(std::vector<Value>& args);
    Value exp2(std::vector<Value>& args);
    Value expm1(std::vector<Value>& args);
    Value cbrt(std::vector<Value>& args);
    Value hypot(std::vector<Value>& args);
    Value ceil(std::vector<Value>& args);
    Value floor(std::vector<Value>& args);
    Value round(std::vector<Value>& args);
    Value trunc(std::vector<Value>& args);
    Value fabs(std::vector<Value>& args);
    Value fmod(std::vector<Value>& args);
    Value remainder(std::vector<Value>& args);
    Value copysign(std::vector<Value>& args);
    Value fma(std::vector<Value>& args);
    Value ln(std::vector<Value>& args);
    Value factorial(std::vector<Value>& args);
    Value sign(std::vector<Value>& args);
    Value deg2rad(std::vector<Value>& args);
    Value rad2deg(std::vector<Value>& args);
}

// ═══════════════════════════════════════════════════════════════════
//  统计函数
// ═══════════════════════════════════════════════════════════════════

namespace statistics {
    Value mean(std::vector<Value>& args);
    Value median(std::vector<Value>& args);
    Value variance(std::vector<Value>& args);
    Value stddev(std::vector<Value>& args);
    Value mode(std::vector<Value>& args);
    Value percentile(std::vector<Value>& args);
    Value min(std::vector<Value>& args);
    Value max(std::vector<Value>& args);
    Value range(std::vector<Value>& args);
    Value correlation(std::vector<Value>& args);
    Value covariance(std::vector<Value>& args);
    Value linearRegression(std::vector<Value>& args);
}

// ═══════════════════════════════════════════════════════════════════
//  随机数增强
// ═══════════════════════════════════════════════════════════════════

namespace random {
    Value randomFloat(std::vector<Value>& args);
    Value randomNormal(std::vector<Value>& args);
    Value randomSeed(std::vector<Value>& args);
    Value shuffle(std::vector<Value>& args);
    Value randomUniformInt(std::vector<Value>& args);
    Value randomExponential(std::vector<Value>& args);
    Value randomBernoulli(std::vector<Value>& args);
    Value randomPoisson(std::vector<Value>& args);
}

} // namespace cplang
