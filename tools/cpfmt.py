#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CP 语言代码格式化工具 (cpfmt.py)
用法: python cpfmt.py [选项] <文件>
      python cpfmt.py < 源文件.cp > 输出.cp

选项:
  -i, --in-place   直接修改文件（原地格式化）
  -w, --indent N   缩进空格数（默认 4）
  -c, --check      只检查格式（不输出），格式有误时返回非零退出码
  -h, --help       显示帮助

格式化规则:
  • 4空格缩进
  • 关键字后加空格：`如果 (条件) {`
  • 运算符周围加空格：`a + b`
  • 函数左花括号在行末：`函数 foo() {`
  • 语句末尾加分号
  • 注释 `//` 后加空格
  • 函数之间空一行
"""

import sys
import re
import os
import io

# 确保 stdout/stderr 使用 UTF-8
if sys.stdout.encoding != 'utf-8':
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
if sys.stderr.encoding != 'utf-8':
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8')


# 关键字集合
ALL_KEYWORDS = {
    "函数", "变量", "常量", "返回", "如果", "否则", "当", "循环", "遍历",
    "跳出", "继续", "选择", "情况", "其他", "类", "接口", "枚举", "结构体",
    "新建", "公有", "私有", "保护", "真", "假", "空", "空值",
    "导入", "包名", "类型", "类型定义", "抛出", "尝试", "捕获", "推迟",
    "信任", "为", "作为", "删除", "是", "不是", "打印", "println",
    "并且", "或者", "非",
    "function", "fn", "var", "const", "return", "if", "else", "while",
    "for", "foreach", "break", "continue", "switch", "case", "default",
    "class", "interface", "enum", "struct", "new", "public", "private",
    "protected", "true", "false", "nil", "null", "import", "package",
    "typedef", "throw", "try", "catch", "defer", "trust", "as",
    "delete", "is", "typeof", "typeOf", "print", "println",
    "and", "or", "not",
}

MID_BLOCK_KEYWORDS = {"否则", "else", "捕获", "catch", "情况", "case", "其他", "default"}


def _strip_comment(line):
    """分离代码和行注释"""
    in_str = False
    str_ch = ''
    i = 0
    while i < len(line):
        ch = line[i]
        if in_str:
            if ch == '\\':
                i += 2
                continue
            if ch == str_ch:
                in_str = False
            i += 1
            continue
        if ch in ('"', "'"):
            in_str = True
            str_ch = ch
            i += 1
            continue
        if ch == '/' and i + 1 < len(line):
            if line[i+1] == '/':
                return line[:i].rstrip(), line[i:]
            if line[i+1] == '*':
                i += 2
                continue
        i += 1
    return line, None


class CPFormatter:
    """CP 语言代码格式化器 — 基于行的重构"""

    def __init__(self, indent_width=4):
        self.indent_width = indent_width
        self.indent_level = 0
        self.lines = []
        self.result = []

    def _get_indent(self):
        return ' ' * (self.indent_level * self.indent_width)

    def _count_braces(self, line):
        """计算一行中括号对缩进的影响"""
        inc = 0
        dec = 0
        in_str = False
        str_ch = ''
        i = 0
        while i < len(line):
            ch = line[i]
            if in_str:
                if ch == '\\':
                    i += 2
                    continue
                if ch == str_ch:
                    in_str = False
                i += 1
                continue
            if ch in ('"', "'"):
                in_str = True
                str_ch = ch
                i += 1
                continue
            if ch == '/' and i + 1 < len(line):
                if line[i+1] == '/':
                    break
                if line[i+1] == '*':
                    i += 2
                    continue
            if ch == '{':
                inc += 1
            elif ch == '}':
                dec += 1
            i += 1
        return inc, dec

    def _needs_semicolon(self, line):
        """判断行是否需要分号结尾"""
        stripped = line.strip()
        if not stripped:
            return False
        if stripped.endswith('{') or stripped.endswith('}') or stripped in ('{', '}'):
            return False
        if stripped.endswith(';') or stripped.endswith(':') or stripped.endswith(','):
            return False
        first_word = stripped.split()[0] if stripped.split() else ''
        control_kw = {"如果", "if", "否则", "else", "当", "while", "循环", "for",
                       "遍历", "foreach", "选择", "switch", "情况", "case",
                       "其他", "default", "尝试", "try", "捕获", "catch",
                       "信任", "trust"}
        if first_word in control_kw:
            return False
        if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
            return False
        return True

    def _add_semicolon(self, line):
        """在需要的地方添加分号"""
        code, comment = _strip_comment(line)
        if self._needs_semicolon(code):
            if comment:
                return code.rstrip() + ';  ' + comment.lstrip()
            else:
                return code.rstrip() + ';'
        return line

    def _normalize_spacing(self, line):
        """规范空格（字符串感知 — 字符串内部原样保留）"""
        code, comment = _strip_comment(line)
        if not code.strip():
            return line

        # 将代码分为「字符串外」和「字符串内」片段交替处理
        # 偶数索引 = 非字符串代码，奇数索引 = 字符串内容
        segments = []
        in_str = False
        str_ch = ''
        buf = []
        i = 0
        while i < len(code):
            ch = code[i]
            if in_str:
                if ch == '\\':
                    buf.append(ch)
                    i += 1
                    if i < len(code):
                        buf.append(code[i])
                        i += 1
                    continue
                if ch == str_ch:
                    buf.append(ch)
                    segments.append(''.join(buf))  # 字符串内容（含引号）
                    buf = []
                    in_str = False
                    i += 1
                    continue
                buf.append(ch)
                i += 1
                continue
            if ch in ('"', "'"):
                if buf:
                    segments.append(''.join(buf))  # 非字符串代码
                    buf = []
                buf.append(ch)
                in_str = True
                str_ch = ch
                i += 1
                continue
            buf.append(ch)
            i += 1
        if buf:
            segments.append(''.join(buf))

        # 偶数片段是代码（应用运算符空格），奇数片段是字符串（保持原样）
        processed = []
        for idx, seg in enumerate(segments):
            if idx % 2 == 0 and seg.strip():
                seg = self._apply_operator_spacing(seg)
            processed.append(seg)
        s = ''.join(processed)

        # 逗号后加空格
        s = re.sub(r',(\S)', r', \1', s)
        # 移除多余空格
        s = re.sub(r'  +', ' ', s)
        # 移除括号内多余空格: `( a` → `(a`, `a )` → `a)`
        s = re.sub(r'\(\s+', '(', s)
        s = re.sub(r'\s+\)', ')', s)
        s = re.sub(r'\[\s+', '[', s)
        s = re.sub(r'\s+\]', ']', s)
        s = s.rstrip()

        if comment:
            return s + '  ' + comment
        return s

    def _apply_operator_spacing(self, code):
        """对非字符串代码部分应用运算符空格"""
        if not code.strip():
            return code
        s = code

        # 控制流关键字与左括号之间加空格: `如果(` → `如果 (`
        ctrl_kw = ["如果", "if", "当", "while", "循环", "for", "选择", "switch",
                    "尝试", "try", "捕获", "catch", "遍历", "foreach"]
        for kw in ctrl_kw:
            s = re.sub(r'\b' + re.escape(kw) + r'\(', kw + ' (', s)

        # 运算符周围加空格（不破坏复合运算符）
        # 先保护复合运算符
        s = re.sub(r'(<=|>=|==|!=|\+=|-=|\*=|/=|%=|&=|\|=|\^=|<<=|>>=|&&|\|\||<<|>>|->|\.\.)', r' \1 ', s)
        # 单字符运算符周围加空格（多次执行直到稳定，以处理重叠匹配如 i+3-k）
        prev = None
        while prev != s:
            prev = s
            s = re.sub(r'(\S)([+\-*/%&|^<>=!])(\S)', r'\1 \2 \3', s)
        # 修复开头/结尾的运算符空格
        s = re.sub(r'\(\s*([+\-*/%&|^<>=!])', r'(\1', s)
        # 移除多余空格
        s = re.sub(r'  +', ' ', s)
        return s

    def format(self, source):
        """格式化完整源码"""
        # 移除 UTF-8 BOM (U+FEFF)
        if source and source[0] == '﻿':
            source = source[1:]
        self.lines = source.split('\n')
        self.result = []
        self.indent_level = 0
        i = 0

        while i < len(self.lines):
            raw_line = self.lines[i]
            stripped = raw_line.strip()

            # 空行保留（最多连续两个）
            if not stripped:
                if self.result and self.result[-1] != '':
                    self.result.append('')
                i += 1
                continue

            # 行注释
            if stripped.startswith('//'):
                self.result.append(self._get_indent() + stripped)
                i += 1
                continue

            # 块注释
            if stripped.startswith('/*') or stripped.startswith('*'):
                # 收集块注释所有行
                block_lines = []
                while i < len(self.lines):
                    bl = self.lines[i].strip()
                    block_lines.append(bl)
                    if '*/' in bl:
                        i += 1
                        break
                    i += 1
                # 输出块注释
                for bl in block_lines:
                    if bl:
                        self.result.append(self._get_indent() + bl)
                    else:
                        self.result.append('')
                continue

            # 计算大括号
            inc, dec = self._count_braces(stripped)

            # 如果以闭合括号开头，先减少缩进
            if stripped.startswith('}') or stripped.startswith(']'):
                self.indent_level = max(0, self.indent_level - 1)

            # 规范化空格
            formatted = self._normalize_spacing(stripped)

            # 添加分号
            formatted = self._add_semicolon(formatted)

            # 输出
            self.result.append(self._get_indent() + formatted)

            # 更新缩进
            # 对于以 } 开头的行，前导 } 的缩进已由第248行提前处理
            # 所以此处只处理剩余的括号变化
            if stripped.startswith('}') or stripped.startswith(']'):
                self.indent_level += inc
                if dec > 0:
                    self.indent_level -= (dec - 1)  # 前导 } 已处理
            else:
                self.indent_level += inc
                self.indent_level -= dec
            self.indent_level = max(0, self.indent_level)
            i += 1

        # 清理
        output = '\n'.join(self.result)
        output = re.sub(r'\n{3,}', '\n\n', output)  # 最多连续两个空行
        output = output.rstrip('\n') + '\n'
        return output


# ============================================================
# CLI
# ============================================================

def format_file(filepath, indent_width=4, in_place=False, check_only=False):
    """格式化一个文件"""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            source = f.read()
    except FileNotFoundError:
        print(f"错误: 找不到文件 '{filepath}'", file=sys.stderr)
        return False
    except Exception as e:
        print(f"错误: 读取文件失败: {e}", file=sys.stderr)
        return False

    formatter = CPFormatter(indent_width=indent_width)
    formatted = formatter.format(source)

    if check_only:
        # 比较时忽略 BOM 差异
        source_clean = source
        bom = '﻿'
        if source_clean.startswith(bom):
            source_clean = source_clean[len(bom):]
        if source_clean == formatted:
            return True
        else:
            print(f"{filepath}: 格式不符合规范", file=sys.stderr)
            return False

    if in_place:
        try:
            with open(filepath, 'w', encoding='utf-8', newline='') as f:
                f.write(formatted)
            print(f"已格式化: {filepath}", flush=True)
            return True
        except Exception as e:
            print(f"错误: 写入文件失败: {e}", file=sys.stderr)
            return False
    else:
        sys.stdout.buffer.write(formatted.encode('utf-8'))
        return True


def main():
    indent_width = 4
    in_place = False
    check_only = False
    files = []

    args = sys.argv[1:]
    i = 0
    while i < len(args):
        if args[i] in ('-i', '--in-place'):
            in_place = True
        elif args[i] in ('-c', '--check'):
            check_only = True
        elif args[i] in ('-w', '--indent'):
            i += 1
            if i < len(args):
                indent_width = int(args[i])
        elif args[i] in ('-h', '--help'):
            print(__doc__.strip())
            return 0
        else:
            files.append(args[i])
        i += 1

    if not files:
        source = sys.stdin.buffer.read().decode('utf-8')
        if not source.strip():
            print(__doc__.strip())
            return 0
        formatter = CPFormatter(indent_width=indent_width)
        result = formatter.format(source)
        sys.stdout.buffer.write(result.encode('utf-8'))
        return 0

    success = True
    for fp in files:
        if not format_file(fp, indent_width, in_place, check_only):
            success = False

    return 0 if success else 1


if __name__ == '__main__':
    sys.exit(main())
