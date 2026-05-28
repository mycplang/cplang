# -*- coding: utf-8 -*-
import subprocess

tests = [
    ('变量 x = 5; 打印
    ('常量 PI = 3; 打印(PI);', '3'),
    ('如果(1) 打印(42);', '42'),
    ('如果(0) 打印(1); 否则 打印(2);', '2'),
    ('变量 i = 0; 当(i < 3) { 打印
]

print("=== CPLang 中文测试 ===\n")

for src, expected in tests:
    with open('test_src.cp', 'w', encoding='utf-8') as f:
        f.write(src)
    r = subprocess.run([r'.\build\codegen_e2e_test.exe', r'.\test_src.cp'], 
                       capture_output=True, timeout=5)
    out = r.stdout.decode('utf-8', errors='replace')
    
    lines = []
    in_output = False
    for line in out.split('\n'):
        if 'loadModule' in line:
            in_output = True
            continue
        if in_output and 'returned' in line:
            break
        if in_output and line.strip():
            lines.append(line.strip())
    result = ' '.join(lines)
    
    status = 'OK' if expected == result else 'FAIL'
    print(f'{status} | {src[:40]} | expected={expected} | got={result}')
