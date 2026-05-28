#!/usr/bin/env python3
"""StdLib Progress Dashboard — cplang 项目标准库进度可视化"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).parent.parent
FILE  = ROOT / "src" / "stdlib" / "stdlib.cpp"
HPP   = ROOT / "include" / "stdlib" / "stdlib.hpp"

# ── 模块定义 ─────────────────────────────────────────────────────────
MODULES = {
    "Math":    ("math::",       ["abs","sqrt","pow","floor","ceil","round","random",
                                  "max","min","sin","cos","tan","asin","acos","atan",
                                  "atan2","log","log10","exp","pi","e","clamp","lerp",
                                  "degToRad","radToDeg"]),
    "String":  ("string::",     ["len","substr","concat","find","replace","split","join",
                                  "trim","lower","upper","startsWith","endsWith","contains",
                                  "format","repeat","reverse","padLeft","padRight"]),
    "Array":   ("array::",      ["len","push","pop","shift","unshift","insert","remove",
                                  "slice","splice","reverse","sort","map","filter","reduce",
                                  "find","findIndex","includes","indexOf","lastIndexOf",
                                  "fill","copy","flatten","unique","zip","unzip"]),
    "Table":   ("table::",      ["len","keys","values","entries","has","delete_","clear",
                                  "merge","clone","toArray","fromArray"]),
    "IO":      ("io::",         ["print","println","input","readFile","writeFile","appendFile",
                                  "exists","deleteFile","copyFile","moveFile","mkdir","rmdir",
                                  "listDir","isFile","isDir"]),
    "Time":    ("time::",       ["now","date","format","parse","sleep","tick"]),
    "System":  ("system::",    ["exit","getEnv","setEnv","exec","spawn","platform","arch",
                                  "pid","cwd","chdir"]),
    "Types":   ("types::",      ["isNil","isBool","isInt","isFloat","isNumber","isString",
                                  "isArray","isTable","isFunction","isObject","typeOf",
                                  "toString","toInt","toFloat","toBool"]),
    "File":    ("file::",       ["read","write","append","exists","delete_","copy","move",
                                  "mkdir","rmdir","listDir","isFile","isDir","size","time"]),
    "Network": ("network::",   ["httpGet","httpPost","tcpConnect","udpSocket","resolve",
                                  "download","upload"]),
    "JSON":    ("json::",       ["parse","stringify","validate","path"]),
    "Regex":   ("regex::",      ["match","search","replace","split","test"]),
    "Crypto":  ("crypto::",    ["md5","sha1","sha256","base64Encode","base64Decode",
                                  "aesEncrypt","aesDecrypt","randomBytes"]),
    "Encoding":("encoding::",  ["urlEncode","urlDecode","htmlEncode","htmlDecode",
                                  "utf8ToAnsi","ansiToUtf8"]),
    "Reflection":("reflection::",["typeof","getProp","setProp","hasProp","call",
                                   "apply","bind","construct","methods","properties"]),
}

TICK = "✅"
CROSS = "❌"
PART  = "🔄"

def get_defined_functions(src: str) -> set:
    """从源码中提取所有已实现的函数名（namespace::func格式）"""
    pattern = re.compile(r'^Value\s+(\w+)\s*::\s*(\w+)\s*\(', re.MULTILINE)
    return {f"{m}::{n}" for m, n in pattern.findall(src)}

def get_declared_functions(hpp: str) -> dict[str, set]:
    """从头文件中提取各模块声明的函数名"""
    result = {}
    current_ns = None
    # 找 namespace xxx { 块
    ns_blocks = re.findall(
        r'namespace\s+(\w+)\s*\{([^}]+)\}',
        hpp, re.DOTALL
    )
    for ns, body in ns_blocks:
        funcs = re.findall(r'Value\s+(\w+)\s*\(\s*std::vector<Value>', body)
        result[ns] = set(funcs)
    return result

def render(src: str, hpp_decl: dict[str, set]) -> str:
    defined = get_defined_functions(src)
    total_defined = len(defined)

    lines = []
    lines.append("═" * 78)
    lines.append("  StdLib Progress — cplang 编译器标准库")
    lines.append("═" * 78)
    lines.append(f"  文件: {FILE}")
    lines.append(f"  总行数: {len(src.splitlines())} 行")
    lines.append(f"  已实现函数: {total_defined} 个")
    lines.append("═" * 78)

    grand_total = 0
    grand_impl = 0

    for name, (ns, func_list) in MODULES.items():
        total    = len(func_list)
        impl_set = {f"{ns}{f}" for f in func_list if f in ["delete_"]}
        # 按后缀匹配（delete_ 在源码中仍是 delete_）
        impl_set = {f"{ns}{f}" for f in func_list}
        impl_funcs = defined & impl_set
        impl      = len(impl_funcs)
        missing   = [f for f in func_list if f"{ns}{f}" not in defined]
        pct       = impl / total * 100 if total else 0
        grand_total += total
        grand_impl  += impl

        bar_len  = int(pct / 5)
        bar      = "█" * bar_len + "░" * (20 - bar_len)
        status   = TICK if impl == total else PART if impl > 0 else CROSS

        lines.append(f"\n  {status} [{name}]  {impl}/{total}  {bar}  {pct:.0f}%")

        if missing:
            lines.append(f"      缺失: {', '.join(missing[:8])}{'...' if len(missing)>8 else ''}")

    lines.append("\n" + "═" * 78)
    lines.append(f"  总体进度: {grand_impl}/{grand_total}  "
                 f"({grand_impl/grand_total*100:.1f}%)")
    # 估算完成度（已有类别按比例，未有类别计0）
    lines.append("═" * 78)
    return "\n".join(lines)

def main():
    if not FILE.exists():
        print(f"❌ 文件不存在: {FILE}", file=sys.stderr)
        sys.exit(1)

    src = FILE.read_text(encoding="utf-8", errors="replace")
    hpp = HPP.read_text(encoding="utf-8", errors="replace") if HPP.exists() else ""
    print(render(src, {}))

if __name__ == "__main__":
    main()