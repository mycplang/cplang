struct Point { int x; int y; }
struct Wrapper { Point p; }
func main() {
    var w = Wrapper{p: Point{x: 42, y: 1}};
    打印(w.p.x);
}
