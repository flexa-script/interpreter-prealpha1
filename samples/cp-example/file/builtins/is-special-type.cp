println("\nnone:");
var b: int = 10;
println("is_any: ", is_any(b));
println("is_array: ", is_array(b));
println("is_struct: ", is_struct(b));

println("any:");
var a = 10;
println("is_any: ", is_any(a));
println("is_array: ", is_array(a));
println("is_struct: ", is_struct(a));

println("\nany array:");
var f[3] = {10,9,8};
println("is_any: ", is_any(f));
println("is_array: ", is_array(f));
println("is_struct: ", is_struct(f));

println("\narray:");
var c[3]: int = {10,9,8};
println("is_any: ", is_any(c));
println("is_array: ", is_array(c));
println("is_struct: ", is_struct(c));

println("\nmulti-dim array:");
var d[3]: int = {{10,9,8},{10,9,8},{10,9,8}};
println("is_any: ", is_any(d));
println("is_array: ", is_array(d));
println("is_struct: ", is_struct(d));

struct Foo {
  var bar: int;
};

println("\nstruct:");
var e: Foo = Foo{bar=10};
println("is_any: ", is_any(e));
println("is_array: ", is_array(e));
println("is_struct: ", is_struct(e));

println("\nany struct:");
var i = Foo{bar=10};
println("is_any: ", is_any(i));
println("is_array: ", is_array(i));
println("is_struct: ", is_struct(i));

println("\nstruct array:");
var g[3]: Foo = {e,e,e};
println("is_any: ", is_any(g));
println("is_array: ", is_array(g));
println("is_struct: ", is_struct(g));

println("\nany array of struct:");
var h[3] = {e,e,e};
println("is_any: ", is_any(h));
println("is_array: ", is_array(h));
println("is_struct: ", is_struct(h));
