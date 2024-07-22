// struct Foo {
//   var a: int;
//   var b: int;
// };

// struct Bar {
//   var x: string;
//   var foo: Foo;
// };

// var bar: Bar = Bar{x="10", foo=Foo{a=10,b=50}};

// println(bar);

fun any_void_fun() {
  print("(void -> undefined)");
}

fun void_fun(): void {
  print("(void -> undefined)");
}

fun any_fun(s) {
  switch (s) {
  case 0:
    print("(any -> int)\t");
    return 0;
  case 1:
    print("(any -> string)\t");
    return "0000";
  case 2:
    print("(any -> arr)\t");
    return { 0,1,2};
  }
}

fun int_fun(): int {
  return 10;
}

// println(void_fun());
// println(any_void_fun());
println(any_fun(0));
println(any_fun(1));
println(any_fun(2));
println(int_fun());
