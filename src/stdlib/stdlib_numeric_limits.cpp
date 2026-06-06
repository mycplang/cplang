#include "stdlib/stdlib.hpp"

namespace cplang {

// ═══════════════════════════════════════════════════════════════
//  CP语言 标准库 — 数值极限 (numeric_limits)
// ═══════════════════════════════════════════════════════════════

#include <cmath>

namespace numlim {
    Value int8Max(std::vector<Value>&)   { return Value::fromInt8(127); }
    Value int8Min(std::vector<Value>&)   { return Value::fromInt8(-128); }
    Value int16Max(std::vector<Value>&)  { return Value::fromInt16(32767); }
    Value int16Min(std::vector<Value>&)  { return Value::fromInt16(-32768); }
    Value int32Max(std::vector<Value>&)  { return Value::fromInt32(2147483647); }
    Value int32Min(std::vector<Value>&)  { return Value::fromInt32(-2147483647 - 1); }
    Value int64Max(std::vector<Value>&)  { return Value::fromOldInt(9223372036854775807LL); }
    Value int64Min(std::vector<Value>&)  { return Value::fromOldInt(-9223372036854775807LL - 1); }
    Value float32Max(std::vector<Value>&) { return Value::fromFloat32(3.402823466e+38f); }
    Value float32Min(std::vector<Value>&) { return Value::fromFloat32(1.175494351e-38f); }
    Value float32Epsilon(std::vector<Value>&) { return Value::fromFloat32(1.192092896e-7f); }
    Value floatMax(std::vector<Value>&)  { return Value::fromFloat(1.7976931348623157e+308); }
    Value floatMin(std::vector<Value>&)  { return Value::fromFloat(2.2250738585072014e-308); }
    Value floatEpsilon(std::vector<Value>&) { return Value::fromFloat(2.2204460492503131e-16); }
    Value floatInfinity(std::vector<Value>&) { return Value::fromFloat(INFINITY); }
    Value floatNaN(std::vector<Value>&)  { return Value::fromFloat(NAN); }
    Value isNaN(std::vector<Value>& args) {
        if (args.empty()) return Value::Bool(false);
        if (args[0].isFloat()) return Value::Bool(std::isnan(args[0].asFloat()));
        if (args[0].isFloat32()) return Value::Bool(std::isnan(args[0].asFloat32()));
        return Value::Bool(false);
    }
    Value isInfinity(std::vector<Value>& args) {
        if (args.empty()) return Value::Bool(false);
        if (args[0].isFloat()) return Value::Bool(std::isinf(args[0].asFloat()));
        if (args[0].isFloat32()) return Value::Bool(std::isinf(args[0].asFloat32()));
        return Value::Bool(false);
    }
    Value isFinite(std::vector<Value>& args) {
        if (args.empty()) return Value::Bool(false);
        if (args[0].isFloat()) return Value::Bool(std::isfinite(args[0].asFloat()));
        if (args[0].isFloat32()) return Value::Bool(std::isfinite(args[0].asFloat32()));
        return Value::Bool(false);
    }
    Value intBytes(std::vector<Value>&)  { return Value::Int(sizeof(int)); }
    Value floatBytes(std::vector<Value>&) { return Value::Int(sizeof(double)); }
}

void StdLib::registerNumericLimits(VM* vm) {
    registerFunction(vm, "int8Max",   numlim::int8Max);
    registerFunction(vm, "int8Min",   numlim::int8Min);
    registerFunction(vm, "int16Max",  numlim::int16Max);
    registerFunction(vm, "int16Min",  numlim::int16Min);
    registerFunction(vm, "int32Max",  numlim::int32Max);
    registerFunction(vm, "int32Min",  numlim::int32Min);
    registerFunction(vm, "int64Max",  numlim::int64Max);
    registerFunction(vm, "int64Min",  numlim::int64Min);
    registerFunction(vm, "float32Max", numlim::float32Max);
    registerFunction(vm, "float32Min", numlim::float32Min);
    registerFunction(vm, "float32Epsilon", numlim::float32Epsilon);
    registerFunction(vm, "floatMax",  numlim::floatMax);
    registerFunction(vm, "floatMin",  numlim::floatMin);
    registerFunction(vm, "floatEpsilon", numlim::floatEpsilon);
    registerFunction(vm, "floatInfinity", numlim::floatInfinity);
    registerFunction(vm, "floatNaN",  numlim::floatNaN);
    registerFunction(vm, "isNaN",     numlim::isNaN);
    registerFunction(vm, "isInfinity", numlim::isInfinity);
    registerFunction(vm, "isFinite",  numlim::isFinite);
    registerFunction(vm, "intBytes",  numlim::intBytes);
    registerFunction(vm, "floatBytes", numlim::floatBytes);
    registerAlias(vm, "整数8最大值", "int8Max");
    registerAlias(vm, "整数8最小值", "int8Min");
    registerAlias(vm, "整数16最大值", "int16Max");
    registerAlias(vm, "整数16最小值", "int16Min");
    registerAlias(vm, "整数32最大值", "int32Max");
    registerAlias(vm, "整数32最小值", "int32Min");
    registerAlias(vm, "整数64最大值", "int64Max");
    registerAlias(vm, "整数64最小值", "int64Min");
    registerAlias(vm, "浮点32最大值", "float32Max");
    registerAlias(vm, "浮点32最小值", "float32Min");
    registerAlias(vm, "浮点32精度", "float32Epsilon");
    registerAlias(vm, "浮点最大值", "floatMax");
    registerAlias(vm, "浮点最小值", "floatMin");
    registerAlias(vm, "浮点精度", "floatEpsilon");
    registerAlias(vm, "浮点无穷大", "floatInfinity");
    registerAlias(vm, "浮点非数", "floatNaN");
    registerAlias(vm, "是非数", "isNaN");
    registerAlias(vm, "是无穷大", "isInfinity");
    registerAlias(vm, "是有限数", "isFinite");
    registerAlias(vm, "整数字节数", "intBytes");
    registerAlias(vm, "浮点字节数", "floatBytes");
}

} // namespace cplang
