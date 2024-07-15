// using cp.core.exception;
using cp.core.pair;

// try {
// 	var a = 10/0;
// } catch (var ex: cp::Exception) {
// 	print("generated error: " + ex.error);
// }

// // try {
// // 	var a = 10/0;
// // } catch (var [error]) {
// // 	print("generated error: " + error);
// // }

// try {
// 	var a = 10/0;
// } catch (...) {
// 	print("generated error: ");
// }

struct Foo {
	var name: string;
	var age: int;
};

var f = Foo{name="Carlos",age=29};

foreach(var i: cp::Pair in f){
  println("", i.key, ":", i.value);
}

// foreach(var [key, value] in f){
//   println("", key, ":", value);
// }
