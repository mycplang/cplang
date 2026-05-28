// 新标准库功能测试：forEach, transform, channelTryRecv, channelSelect

// ===== forEach / transform =====
函数 累加(x) { 全局变量 sum; sum = sum + x; }

函数 test_forEach() {
    变量 arr = [1, 2, 3, 4, 5];
    全局变量 sum = 0;
    遍历(arr, 累加);
    返回 sum == 15;
}

函数 乘十(x) { 返回 x * 10; }

函数 test_transform() {
    变量 arr = [1, 2, 3];
    变量 result = 数组变换(arr, 乘十);
    返回 (result[0] == 10) 且 (result[1] == 20) 且 (result[2] == 30);
}

// ===== 通道 tryRecv =====
函数 test_channel_tryrecv() {
    变量 ch = 通道创建(1);
    变量 ok = 通道发送(ch, 42);
    如果 (非 ok) { 返回 0; }
    变量 r = 通道尝试接收(ch);
    返回 (r[0] == 真) 且 (r[1] == 42);
}

// ===== 通道 select =====
函数 test_channel_select() {
    变量 ch1 = 通道创建(1);
    变量 ch2 = 通道创建(1);
    通道发送(ch1, 100);
    通道发送(ch2, 200);
    变量 sel = 通道选择([ch1, ch2], 100);
    返回 (sel[0] == 0 且 sel[1] == 100) 或 (sel[0] == 1 且 sel[1] == 200);
}

// ===== 通道 select 超时 =====
函数 test_channel_select_timeout() {
    变量 ch = 通道创建(1);
    变量 sel = 通道选择([ch], 10);
    返回 长度(sel) == 0;
}

// ===== 主函数 =====
函数 main() {
    变量 ok = 真;

    打印("1. forEach: ");
    变量 t1 = test_forEach();
    打印(t1 ? "通过" : "失败");
    ok = ok 且 t1;

    打印("2. transform: ");
    变量 t2 = test_transform();
    打印(t2 ? "通过" : "失败");
    ok = ok 且 t2;

    打印("3. channelTryRecv: ");
    变量 t3 = test_channel_tryrecv();
    打印(t3 ? "通过" : "失败");
    ok = ok 且 t3;

    打印("4. channelSelect (有数据): ");
    变量 t4 = test_channel_select();
    打印(t4 ? "通过" : "失败");
    ok = ok 且 t4;

    打印("5. channelSelect (超时): ");
    变量 t5 = test_channel_select_timeout();
    打印(t5 ? "通过" : "失败");
    ok = ok 且 t5;

    打印("========");
    如果 (ok) {
        打印("全部通过!");
    } 否则 {
        打印("有测试失败!");
    }

    返回 ok ? 0 : 1;
}