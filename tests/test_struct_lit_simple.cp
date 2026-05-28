// 结构体字面量测试 - 简化

struct Point {
    x;
    y;
};

func main() {
    // 只创建，不访问
    var p = Point{x: 10, y: 20};
    print("结构体创建成功");
}
