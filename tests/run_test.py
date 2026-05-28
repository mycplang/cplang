import subprocess, sys, os

src = sys.argv[1]
# Write source to UTF-8 file
with open('test_src.cp', 'w', encoding='utf-8') as f:
    f.write(src)

# Pass FILE PATH to exe (exe reads file as UTF-8)
r = subprocess.run(['.\\build\\codegen_e2e_test.exe', '.\\test_src.cp'],
                   capture_output=True, encoding='utf-8', errors='replace')
print('STDOUT:', r.stdout)
if r.stderr:
    print('STDERR:', r.stderr)
print('RC:', r.returncode)
