#!/usr/bin/env python3
"""AOT 扫测运行器 — 测试所有 .cp 文件的 AOT 编译和运行"""
import subprocess, os, sys, tempfile, shutil

# 配置
if len(sys.argv) > 1:
    CPLANG = os.path.abspath(sys.argv[1])
else:
    CPLANG = os.environ.get("CPLANG_BIN")
    if CPLANG:
        CPLANG = os.path.abspath(CPLANG)
    else:
        # do not hardcode relative build path — resolve from script location
        import inspect
        _script_dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
        CPLANG = os.path.abspath(os.path.join(_script_dir, "..", "build", "cplang.exe"))

TEST_DIR = os.path.dirname(os.path.abspath(__file__))
CPLANG_DIR = os.path.dirname(CPLANG)
OUT_DIR = os.path.join(CPLANG_DIR, "aot_scan")
REPORT_FILE = os.path.join(OUT_DIR, "aot_report.txt")

# 已知不适用于 AOT 的测试（raylib 依赖、系统级依赖等）
skip = {
    "circular_a.cp", "circular_b.cp",  # 循环导入
    "test_import_real.cp", "test_slot_main.cp",  # 系统级问题
    # raylib 依赖（含未暴露命名规则的 raylib 测试）
    "test_raylib.cp", "test_raylib_basics.cp", "test_raylib_debug.cp",
    "test_raylib_english.cp", "test_raylib_minimal.cp", "test_raylib_probe.cp",
    "test_raylib_single.cp",
    "test_begindraw.cp", "test_concat.cp",
    "test_drawfps.cp", "test_drawtext.cp",
    "test_dt_after_fps.cp", "test_dt_debug.cp", "test_dt_fixed.cp", "test_dt_min.cp",
    "test_fontdebug.cp", "test_full.cp", "test_gameloop_trace.cp",
    "test_minfont.cp",
    "test_onerender.cp", "test_onerender_en.cp", "test_onerender_println.cp",
    "test_render_full.cp", "test_render_t.cp",
    "test_rl_bare_loop.cp", "test_rl_loop_min.cp", "test_rl_min.cp", "test_rl_render_min.cp",
    "test_shapeonly.cp", "test_simple_game.cp", "test_snake_loop.cp",
    "test_step1.cp", "test_step5.cp",
    "test_while_int.cp", "test_window.cp", "test_window2.cp",
    # 线程安全（AOT 运行时暂未处理）
    "test_threading.cp", "test_threading2.cp",
    # WebSocket（DLL 依赖）
    "test_websocket.cp",
}

# 超时设置
AOT_TIMEOUT = 60  # 单次 AOT 编译+运行超时
BENCHMARK_TIMEOUT = 120
benchmark_keywords = {"bench", "perf", "pure", "loop"}

# 查找测试文件
test_files = sorted(
    f for f in os.listdir(TEST_DIR)
    if f.endswith(".cp") and not f.startswith("_") and f not in skip
)

if not os.path.exists(CPLANG):
    print(f"ERROR: cplang binary not found: {CPLANG}", flush=True)
    sys.exit(1)

# 清理输出目录（跳过锁定的文件）
if os.path.exists(OUT_DIR):
    for _ in range(3):  # 最多重试 3 次
        try:
            shutil.rmtree(OUT_DIR)
            break
        except PermissionError:
            import time; time.sleep(1)
            continue
os.makedirs(OUT_DIR, exist_ok=True)

print(f"=== AOT Scan ({len(test_files)} tests) ===", flush=True)
print(f"Binary: {CPLANG}", flush=True)
print(f"Output: {OUT_DIR}", flush=True)
print(f"Skipped: {len(skip)} tests ({', '.join(sorted(skip))})", flush=True)
print(flush=True)

passed = 0
failed = []
timeouts = []
binary_fails = []  # compile succeeded, binary run failed
skipped_count = len([f for f in os.listdir(TEST_DIR) if f.endswith(".cp") and not f.startswith("_") and f in skip])

for tf in test_files:
    fp = os.path.join(TEST_DIR, tf)
    base = tf.replace(".cp", "")
    exe_path = os.path.join(OUT_DIR, f"{base}.exe")
    timeout = BENCHMARK_TIMEOUT if any(kw in tf for kw in benchmark_keywords) else AOT_TIMEOUT

    # === Phase 1: AOT Compile ===
    try:
        r = subprocess.run(
            [CPLANG, "-a", fp, "-o", exe_path],
            capture_output=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout,
            cwd=CPLANG_DIR if CPLANG_DIR else "."
        )
    except subprocess.TimeoutExpired:
        timeouts.append(tf)
        print(f"  TIMEOUT {tf} (compile)", flush=True)
        continue
    except Exception as e:
        failed.append((tf, f"exception: {e}"))
        print(f"  ERR {tf}: {e}", flush=True)
        continue

    compile_ok = r.returncode == 0 and os.path.exists(exe_path)

    # 自动分类编译失败原因
    if not compile_ok:
        reason = "compile failed"
        combined = r.stdout + r.stderr
        if "closeWindow" in combined or "__u5173" in combined:
            reason = "raylib dep"
            reason_type = "raylib"
        elif "无法解析的外部符号" in combined:
            reason = "unresolved stdlib sym"
            reason_type = "stdlib"
        else:
            reason_type = "other"
        failed.append((tf, reason))
        if reason_type != "raylib":  # hide raylib noise from output
            print(f"  FAIL {tf} ({reason})", flush=True)
            for line in r.stdout.strip().split("\n")[-3:]:
                if line.strip():
                    print(f"       {line}", flush=True)
            for line in r.stderr.strip().split("\n")[-3:]:
                if line.strip():
                    print(f"       {line}", flush=True)
        continue

    # === Phase 2: Run ===
    try:
        r2 = subprocess.run(
            [exe_path],
            capture_output=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout,
            cwd=OUT_DIR
        )
    except subprocess.TimeoutExpired:
        binary_fails.append(tf)
        print(f"  TIMEOUT {tf} (run)", flush=True)
        # 清理残留 .exe 进程
        continue
    except Exception as e:
        binary_fails.append(tf)
        print(f"  FAIL {tf} (run exception: {e})", flush=True)
        continue

    # 检查二进制运行结果
    run_ok = r2.returncode == 0
    if run_ok:
        passed += 1
        print(f"  OK  {tf}", flush=True)
    else:
        binary_fails.append(tf)
        print(f"  FAIL {tf} (run exit={r2.returncode})", flush=True)
        for line in r2.stdout.strip().split("\n")[-3:]:
            if line.strip():
                print(f"       out: {line}", flush=True)
        for line in r2.stderr.strip().split("\n")[-3:]:
            if line.strip():
                print(f"       err: {line}", flush=True)

    # 清理运行后的 exe（避免文件锁膨胀）
    try:
        os.unlink(exe_path)
    except PermissionError:
        pass

# === 分类统计 ===
raylib_fails = [f for f, r in failed if 'raylib' in r]
stdlib_fails = [f for f, r in failed if 'unresolved' in r]
other_compile_fails = [f for f, r in failed if 'raylib' not in r and 'unresolved' not in r]

# === Summary ===
print(f"\n=== AOT 扫测完成 ===", flush=True)
print(f"  PASS:          {passed}/{len(test_files)}", flush=True)
total_fail = len(failed) + len(timeouts) + len(binary_fails)
print(f"  FAIL:          {total_fail}", flush=True)
print(f"    ├─ stdlib:   {len(stdlib_fails)} (standalone runtime 未实现的标准库函数)", flush=True)
print(f"    ├─ raylib:   {len(raylib_fails)} (无法链接 raylib DLL)", flush=True)
print(f"    ├─ compile:  {len(other_compile_fails)} (其他编译错误)", flush=True)
print(f"    ├─ timeout:  {len(timeouts)}", flush=True)
print(f"    └─ run:      {len(binary_fails)} (编译成功但运行失败)", flush=True)
print(f"  SKIP:          {skipped_count}", flush=True)

# 生成报告文件
with open(REPORT_FILE, "w", encoding="utf-8") as f:
    f.write(f"AOT Scan Report\n")
    f.write(f"Date: {__import__('datetime').datetime.now().isoformat()}\n")
    f.write(f"Binary: {CPLANG}\n")
    f.write(f"Total: {len(test_files)}, Passed: {passed}, Failed: {total_fail}, Skipped: {skipped_count}\n\n")
    if failed:
        f.write("=== Compile Failures ===\n")
        for tf, reason in failed:
            f.write(f"  {tf}: {reason}\n")
    if timeouts:
        f.write("\n=== Compile Timeouts ===\n")
        for tf in timeouts:
            f.write(f"  {tf}\n")
    if binary_fails:
        f.write("\n=== Run Failures ===\n")
        for tf in binary_fails:
            f.write(f"  {tf}\n")
    f.write("\n=== All Results ===\n")
    for tf in test_files:
        if tf not in binary_fails and tf not in timeouts and not any(t[0] == tf for t in failed):
            f.write(f"  PASS {tf}\n")
        else:
            f.write(f"  FAIL {tf}\n")

print(flush=True)
print(f"Report: {REPORT_FILE}", flush=True)
sys.exit(0 if total_fail == 0 else 1)
