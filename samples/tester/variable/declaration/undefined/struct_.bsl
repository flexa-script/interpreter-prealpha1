start_test("DECLARATION / UNDEFINED / SIMPLE");

print("declaring 'ustr1' as Foo");
var ustr1: Foo;
println(" ... done");

print("declaring 'ustr2' as Bar");
var ustr2: Bar;
println(" ... done");

print("declaring 'ustra1x' as Foo");
var ustra11[3]: Foo;
var ustra12[3][3]: Foo;
var ustra13[3][3][3]: Foo;
println(" ... done");

print("declaring 'ustra2x' as Bar");
var ustra21[3]: Bar;
var ustra22[3][3]: Bar;
var ustra23[3][3][3]: Bar;
println(" ... done");

end_test();
