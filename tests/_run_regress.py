import subprocess, os, sys, re, time

# --- Auto-detect paths from script location ---
_script_dir = os.path.dirname(os.path.abspath(__file__))
_project_dir = os.path.dirname(_script_dir)
CPLANG = os.path.join(_project_dir, "build", "cplang.exe")
TEST_DIR = _script_dir

# 已知循环导入的文件跳过
skip = {'circular_a.cp', 'circular_b.cp',
         # 负向测试：预期编译错误，不纳入通过率统计
         'generic_constraint_violation.cp',
         'test_defer.cp',
         'test_err_type.cp', 'test_err_type2.cp',
         'test_err_unknown_struct.cp',
         # GUI 测试：需要 Raylib 窗口，在无显示器环境超时
         'test_dt_after_fps.cp',
         }

# 基准测试文件名模式（自动检测）
BENCH_PATTERNS = [
    r'benchmark',
    r'bench',
    r'perf_',
    r'loop100m',
    r'jit_all',
    r'jit_big',
    r'jit_loop',
    r'jit_perf',
    r'jit_pure',
]

def is_benchmark(filename):
    """检测是否为基准测试文件"""
    name = filename.lower()
    for pattern in BENCH_PATTERNS:
        if re.search(pattern, name):
            return True
    return False

# 不需要特殊 JIT 模式，所有基准测试统一使用热点 JIT（更稳健）
# 热点模式会先解释执行字节码，热点函数自动触发 JIT 编译

test_files = sorted(f for f in os.listdir(TEST_DIR) if f.endswith(".cp") and not f.startswith("_") and f not in skip)
print(f"=== CP Test Suite ({len(test_files)} files) ===", flush=True)
print(f"    检测到 {sum(1 for f in test_files if is_benchmark(f))} 个基准测试文件（自动使用更长超时 + JIT）", flush=True)
passed = 0; failed = 0; skipped = 0; errors = []
start_time = time.time()

for tf in test_files:
    fp = os.path.join(TEST_DIR, tf)
    is_bench = is_benchmark(tf)

    # 构建命令行：所有基准测试使用热点 JIT，普通测试使用纯字节码
    if is_bench:
        cmd = [CPLANG, "-c", "--hotspot", "--hotspot-threshold=50", fp]
    else:
        cmd = [CPLANG, "-c", fp]

    # 超时：基准测试 120s，普通测试 15s
    timeout_val = 120 if is_bench else 15

    try:
        r = subprocess.run(cmd, capture_output=True, encoding="utf-8",
                           errors="replace", timeout=timeout_val, cwd=_project_dir)
        ok = ("编译成功" in r.stdout or "执行完成" in r.stdout) and \
             "编译失败" not in r.stdout and "VM ERR" not in r.stdout and \
             "错误" not in r.stderr and "错误" not in r.stdout
        if ok:
            passed += 1
            prefix = "BENCH" if is_bench else "OK"
            print(f"  {prefix:5s} {tf}", flush=True)
        else:
            failed += 1; errors.append(tf)
            print(f"  FAIL  {tf}", flush=True)
            if r.stdout.strip():
                for line in r.stdout.strip().split("\n")[-3:]:
                    if line.strip(): print(f"       {line}", flush=True)
            if r.stderr.strip():
                for line in r.stderr.strip().split("\n")[-3:]:
                    if line.strip(): print(f"       {line}", flush=True)
    except subprocess.TimeoutExpired:
        failed += 1; errors.append(tf)
        print(f"  TIMEOUT({timeout_val}s) {tf}", flush=True)
    except Exception as e:
        failed += 1; errors.append(tf)
        print(f"  ERR {tf}: {e}", flush=True)

elapsed = time.time() - start_time
print(f"\n=== {passed}/{len(test_files)} passed, {failed} failed ({elapsed:.1f}s) ===", flush=True)
if errors: print("Failures:", ", ".join(errors), flush=True)