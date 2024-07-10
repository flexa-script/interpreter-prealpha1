//not working
struct Foo {
	var bar: int;
};
fun foo() {
	return Foo{bar=10};
}
print(foo().bar);

// not working
fun arr() {
	return {10,11,21};
}
print(arr()[1]);

// fun string_val(){
// 	return "test";
// }
// print(string_val());
// print(string_val()[1]); // not working

// not generating correct error
// fun test();
// test();
