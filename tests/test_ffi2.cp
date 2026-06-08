函数 main() {
    打印("测试加载 kernel32.dll");

    lib = 加载库("kernel32.dll");
    如果 (lib == 0) {
        打印("加载失败");
        返回;
    }
    打印("句柄: " + 转字符串(lib));

    // 测试 GetLastError — 这个函数几乎所有 Windows 程序都能调
    fn = 取函数(lib, "GetLastError");
    如果 (fn == 0) {
        打印("取 GetLastError 失败, 错误码: " + 转字符串(错误码()));
    } 否则 {
        打印("GetLastError 地址: " + 转字符串(fn));
    }

    // 测试 GetSystemInfo — 无参数，通过输出参数返回
    fn2 = 取函数(lib, "GetSystemInfo");
    如果 (fn2 == 0) {
        打印("取 GetSystemInfo 失败");
    } 否则 {
        打印("GetSystemInfo 地址: " + 转字符串(fn2));
    }

    // 测试 user32.dll
    lib2 = 加载库("user32.dll");
    如果 (lib2 != 0) {
        打印("user32 句柄: " + 转字符串(lib2));
        fn3 = 取函数(lib2, "GetSystemMetrics");
        如果 (fn3 == 0) {
            打印("取 GetSystemMetrics 失败, 错误码: " + 转字符串(错误码()));
        } 否则 {
            打印("GetSystemMetrics 地址: " + 转字符串(fn3));
            w = 调用整数(fn3, 0);
            h = 调用整数(fn3, 1);
            打印("屏幕: " + 转字符串(w) + "x" + 转字符串(h));
        }
        fn4 = 取函数(lib2, "MessageBoxA");
        如果 (fn4 == 0) {
            码 = 错误码();
            打印("取 MessageBoxA 失败, 错误码: " + 转字符串(码) + " = " + 错误信息(码));
        } 否则 {
            打印("MessageBoxA 地址: " + 转字符串(fn4));
            打印("准备弹窗...");
            结果 = 调用整数(fn4, 0, "你好世界", "CP FFI", 0);
            打印("结果: " + 转字符串(结果));
        }
        释放库(lib2);
    }

    释放库(lib);
    打印("测试完成");
}
