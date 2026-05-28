// Math special functions: gamma, error functions
// #include'd from stdlib.cpp, in namespace cplang

namespace math_ns_special {

// Gamma functions
Value tgamma_(std::vector<Value>& args) {
    if (args.empty()) return Value::Float(std::tgamma(0.0)); // +inf
    return Value::Float(std::tgamma(args[0].asFloat()));
}
Value lgamma_(std::vector<Value>& args) {
    if (args.empty()) return Value::Float(0.0);
    return Value::Float(std::lgamma(args[0].asFloat()));
}

// Error functions
Value erf_(std::vector<Value>& args) {
    if (args.empty()) return Value::Float(0.0);
    return Value::Float(std::erf(args[0].asFloat()));
}
Value erfc_(std::vector<Value>& args) {
    if (args.empty()) return Value::Float(1.0);
    return Value::Float(std::erfc(args[0].asFloat()));
}

} // namespace math_ns_special

void StdLib::registerMathSpecial(VM* vm) {
    using namespace math_ns_special;
    registerFunction(vm, "tgamma",   tgamma_);
    registerFunction(vm, "lgamma",   lgamma_);
    registerFunction(vm, "erf",      erf_);
    registerFunction(vm, "erfc",     erfc_);
    registerAlias(vm, "伽马函数",    "tgamma");
    registerAlias(vm, "对数伽马",    "lgamma");
    registerAlias(vm, "误差函数",    "erf");
    registerAlias(vm, "互补误差",    "erfc");
}
