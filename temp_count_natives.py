import re

with open(r'D:\CPLANG\src\stdlib\stdlib.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

registers = re.findall(r'registerNative\(', content)
print(f'registerNative 调用次数: {len(registers)}')

names = re.findall(r'registerNative\(\s*"([^"]+)"', content)
print(f'\n注册的函数名数量: {len(names)}')
print('\n前30个函数名:')
for n in names[:30]:
    print(f'  - {n}')
print(f'\n  ... 共 {len(names)} 个')
