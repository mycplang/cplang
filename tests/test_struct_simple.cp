// 测试结构体类型系统 - 简化版

struct Point {
    x;
    y;
};

func main() {
    // 创建结构体实例（使用Table模拟）
    var p = {};
    p.x = 10;
    p.y = 20;
    
    // 访问成员
    print(p.x);
    print(p.y);
    
    // 修改成员
    p.x = 100;
    p.y = 200;
    
    print(p.x);
    print(p.y);
    
    print("结构体测试完成");
}
