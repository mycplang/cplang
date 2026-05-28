// 数组索引测试

函数 数组求和(arr, n) {
    变量 sum = 0;
    变量 i = 0;
    当 (i < n) {
        sum = sum + arr[i];
        i = i + 1;
    }
    返回 sum;
}

函数 数组最大值(arr, n) {
    变量 max = arr[0];
    变量 i = 1;
    当 (i < n) {
        如果 (arr[i] > max) {
            max = arr[i];
        }
        i = i + 1;
    }
    返回 max;
}

函数 主() {
    返回 0;
}
