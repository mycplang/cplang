函数 是偶数(n) { 返回 n % 2 == 0; }
函数 大于五(n) { 返回 n > 5; }
函数 平方(n) { 返回 n * n; }

变量 arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

变量 r = arrFilter(arr, 是偶数);
打印("filter偶数:", arrlen(r));

打印("countIf>5:", countIf(arr, 大于五));

打印("findIf偶数:", findIf(arr, 是偶数));

r = transformArr(arr, 平方);
打印("transform:", arrlen(r));

打印("CP_PRED_OK");
