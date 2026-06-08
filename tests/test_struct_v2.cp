struct Point { int x; int y; }
struct Rect { Point topLeft; Point bottomRight; }
func main() {
    var r = Rect{topLeft: Point{x:42,y:1}, bottomRight: Point{x:100,y:100}};
    print(r.topLeft.x);
}
