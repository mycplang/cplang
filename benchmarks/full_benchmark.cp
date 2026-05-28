// CP语言完整性能基准测试
// 测试解释执行、优化执行、JIT执行等不同模式下的性能

// 时间测量辅助
var start_time = 0
var end_time = 0

function time_start() {
    start_time = time_now()
}

function time_end() {
    end_time = time_now()
    return end_time - start_time
}

// 1. fibonacci递归 - 函数调用测试
function fib_rec(n) {
    if (n <= 1) {
        return n
    }
    return fib_rec(n - 1) + fib_rec(n - 2)
}

// 2. fibonacci迭代 - 循环测试
function fib_iter(n) {
    if (n <= 1) { return n }
    var a = 0
    var b = 1
    var i = 2
    while (i <= n) {
        var c = a + b
        a = b
        b = c
        i = i + 1
    }
    return b
}

// 3. 素数计数 - 数学运算测试
function is_prime(n) {
    if (n < 2) { return 0 }
    if (n == 2) { return 1 }
    if (n % 2 == 0) { return 0 }
    var i = 3
    while (i * i <= n) {
        if (n % i == 0) {
            return 0
        }
        i = i + 2
    }
    return 1
}

function count_primes(limit) {
    var count = 0
    var i = 0
    while (i < limit) {
        if (is_prime(i) == 1) {
            count = count + 1
        }
        i = i + 1
    }
    return count
}

// 4. 数组操作 - 内存访问和存储测试
function array_ops(size) {
    var arr = []
    var i = 0
    while (i < size) {
        append(arr, i)
        i = i + 1
    }
    var sum = 0
    i = 0
    while (i < size) {
        sum = sum + arr[i]
        i = i + 1
    }
    return sum
}

// 5. 有序集合操作
function ordered_set_test() {
    var set = osNew()
    var i = 0
    while (i < 10000) {
        osInsert(set, i)
        i = i + 1
    }
    var count = 0
    i = 0
    while (i < 10000) {
        if (osContains(set, i) == 1) {
            count = count + 1
        }
        i = i + 1
    }
    return count
}

// 6. 字符串插值测试
function string_interpolation_test() {
    var result = ""
    var i = 0
    while (i < 1000) {
        var s = strInterpolate("Hello ${0}!", i)
        result = strConcat(result, s)
        i = i + 1
    }
    return strLen(result)
}

// 7. 排序算法测试
function sort_test() {
    var arr = []
    var i = 9999
    while (i >= 0) {
        append(arr, i)
        i = i - 1
    }
    sort(arr)
    return arr[0] + arr[9999]
}

// 测试运行
print("=== CP语言完整性能基准测试 ===\n")

// 预热
print("预热中...\n")
fib_iter(30)
count_primes(100)

// 开始正式测试
print("\n--- 测试开始 ---\n")

// 1. fibonacci迭代
print("1. Fibonacci迭代 (n=40):")
time_start()
var res = fib_iter(40)
var t = time_end()
print("   结果: " + res + ", 时间: " + t + "ms\n")

// 2. fibonacci递归
print("2. Fibonacci递归 (n=30):")
time_start()
res = fib_rec(30)
t = time_end()
print("   结果: " + res + ", 时间: " + t + "ms\n")

// 3. 素数计数
print("3. 素数计数 (到10000):")
time_start()
res = count_primes(10000)
t = time_end()
print("   结果: " + res + "个, 时间: " + t + "ms\n")

// 4. 数组操作
print("4. 数组操作 (大小=50000):")
time_start()
res = array_ops(50000)
t = time_end()
print("   结果: sum=" + res + ", 时间: " + t + "ms\n")

// 5. 有序集合
print("5. 有序集合测试:")
time_start()
res = ordered_set_test()
t = time_end()
print("   结果: count=" + res + ", 时间: " + t + "ms\n")

// 6. 字符串插值
print("6. 字符串插值测试:")
time_start()
res = string_interpolation_test()
t = time_end()
print("   结果: len=" + res + ", 时间: " + t + "ms\n")

// 7. 排序
print("7. 排序测试 (10000个元素):")
time_start()
res = sort_test()
t = time_end()
print("   结果: first+last=" + res + ", 时间: " + t + "ms\n")

print("\n=== 测试完成 ===")

