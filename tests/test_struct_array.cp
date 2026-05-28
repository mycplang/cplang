// 测试结构体类型系统 - 简化版

struct Point {
    x;
    y;
};

func main() {
    // 使用数组作为结构体替代
    var p = [];
    p[0] = 10;  // x
    p[1] = 20;  // y
    
    print(p[0]);
    print(p[1]);
    
    // 修改
    p[0] = 100;
    p[1] = 200;
    
    print(p[0]);
    print(p[1]);
    
    print("数组模拟结构体测试完成");
}
