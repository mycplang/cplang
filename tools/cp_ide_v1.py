#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CP IDE v2.0 - Python/tkinter"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import os, subprocess, re, sys, threading, json

CP_KEYWORDS = sorted([
    "函数","func","变量","var","常量","const",
    "如果","if","否则","else","当","while","循环","for",
    "遍历","foreach","返回","return","跳出","break","继续","continue",
    "选择","switch","情况","case","其他","default",
    "类","class","结构","struct","枚举","enum","接口","interface",
    "新建","new","公有","public","私有","private","保护","protected",
    "真","true","假","false","空","nil","null",
    "导入","import","类型","typedef",
    "抛出","throw","尝试","try","捕获","catch",
    "推迟","defer","信任","trust","为","as","删除","delete",
    "是","is","不是","typeof",
    "打印","print","println","长度","len","size","sizeof",
], key=len, reverse=True)

CP_TYPES = sorted([
    "整数","布尔","浮点","文本","变体","字节","数组","表",
    "int","bool","float","string","var","auto","void",
    "i8","i16","i32","i64","u8","u16","u32","u64",
    "f32","f64","char","byte",
], key=len, reverse=True)

KW_RE = re.compile("|".join(rf"\\b{re.escape(k)}\\b" for k in CP_KEYWORDS))
TYPE_RE = re.compile("|".join(rf"\\b{re.escape(t)}\\b" for t in CP_TYPES))



# -- test --

class CPIDE:
    def __init__(self, root):
        self.root = root
