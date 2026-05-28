#pragma once
#include "vm/value.hpp"
#include <vector>

namespace cplang {

namespace optional_ {
    Value optNone(std::vector<Value>& args);
    Value optSome(std::vector<Value>& args);
    Value optHas(std::vector<Value>& args);
    Value optIsNone(std::vector<Value>& args);
    Value optGet(std::vector<Value>& args);
    Value optOr(std::vector<Value>& args);
}

namespace variant_ {
    Value varNew(std::vector<Value>& args);
    Value varType(std::vector<Value>& args);
    Value varVal(std::vector<Value>& args);
    Value varIs(std::vector<Value>& args);
    Value varVisit(std::vector<Value>& args);
}

namespace any_ {
    Value anyNew(std::vector<Value>& args);
    Value anyHasValue(std::vector<Value>& args);
    Value anyType(std::vector<Value>& args);
    Value anyGet(std::vector<Value>& args);
    Value anyCast(std::vector<Value>& args);
    Value anyReset(std::vector<Value>& args);
}

namespace tuple_ {
    Value tupMake(std::vector<Value>& args);
    Value tupGet(std::vector<Value>& args);
    Value tupSize(std::vector<Value>& args);
    Value tupCat(std::vector<Value>& args);
    Value tupSlice(std::vector<Value>& args);
}

namespace utils {
    Value clamp(std::vector<Value>& args);
    Value lerp(std::vector<Value>& args);
    Value map(std::vector<Value>& args);
    Value randomInt(std::vector<Value>& args);
    Value randomChoice(std::vector<Value>& args);
    Value isEmpty(std::vector<Value>& args);
    Value defaultVal(std::vector<Value>& args);
}

namespace more_utils {
    Value arrShuffle(std::vector<Value>& args);
    Value strRepeat(std::vector<Value>& args);
    Value deepEqual(std::vector<Value>& args);
    Value shallowClone(std::vector<Value>& args);
    Value typeOf(std::vector<Value>& args);
    Value assertFn(std::vector<Value>& args);
    Value benchmark(std::vector<Value>& args);
    Value levenshtein(std::vector<Value>& args);
    Value uuid4(std::vector<Value>& args);
    Value csvParse(std::vector<Value>& args);
    Value strFormat(std::vector<Value>& args);
    Value globMatch(std::vector<Value>& args);
    Value strInterpolate(std::vector<Value>& args);
}

} // namespace cplang
