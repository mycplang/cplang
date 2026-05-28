// 测试结构体类型系统

struct Point {
    int x;
    int y;
}

struct Rect {
    Point topLeft;
    Point bottomRight;
}

func main() {
    // 创建结构体变量
    var p = Point{x: 10, y: 20};
    
    // 访问成员
    print(p.x);
    print(p.y);
    
    // 修改成员
    p.x = 100;
    p.y = 200;
    
    print(p.x);
    print(p.y);
    
    // 嵌套结构体
    var r = Rect{
        topLeft: Point{x: 0, y: 0},
        bottomRight: Point{x: 100, y: 100}
    };
    
    print(r.topLeft.x);
    print(r.bottomRight.y);
}
