// 复数运算测试
打印("=== Complex ===");

// 创建复数
变量 c1 = 复数新建(3, 4);
打印("c1: ", jsonStringify(c1));

// 实部/虚部
打印("real: ", 复数实部(c1));
打印("imag: ", 复数虚部(c1));

// 模 = sqrt(9+16) = 5
变量 absVal = 复数模(c1);
打印("abs: ", absVal);

// 加法 (3+4i) + (1+2i) = 4+6i
变量 c2 = 复数新建(1, 2);
变量 sum = 复数加(c1, c2);
打印("add: ", jsonStringify(sum));

// 减法 (3+4i) - (1+2i) = 2+2i
变量 sub = 复数减(c1, c2);
打印("sub: ", jsonStringify(sub));

// 乘法 (3+4i)*(1+2i) = 3+6i+4i+8i² = -5+10i
变量 mul = 复数乘(c1, c2);
打印("mul: ", jsonStringify(mul));

// 除法
变量 div = 复数除(c1, c2);
打印("div: ", jsonStringify(div));

// 共轭 conj(3+4i) = 3-4i
变量 conj = 复数共轭(c1);
打印("conj: ", jsonStringify(conj));

// 辐角
变量 arg = 复数辐角(c1);
打印("arg: ", arg);

打印("COMPLEX_OK");