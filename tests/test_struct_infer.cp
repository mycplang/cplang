// 测试结构体类型推导

// 1. 显式类型
struct Point1 {
    int x;
    int y;
}

// 2. 类型推导（从初始值）
struct Point2 {
    x = 0;      // 推导为 int
    y = 0;      // 推导为 int
}

// 3. 混合使用
struct Point3 {
    int x;      // 显式类型
    y = 0;      // 类型推导
}

// 4. 不同类型推导
struct Data {
    count = 10;         // int
    price = 99.9;       // float
    name = "test";      // string
    flag = true;        // bool
}

// 主函数
func main() {
    // 测试显式类型
    var p1 = Point1{x: 10, y: 20};
    print(p1.x);
    print(p1.y);
    
    // 测试类型推导
    var p2 = Point2{x: 100, y: 200};
    print(p2.x);
    print(p2.y);
    
    // 测试混合
    var p3 = Point3{x: 5, y: 15};
    print(p3.x);
    print(p3.y);
    
    // 测试不同类型
    var d = Data{count: 1, price: 19.99, name: "hello", flag: true};
    print(d.count);
    print(d.price);
    print(d.name);
    print(d.flag);
    
    print("结构体类型推导测试完成");
}
