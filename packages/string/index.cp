函数 反转(s) {
    结果 = "";
    i = strlen(s) - 1;
    while (i >= 0) {
        结果 = 结果 + substr(s, i, 1);
        i = i - 1;
    }
    返回 结果;
}

函数 重复(s, n) {
    结果 = ""; i = 0;
    while (i < n) {
        结果 = 结果 + s;
        i = i + 1;
    }
    返回 结果;
}
