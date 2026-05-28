#!/usr/bin/env python3
"""CP Lang stdlib NaN-boxing migration"""
import re, sys

def migrate(text):
    # 1. Factory methods
    for old, new in [
        (r'Value::String\(', 'makeStringVal('),
        (r'Value::Array\(', 'makeArrayVal('),
        (r'Value::Table\(', 'makeTableVal('),
        (r'Value::Function\(', 'makeFunctionVal('),
    ]:
        text = re.sub(old, new, text)
    
    for cls in ['Set', 'Stack', 'Queue', 'Deque', 'PriorityQueue',
                 'LinkedList', 'SLinkedList', 'MultiSet', 'MultiMap',
                 'UnorderedSet', 'UnorderedMultiSet', 'UnorderedMap', 'UnorderedMultiMap',
                 'Object', 'UserData']:
        text = re.sub(rf'Value::{cls}\(([^)]+)\)', rf'makePtrVal(reinterpret_cast<VMObject*>(\1))', text)
    
    # 2. Tag comparisons
    tags = {
        'T_NIL':'isNil()','T_BOOL':'isBool()','T_INT':'isInt()','T_FLOAT':'isFloat()',
        'T_STRING':'isString()','T_ARRAY':'isArray()','T_TABLE':'isTable()',
        'T_SET':'isSet()','T_STACK':'isStack()','T_QUEUE':'isQueue()',
        'T_DEQUE':'isDeque()','T_PRIORITY_QUEUE':'isPriorityQueue()',
        'T_LINKEDLIST':'isLinkedList()','T_SLINKEDLIST':'isSLinkedList()',
        'T_MULTISET':'isMultiSet()','T_MULTIMAP':'isMultiMap()',
        'T_UNORDERED_SET':'isUnorderedSet()','T_UNORDERED_MULTISET':'isUnorderedMultiSet()',
        'T_UNORDERED_MAP':'isUnorderedMap()','T_UNORDERED_MULTIMAP':'isUnorderedMultiMap()',
        'T_FUNCTION':'isFunction()','T_CFUNCTION':'isCFunction()','T_CLOSURE':'isClosure()',
        'T_CLASS':'isClass()','T_INSTANCE':'isInstance()','T_UPVALUE':'isUpvalue()',
        'T_USERDATA':'isUserData()',
    }
    for tag, method in tags.items():
        text = text.replace(f'.tag == Value::{tag}', f'.{method}')
        text = text.replace(f'.tag != Value::{tag}', f'!.{method}')
    text = text.replace('.tag >= Value::T_STRING', '.isObject()')
    
    # 3. Member access
    text = re.sub(r'(?<![a-zA-Z_])v\.i\b(?!s)', 'v.asInt()', text)
    text = re.sub(r'(?<![a-zA-Z_])v\.f\b', 'v.asFloat()', text)
    text = re.sub(r'(?<![a-zA-Z_])v\.obj\b', 'v.asPtr()', text)
    text = re.sub(r'(?<![a-zA-Z_])v\.str\b', 'v.asString()', text)
    text = re.sub(r'(?<![a-zA-Z_])v\.arr\b', 'v.asArray()', text)
    text = re.sub(r'(?<![a-zA-Z_])v\.func\b', 'v.asFunction()', text)
    
    # Specific fixes
    text = re.sub(r'args\[0\]\.i\b', 'args[0].asInt()', text)
    text = re.sub(r'\.asInt\(\)\s*\?\s*"true"\s*:\s*"false"', '.asBool() ? "true" : "false"', text)
    text = re.sub(r'\.asInt\(\)\s*\?\s*1\s*:\s*0\b(?!\.)', '.asBool() ? 1 : 0', text)
    text = text.replace('a.tag != b.tag', 'a.raw() != b.raw()')
    
    return text

with open(sys.argv[1], 'r', encoding='utf-8') as f:
    original = f.read()

migrated = migrate(original)

# Verify no corruption
for i, line in enumerate(migrated.split('\n'), 1):
    if line.lstrip().startswith('include') and not line.lstrip().startswith('#include'):
        print(f"ERROR: corrupted include at line {i}: {line[:80]}", file=sys.stderr)
        sys.exit(1)

with open(sys.argv[1], 'w', encoding='utf-8') as f:
    f.write(migrated)

print(f"OK: {len(original)} → {len(migrated)} chars")
