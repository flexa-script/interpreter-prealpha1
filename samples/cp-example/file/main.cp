// def foo(msg, ...args) {
// 	print(msg, ": ");
// 	foreach (var a in args) {
// 		print(a, ' ');
// 	}
// }

// foo("this is a normal parameters and then", 10, 'a', 99, "asdasd");

const NEW_LINE: char = '\n';

def println(value: any) {
  print(string(value) + NEW_LINE);
}

print("hello world");
