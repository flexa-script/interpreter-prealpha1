using cp.core.exception;
// using cp.core.pair;

try {
	var a = 10/0;
} catch (var ex: cp::Exception) {
	print("generated error: " + ex.error);
}

try {
	var a = 10/0;
} catch (var [type, error]) {
	print("generated error: " + error);
}

try {
	var a = 10/0;
} catch (...) {
	print("generated error: ");
}

// struct Foo {
// 	var name: string;
// 	var age: int;
// };

// var f = Foo{name="Carlos",age=29};

// println("foreach struct as cp::Pair");
// foreach(var i: cp::Pair in f){
//   println("", i.key, ":", i.value);
// }

// println("foreach struct as unpack declaration");
// foreach(var [key, value] in f){
//   println("", key, ":", value);
// }
