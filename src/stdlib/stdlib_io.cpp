#include "stdlib/stdlib.hpp"

namespace cplang {

// IO functions (print, println, input, readFile, writeFile, exists)
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerIO(VM* vm) {
    registerFunction(vm, "print", io::print);
    registerFunction(vm, "println", io::println);
    registerFunction(vm, "input", io::input);
    registerFunction(vm, "readFile", io::readFile);
    registerFunction(vm, "writeFile", io::writeFile);
    registerFunction(vm, "exists", io::exists);
    
    // 中文别名
    registerAlias(vm, "打印", "println");
    registerAlias(vm, "输入", "input");
}

Value io::print(std::vector<Value>& args) {
    for (size_t i = 0; i < args.size(); i++) {
        if (i > 0) std::cout << " ";
        
        if (args[i].isString()) {
            std::cout.write(args[i].asString()->data, args[i].asString()->length);
        } else if (args[i].isInt()) {
            std::cout << args[i].asInt();
        } else if (args[i].isFloat()) {
            std::cout << args[i].asFloat();
        } else if (args[i].isBool()) {
            std::cout << (args[i].asInt() ? "true" : "false");
        } else if (args[i].isNil()) {
            std::cout << "nil";
        } else if (args[i].isArray()) {
            std::cout << "[array]";
        } else {
            std::cout << "[object]";
        }
    }
    return Value::nil();
}

Value io::println(std::vector<Value>& args) {
    io::print(args);
    std::cout << std::endl;
    return Value::nil();
}

Value io::input(std::vector<Value>& args) {
    if (!args.empty()) {
        io::print(args);
    }
    
    std::string line;
    std::getline(std::cin, line);
    return Value::String(VMString::create(line));
}

Value io::readFile(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    
    std::string filename(args[0].asString()->data, args[0].asString()->length);
    std::ifstream file(filename);
    
    if (!file.is_open()) return Value::nil();
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return Value::String(VMString::create(buffer.str()));
}

Value io::writeFile(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) {
        return Value::Bool(false);
    }
    
    std::string filename(args[0].asString()->data, args[0].asString()->length);
    std::ofstream file(filename);
    
    if (!file.is_open()) return Value::Bool(false);
    
    file.write(args[1].asString()->data, args[1].asString()->length);
    file.close();
    
    return Value::Bool(true);
}

Value io::exists(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    
    std::string filename(args[0].asString()->data, args[0].asString()->length);
    return Value::Bool(std::ifstream(filename).good());
}

} // namespace cplang
