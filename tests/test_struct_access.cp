// 结构体测试 - 带成员访问

struct Point {
    x;
    y;
};

func main() {
    var p = Point{x: 10, y: 20};
    var x = p.x;
    print(x);
}
