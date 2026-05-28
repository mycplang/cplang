#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "codegen/codegen.hpp"
#include "vm/vm.hpp"

using namespace cplang;

std::string readFile(const char* path) {
    std::ifstream file(path);
    if (!file) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

const char* opcodeName(UInt8 op) {
    switch (op) {
        case 0x00: return "NOP";
        case 0x01: return "LOADBOOL";
        case 0x02: return "LOADINT";
        case 0x03: return "LOADFLT";
        case 0x04: return "LOADSTR";
        case 0x05: return "LOADCONST";
        case 0x06: return "MOVE";
        case 0x07: return "LOADGLOBAL";
        case 0x08: return "STOREGLOBAL";
        case 0x10: return "LOADLOCAL";
        case 0x11: return "STORELOCAL";
        case 0x20: return "ADD";
        case 0x21: return "SUB";
        case 0x22: return "MUL";
        case 0x23: return "DIV";
        case 0x24: return "IDIV";
        case 0x25: return "MOD";
        case 0x26: return "POW";
        case 0x27: return "NEG";
        case 0x28: return "BAND";
        case 0x29: return "BOR";
        case 0x2A: return "BXOR";
        case 0x2B: return "BSHL";
        case 0x2C: return "BSHR";
        case 0x2D: return "BNOT";
        case 0x30: return "CMPEQ";
        case 0x31: return "CMPNE";
        case 0x32: return "CMPLT";
        case 0x33: return "CMPLE";
        case 0x34: return "CMPGT";
        case 0x35: return "CMPGE";
        case 0x40: return "JUMP";
        case 0x41: return "JUMPIF";
        case 0x42: return "JUMPNIF";
        case 0x50: return "CALL";
        case 0x51: return "TAILCALL";
        case 0x52: return "RETURN";
        case 0x60: return "NEWARRAY";
        case 0x62: return "GETELEM";
        case 0x63: return "SETELEM";
        case 0x64: return "GETIDX";
        case 0x65: return "SETIDX";
        case 0x68: return "CONCAT";
        case 0x69: return "STRLEN";
        case 0x70: return "TONUM";
        case 0x71: return "TOSTR";
        case 0x72: return "TOBOOL";
        case 0x73: return "TYPEOF";
        case 0x74: return "ISNULL";
        case 0x80: return "NEWCLASS";
        case 0x81: return "IMPORT";
        case 0xFF: return "BAD";
        default: return "???";
    }
}

void printBytecode(const std::vector<UInt8>& code) {
    // Format: [op][a][b][c][imm32][pad]
    // VM reads: a=code[i+1], b=code[i+2], c=code[i+3]
    // VM reads imm32 from code[i+4..i+7]
    for (size_t i = 0; i < code.size(); i += 16) {
        std::printf("[%4zu] ", i);
        
        // Print raw bytes
        for (size_t j = 0; j < 16 && i + j < code.size(); j++) {
            std::printf("%02x ", code[i + j]);
        }
        
        UInt8 op = code[i];
        UInt8 a = code[i + 1];
        UInt8 b = code[i + 2];
        UInt8 c = code[i + 3];
        Int32 imm = *(Int32*)&code[i + 4];
        
        std::printf(" | %-10s a=%-3d b=%-3d c=%-3d imm=%d\n", 
            opcodeName(op), a, b, c, imm);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("Usage: %s <file.cp>\n", argv[0]);
        return 1;
    }

    std::string source = readFile(argv[1]);
    if (source.empty()) {
        std::printf("Failed to read file: %s\n", argv[1]);
        return 1;
    }

    // Lexer
    Lexer lexer(source);

    // Parser
    Parser parser(&lexer);
    auto ast = parser.parse();
    if (!ast) {
        std::printf("Parse error\n");
        return 1;
    }

    // Semantic analysis
    SemanticAnalyzer semantic;
    semantic.analyze(ast);

    // Codegen
    Codegen codegen;
    VMFunction* func = codegen.compile(ast);
    if (!func) {
        std::printf("Codegen error: %s\n", codegen.errorMessage().c_str());
        return 1;
    }

    // Print bytecode
    std::printf("=== Bytecode (%zu bytes) ===\n", func->code.size());
    printBytecode(func->code);

    // Print constants
    std::printf("\n=== Constants ===\n");
    for (size_t i = 0; i < func->constants.size(); i++) {
        const Value& v = func->constants[i];
        std::printf("[%zu] tag=%d ", i, v.tag);
        if (v.tag == Value::T_INT) {
            std::printf("int=%lld\n", v.asInt());
        } else if (v.tag == Value::T_STRING && v.asString()) {
            std::printf("string=\"%s\"\n", v.asString()->data);
        } else if (v.tag == Value::T_FUNCTION) {
            std::printf("<function>\n");
        } else {
            std::printf("other (tag=%d)\n", v.tag);
        }
    }

    return 0;
}
