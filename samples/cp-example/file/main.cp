var test = 10;

fun f(x) {
    var fv = "var fun";
    println("print: ", x);
}

while (true) {
    var scoped = 'x';
    var v2 = 99;
    f(v2);
}
