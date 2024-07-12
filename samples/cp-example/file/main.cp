var a = 10;

println("is_any: ", is_any(a));
println("is_array: ", is_array(a));
println("is_struct: ", is_struct(a));

var b: int = 10;

println("is_any: ", is_any(b));
println("is_array: ", is_array(b));
println("is_struct: ", is_struct(b));

var c[3]: int = {10,9,8};

println("is_any: ", is_any(c));
println("is_array: ", is_array(c));
println("is_struct: ", is_struct(c));
