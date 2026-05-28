#!/usr/bin/env python3
"""CI 回归测试运行器 — 接受可配置的二进制路径"""
import subprocess, os, sys

# 从命令行参数或环境变量获取 cplang 路径
if len(sys.argv) > 1:
    CPLANG = sys.argv[1]
else:
    CPLANG = os.environ.get("CPLANG_BIN", "build\\cplang.exe")

TEST_DIR = os.path.dirname(os.path.abspath(__file__))
skip = {
    "circular_a.cp", "circular_b.cp",  # 循环导入测试
    "test_import_real.cp", "test_slot_main.cp",  # 需要特殊模块环境
    "generic_constraint_violation.cp",  # 负向测试：预期编译报错
}

# 基准测试关键词：含这些词的测试获得更长超时
benchmark_keywords = {"bench", "perf", "pure", "loop", "minfont", "shapeonly", "raylib_debug", "raylib_single"}
benchmark_timeout = 120  # 秒

test_files = sorted(
    f for f in os.listdir(TEST_DIR)
    if f.endswith(".cp") and not f.startswith("_") and f not in skip
)

if not os.path.exists(CPLANG):
    print(f"ERROR: cplang binary not found: {CPLANG}", flush=True)
    sys.exit(1)

print(f"=== CP Test Suite ({len(test_files)} files) ===", flush=True)
print(f"Binary: {CPLANG}", flush=True)

passed = 0
failed = 0
errors = []

for tf in test_files:
    fp = os.path.join(TEST_DIR, tf)
    try:
        r = subprocess.run(
            [CPLANG, "-c", fp],
            capture_output=True,
            encoding="utf-8",
            errors="replace",
            timeout=benchmark_timeout if any(kw in tf for kw in benchmark_keywords) else 30,
            cwd=os.path.dirname(CPLANG) if os.path.dirname(CPLANG) else "."
        )
        ok = (
            ("\u7f16\u8bd1\u6210\u529f" in r.stdout or "\u6267\u884c\u5b8c\u6210" in r.stdout)
            and "\u7f16\u8bd1\u5931\u8d25" not in r.stdout
            and "VM ERR" not in r.stdout
            and "\u9519\u8bef" not in r.stderr
        )
        if ok:
            passed += 1
            print(f"  OK  {tf}", flush=True)
        else:
            failed += 1
            errors.append(tf)
            print(f"  FAIL {tf}", flush=True)
            if r.stdout.strip():
                for line in r.stdout.strip().split("\n")[-3:]:
                    if line.strip():
                        print(f"       {line}", flush=True)
            if r.stderr.strip():
                for line in r.stderr.strip().split("\n")[-3:]:
                    if line.strip():
                        print(f"       {line}", flush=True)
    except subprocess.TimeoutExpired:
        failed += 1
        errors.append(tf)
        print(f"  TIMEOUT {tf}", flush=True)
    except Exception as e:
        failed += 1
        errors.append(tf)
        print(f"  ERR {tf}: {e}", flush=True)

print(f"\n=== {passed}/{len(test_files)} passed, {failed} failed ===", flush=True)
if errors:
    print("Failures:", ", ".join(errors), flush=True)
sys.exit(0 if failed == 0 else 1)
