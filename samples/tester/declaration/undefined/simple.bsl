using bsl.std.testing;

println("declaring 'us' as undeclared any");
var us;
cp::assert_equals("'us' any", typeof(any), typeof(us));

println("declaring 'usa' as any");
var usa: any;
assert_equals("'usa' any", typeof(any), typeof(usa));

println("declaring 'usb' as bool");
var usb: bool;
assert_equals("'usb' bool", typeof(bool), typeof(usb));

println("declaring 'usi' as int");
var usi: int;
assert_equals("'usi' int", typeof(int), typeof(usi));

println("declaring 'usf' as float");
var usf: float;
assert_equals("'usf' float", typeof(float), typeof(usf));

println("declaring 'usc' as char");
var usc: char;
assert_equals("'usc' char", typeof(char), typeof(usc));

println("declaring 'uss' as string");
var uss: string;
assert_equals("'uss' string", typeof(string), typeof(uss));

