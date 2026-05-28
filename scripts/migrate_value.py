#!/usr/bin/env python3
"""
CP语言 NaN-boxing 迁移脚本
将旧的 tagged-union Value 用法迁移到新的 NaN-boxing Value API
"""
import re, sys, os

FILES_TO_MIGRATE = [
    "src/vm/vm.cpp",
    "src/vm/vm_opt.cpp", 
    "src/vm/vm_switch.cpp",
    "src/codegen/codegen.cpp",
    "src/codegen/codegen_opt.cpp",
    "src/codegen/llvm_codegen.cpp",
    "src/stdlib/stdlib.cpp",
    "src/stdlib/stdlib_iterator.cpp",
    "src/stdlib/stdlib_threading.cpp",
    "src/stdlib/stdlib_map.cpp",
    "src/stdlib/stdlib_file_log.cpp",
    "src/stdlib/stdlib_aes.cpp",
    "src/stdlib/stdlib_charset.cpp",
    "src/stdlib/stdlib_crypto_plus.cpp",
    "src/stdlib/stdlib_db.cpp",
    "src/stdlib/stdlib_fix_missing.cpp",
    "src/stdlib/stdlib_http.cpp",
    "src/stdlib/stdlib_imgui.cpp",
    "src/stdlib/stdlib_math_special.cpp",
    "src/stdlib/stdlib_misc_modules.cpp",
    "src/stdlib/stdlib_p1_enhance.cpp",
    "src/stdlib/stdlib_p2_more.cpp",
    "src/stdlib/stdlib_p3_util.cpp",
    "src/stdlib/stdlib_r10_r11.cpp",
    "src/stdlib/stdlib_raylib.cpp",
    "src/stdlib/stdlib_raylib_unit.cpp",
    "src/stdlib/stdlib_redis.cpp",
    "src/stdlib/stdlib_stubs.cpp",
    "src/stdlib/stdlib_algo_missing.cpp",
    "src/compiler/optimized_compiler.cpp",
    "src/debug/debugger.cpp",
    "src/exception/exception_handler.cpp", 
    "src/jit/jit_compiler.cpp",
    "src/jit/orc_jit.cpp",
    "src/main.cpp",
    "src/repl.cpp",
    "src/module/module_system.cpp",
    "src/optimizer/constant_folder.cpp",
    "src/optimizer/dead_code_eliminator.cpp",
    "include/vm/vm_opt.hpp",
]

REPLACEMENTS = [
    # tag comparisons → method calls
    (r'\.tag\s*==\s*Value::T_NIL', '.isNil()'),
    (r'\.tag\s*==\s*Value::T_BOOL', '.isBool()'),
    (r'\.tag\s*==\s*Value::T_INT', '.isInt()'),
    (r'\.tag\s*==\s*Value::T_FLOAT', '.isFloat()'),
    (r'\.tag\s*==\s*Value::T_STRING', '.isString()'),
    (r'\.tag\s*==\s*Value::T_ARRAY', '.isArray()'),
    (r'\.tag\s*==\s*Value::T_TABLE', '.isTable()'),
    (r'\.tag\s*==\s*Value::T_SET', '.isSet()'),
    (r'\.tag\s*==\s*Value::T_FUNCTION', '.isFunction()'),
    (r'\.tag\s*==\s*Value::T_CFUNCTION', '.isCFunction()'),
    (r'\.tag\s*==\s*Value::T_CLOSURE', '.isClosure()'),
    (r'\.tag\s*==\s*Value::T_CLASS', '.isClass()'),
    (r'\.tag\s*==\s*Value::T_INSTANCE', '.isInstance()'),
    (r'\.tag\s*==\s*Value::T_UPVALUE', '.isUpvalue()'),
    (r'\.tag\s*==\s*Value::T_USERDATA', '.isUserData()'),
    (r'\.tag\s*==\s*Value::T_RAYLIB', '.isRaylib()'),
    (r'\.tag\s*==\s*Value::T_STACK', '.isStack()'),
    (r'\.tag\s*==\s*Value::T_QUEUE', '.isQueue()'),
    (r'\.tag\s*==\s*Value::T_DEQUE', '.isDeque()'),
    (r'\.tag\s*==\s*Value::T_PRIORITY_QUEUE', '.isPriorityQueue()'),
    
    # read .i (int value) → .asInt()
    (r'\.i\b(?!\w)', '.asInt()'),
    # read .f (float value) → .asFloat()
    (r'\.f\b(?!\w)', '.asFloat()'),
    # .str → .asString()
    (r'\.str\b(?!\w)', '.asString()'),
    # .arr → .asArray()
    (r'\.arr\b(?!\w)', '.asArray()'),
    # .obj → .asPtr()
    (r'\.obj\b(?!\w)', '.asPtr()'),
    # .func → .asFunction()
    (r'\.func\b(?!\w)', '.asFunction()'),
    
    # tag write (assignment to .tag) → remove (handled by factory)
    # Most common: v.tag = Value::T_INT → already handled via factory
    
    # Value::String(s) → makeStringVal(s)
    (r'Value::String\(', 'makeStringVal('),
    (r'Value::Array\(', 'makeArrayVal('),
    (r'Value::Table\(', 'makeTableVal('),
    (r'Value::Set\(', 'makePtrVal(reinterpret_cast<VMObject*>('),
    # Fix the extra paren
    (r'makePtrVal\(reinterpret_cast<VMObject*>\(([^)]+)\)\)', r'makePtrVal(reinterpret_cast<VMObject*>(\1))'),
    (r'Value::Function\(', 'makeFunctionVal('),
    (r'Value::Object\(', 'makePtrVal('),
    (r'Value::UserData\(', 'makePtrVal('),
    (r'Value::RaylibObj\(', 'makePtrVal('),
    (r'Value::Stack\(', 'makePtrVal(reinterpret_cast<VMObject*>('),
    (r'Value::Queue\(', 'makePtrVal(reinterpret_cast<VMObject*>('),
    
    # switch(v.tag) → if-else chain (manual, so just warn)
]

def migrate_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original = content
    changed = 0
    
    for pattern, replacement in REPLACEMENTS:
        new_content = re.sub(pattern, replacement, content)
        if new_content != content:
            changed += 1
            content = new_content
    
    if content != original:
        # Safety: write to .migrated backup
        backup = filepath + '.bak'
        with open(backup, 'w', encoding='utf-8') as f:
            f.write(original)
        
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        
        print(f"✓ {filepath} ({changed} changes)")
        return True
    return False

def main():
    project_root = "/home/cplang"
    os.chdir(project_root)
    
    migrated = 0
    for f in FILES_TO_MIGRATE:
        path = os.path.join(project_root, f)
        if os.path.exists(path):
            if migrate_file(path):
                migrated += 1
    
    print(f"\nMigrated {migrated} files. Backups at *.bak")

if __name__ == "__main__":
    main()
