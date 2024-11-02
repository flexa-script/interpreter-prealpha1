// var x[3]: int = {1,2,3};
// println(x);

// var t = 2;

// switch (t) {
// case 1:
// 	print(1);
// 	break;
// case 2:
// 	print(2);
// case 3:
// 	print(3);
// 	break;
// case 4:
// 	print(4);
// 	break;
// default:
// 	print("def");
// }

fun testl(a, [bar, qux]) {
  println(a);
  println(bar);
  println(qux);
}

fun testr([bar, qux], b) {
  println(bar);
  println(qux);
  println(b);
}

fun testb(a, [bar, qux], b) {
  println(a);
  println(bar);
  println(qux);
  println(b);
}

fun test([bar, qux]) {
  println(bar);
  println(qux);
}

struct Foo {
	var bar: int;
	var qux: string;
};

var foo = Foo {
	bar=10,
	qux="word"
};

var [bar, qux] = foo;

println("declaration");
println(bar);
println(qux);

println("function");
testl(10, foo);
testr(foo, 20);
testb(10, foo, 20);
test(foo);
