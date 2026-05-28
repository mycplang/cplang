// 嵌套结构体测试

struct Point {
    x;
    y;
};

struct Rect {
    topLeft;
    bottomRight;
};

func main() {
    // 嵌套结构体字面量
    var r = Rect{
        topLeft: Point{x: 0, y: 0},
        bottomRight: Point{x: 100, y: 100}
    };
    
    // 访问嵌套字段
    print(r.topLeft.x);
    print(r.topLeft.y);
    print(r.bottomRight.x);
    print(r.bottomRight.y);
    
    print("嵌套结构体测试完成");
}
