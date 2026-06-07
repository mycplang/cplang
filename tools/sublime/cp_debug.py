#!/usr/bin/env python3
"""CP Language — Sublime 一键调试启动器
同时启动编译器调试服务器 + 交互客户端"""

import subprocess, socket, json, os, sys, time, threading

CPLANG = os.path.join(os.environ.get('CPLANG_HOME', 'C:/cplang'), 'build', 'cplang.exe')

def debug_run(cp_file, breakpoints=None):
    # 清理旧进程
    subprocess.run(['taskkill', '/F', '/IM', 'cplang.exe'], capture_output=True)
    time.sleep(0.3)

    # 启动编译器
    proc = subprocess.Popen(
        [CPLANG, '--debug-server', '4711', '-c', cp_file],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1
    )

    # 转发 stdout
    def forward_output():
        for line in proc.stdout:
            print(line.rstrip())
    t = threading.Thread(target=forward_output, daemon=True)
    t.start()

    # 连接
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    for i in range(20):
        try:
            sock.connect(('127.0.0.1', 4711))
            break
        except:
            time.sleep(0.3)
    else:
        print("❌ 无法连接调试服务器")
        proc.kill(); return

    def send(cmd): sock.sendall((json.dumps(cmd) + '\n').encode())
    def recv():
        data = b''
        while True:
            c = sock.recv(1)
            if not c or c == b'\n': break
            data += c
        return json.loads(data.decode()) if data else {}

    # Connected
    print(recv().get('type', ''))

    # 设置断点
    if breakpoints:
        send({'cmd': 'setBreakpoints', 'file': cp_file, 'lines': breakpoints})
        print(f"🔴 断点: {breakpoints}")

    print("\n══ CP 调试器 ══\nc=继续 s=单步 n=跳过 bt=堆栈 v=变量 q=退出\n")

    show_prompt = True
    while proc.poll() is None:
        try:
            sock.settimeout(0.3)
            msg = recv()
            if msg.get('type') == 'paused':
                print(f"\n⏸ {msg.get('reason')} | {os.path.basename(msg.get('file','?'))}:{msg.get('line',0)}")
                show_prompt = True

            if show_prompt:
                cmd = input("(dbg) ").strip()
                show_prompt = False
            else:
                continue

            if cmd in ('c', ''): send({'cmd': 'continue'}); show_prompt = True
            elif cmd in ('s',):   send({'cmd': 'stepInto'});  show_prompt = True
            elif cmd in ('n',):   send({'cmd': 'stepOver'});  show_prompt = True
            elif cmd in ('o',):   send({'cmd': 'stepOut'});   show_prompt = True
            elif cmd in ('bt',):
                send({'cmd': 'getStack'})
                for i, f in enumerate(recv().get('frames', [])):
                    print(f"  #{i} {f.get('name','?')} {f.get('file','?')}:{f.get('line',0)}")
                show_prompt = True
            elif cmd in ('v',):
                send({'cmd': 'getVars'})
                for n, v in recv().get('vars', {}).items():
                    print(f"  {n} = {v}")
                show_prompt = True
            elif cmd in ('q',): send({'cmd': 'continue'}); break
            else: print("?"); show_prompt = True
        except socket.timeout:
            pass
        except (EOFError, KeyboardInterrupt):
            break

    sock.close(); proc.kill()
    print("调试结束")

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("用法: cp_debug.py <file.cp> [行号,行号,...]")
        sys.exit(1)
    bps = [int(x) for x in sys.argv[2].split(',')] if len(sys.argv) > 2 else None
    debug_run(sys.argv[1], bps)
