// Test English keyword aliases
function add(a, b) {
    return a + b;
}

var x = 10;
if (x > 5) {
    println("x is greater than 5");
} else {
    println("x is not greater than 5");
}

var i = 0;
while (i < 3) {
    println("while: " + i);
    i = i + 1;
}

for (var j = 0; j < 3; j = j + 1) {
    println("for: " + j);
}

// Mixed Chinese + English
var 结果 = add(10, 20);
如果 结果 等于 30 则 {
    println("English keyword + Chinese operator OK");
}

println("ALL ENGLISH KEYWORD TESTS PASSED");
