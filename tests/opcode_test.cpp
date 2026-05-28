#include "vm/vm.hpp"
#include <cstdio>
using namespace cplang;
int main() {
    // Print all opcodes
    const char* names[] = {
        "OP_LOADNIL","OP_LOADBOOL","OP_LOADINT","OP_LOADFLT","OP_LOADSTR","OP_LOADCONST",
        "OP_MOVE","OP_LOADGLOBAL","OP_STOREGLOBAL","OP_LOADLOCAL","OP_STORELOCAL",
        "OP_ADD","OP_SUB","OP_MUL","OP_DIV","OP_IDIV","OP_MOD","OP_POW","OP_NEG",
        "OP_BAND","OP_BOR","OP_BXOR","OP_BSHL","OP_BSHR","OP_BNOT",
        "OP_CMPEQ","OP_CMPNE","OP_CMPLT","OP_CMPLE","OP_CMPGT","OP_CMPGE",
        "OP_JUMP","OP_JUMPIF","OP_JUMPNIF","OP_CALL","OP_RETURN"
    };
    int vals[] = {
        OP_LOADNIL,OP_LOADBOOL,OP_LOADINT,OP_LOADFLT,OP_LOADSTR,OP_LOADCONST,
        OP_MOVE,OP_LOADGLOBAL,OP_STOREGLOBAL,OP_LOADLOCAL,OP_STORELOCAL,
        OP_ADD,OP_SUB,OP_MUL,OP_DIV,OP_IDIV,OP_MOD,OP_POW,OP_NEG,
        OP_BAND,OP_BOR,OP_BXOR,OP_BSHL,OP_BSHR,OP_BNOT,
        OP_CMPEQ,OP_CMPNE,OP_CMPLT,OP_CMPLE,OP_CMPGT,OP_CMPGE,
        OP_JUMP,OP_JUMPIF,OP_JUMPNIF,OP_CALL,OP_RETURN
    };
    for (int i = 0; i < 36; i++) {
        printf("%3d = %s\n", vals[i], names[i]);
    }
    return 0;
}
