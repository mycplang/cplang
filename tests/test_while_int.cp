println("TEST START");

变量 活 = 1;
println("loop check: 活 = " + 活);

当 (活 == 1) {
    println("IN LOOP");
    活 = 0;
}

println("AFTER LOOP");
initWindow(100, 100, "T");
closeWindow();
println("DONE");
