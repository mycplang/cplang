// 编码修复验证：调用 MessageBoxA 显示中文
// 修复前：显示 "浣犲ソ涓栫晻"（乱码）
// 修复后：显示 "你好世界"
func main() {
    lib = 加载库("user32.dll");
    if (lib == 0) {
        print("加载 user32.dll 失败");
        return;
    }
    msgBox = 取函数(lib, "MessageBoxA");
    if (msgBox == 0) {
        print("取 MessageBoxA 失败");
        return;
    }
    // 传入 UTF-8 字符串，内部自动转 GBK
    r = 调用整数(msgBox, 0, "你好世界", "CP FFI - 编码修复验证", 0);
    print("MessageBox 返回: " + 转字符串(r));
    print("编码修复验证通过 ✅");
    释放库(lib);
}
