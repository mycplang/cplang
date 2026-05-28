import subprocess

# Test with file input
src = 'const x = 1;'
with open('test_src.cp', 'w', encoding='utf-8') as f:
    f.write(src)

proc = subprocess.Popen(
    [r'.\build\codegen_e2e_test.exe', r'.\test_src.cp'],
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE
)
try:
    stdout, stderr = proc.communicate(timeout=5)
    print('RC:', proc.returncode)
    print('STDOUT:', stdout.decode('utf-8', errors='replace'))
    print('STDERR:', stderr.decode('utf-8', errors='replace'))
except subprocess.TimeoutExpired:
    proc.kill()
    stdout, stderr = proc.communicate()
    print('TIMEOUT!')
    print('STDOUT:', stdout.decode('utf-8', errors='replace'))
    print('STDERR:', stderr.decode('utf-8', errors='replace'))
