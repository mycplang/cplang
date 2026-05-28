#pragma once
#include "vm/vm_fwd.hpp"
#include "vm/vm_object.hpp"
#include "vm/vm_opcodes.hpp"

namespace cplang {

struct HandlerFrame {
    Int32  catchPC = 0;
    Value* savedBase = nullptr;
    Int32  resultReg = 0;
};

struct CallFrame {
    VMFunction* func = nullptr;
    VMClosure*   closure = nullptr;
    Value*       base = nullptr;
    Value*       savedBase = nullptr;
    const UInt8* pc = nullptr;
    const UInt8* returnPC = nullptr;
    Int32        returnBaseOffset = -1;
    Int32        returnPcOffset = 0;
    Int32        resultReg = 0;
    Int32        resultCount = 1;
    Int32        baseOffset = 0;
};

} // namespace cplang
