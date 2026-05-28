# -*- coding: utf-8 -*-
"""CP语言环境工具下载脚本
用途: 下载 MinGit (Git Portable) + LLVM 完整开发包
"""

import urllib.request
import os
import sys

OUT_DIR = os.path.expandvars(r"%USERPROFILE%\Downloads")

DOWNLOADS = [
    # (name, url, description)
    ("MinGit-2.49.0-64-bit.zip",
     "https://registry.npmmirror.com/-/binary/git-for-windows/v2.49.0.windows.1/MinGit-2.49.0-64-bit.zip",
     "MinGit (47MB) - 精简版 Git, 可便携使用, 无需管理员权限"),

    ("LLVM-18.1.8-win64.exe",
     "https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.8/LLVM-18.1.8-win64.exe",
     "LLVM 18.1.8 (600MB) - 完整开发包, 含 C++ 头文件和库, JIT 编译需要"),
]

def download(url, dest, desc):
    if os.path.exists(dest):
        size_mb = os.path.getsize(dest) / (1024*1024)
        print(f"  [跳过] 已存在 ({size_mb:.1f} MB)")
        return True

    print(f"  下载 {desc}")
    print(f"  URL: {url}")

    try:
        def progress(block_num, block_size, total_size):
            downloaded = block_num * block_size
            if total_size > 0:
                pct = min(100, downloaded * 100 // total_size)
                mb = downloaded / (1024*1024)
                total_mb = total_size / (1024*1024)
                print(f"\r  进度: {pct}% ({mb:.1f}/{total_mb:.1f} MB)", end="")

        urllib.request.urlretrieve(url, dest, reporthook=progress)
        print()
        print(f"  OK -> {dest}")
        return True
    except Exception as e:
        print(f"\n  失败: {e}")
        print(f"  请手动下载: {url}")
        return False

def main():
    print("=" * 60)
    print("CP语言环境工具下载器")
    print("=" * 60)
    print()

    os.makedirs(OUT_DIR, exist_ok=True)

    for fname, url, desc in DOWNLOADS:
        dest = os.path.join(OUT_DIR, fname)
        print(f"[{fname}]")
        download(url, dest, desc)
        print()

    print("=" * 60)
    print("安装说明:")
    print("  Git: 解压 MinGit zip 到 C:\\tools\\git, 加入 PATH")
    print("  LLVM: 双击 LLVM-18.1.8-win64.exe 安装 (勾选 Add to PATH)")
    print("=" * 60)

if __name__ == "__main__":
    main()