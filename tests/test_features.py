import subprocess

tests = [
    ('打印(1+2);', '3'),
    ('打印(3*4);', '12'),
    ('变量 x = 10; 打印
    ('如果(1) 打印(42);', '42'),
    ('如果(0) 打印(1); 否则 打印(2);', '2'),
    ('变量 i = 0; 当(i < 3) { 打印
    ('常量 PI = 3; 打印(PI);', '3'),
]

print('=== CPLang 功能测试 ===')
passed = 0
failed = 0

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
    
    if 'COMPILE ERR' in out or 'VM ERR' in out:
        status = 'FAIL'
        failed += 1
        print(f'FAIL | {src[:35]}')
        print(f'     | Error in compilation or runtime')
    elif expected == result:
        status = 'OK'
        passed += 1
        print(f'OK   | {src[:35]} -> {result}')
    else:
        status = 'FAIL'
        failed += 1
        print(f'FAIL | {src[:35]}')
        print(f'     | expected: {expected}, got: {result}')

print()
print(f'=== 结果: {passed} passed, {failed} failed ===')
