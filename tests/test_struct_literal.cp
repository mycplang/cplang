// 结构体字面量测试

struct Point {
    x;
    y;
};

func main() {
    // 使用结构体字面量创建实例
    var p = Point{x: 10, y: 20};
    
    // 访问字段
    print(p.x);
    print(p.y);
    
    // 创建另一个实例
    var p2 = Point{x: 100, y: 200};
    print(p2.x);
    print(p2.y);
    
    print("结构体字面量测试完成");
}
