//not working
struct Foo {
	var str: string;
	var bar: int;
};
fun foo() {
	return Foo{str="asda", bar=10};
}
println(foo().bar);
println(foo().str);
println(foo().str[0]);

fun arr() {
	return {10,11,21};
}
println(arr()[1]);

fun string_val(){
	return "test";
}
println(string_val());
println(string_val()[1]); // not working

// // not generating correct error
// fun test();
// test();

// var a;
// a = 10;
// println(a);

C:\Users\Carlos\repos\cp-lang\interpreter\samples\cp-example\file\2048.cp
