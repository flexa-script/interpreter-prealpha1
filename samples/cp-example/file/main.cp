struct a{
var b:int;
};
def test(aaa: a){
	aaa = null;
}
var asd: a = a{};
print(asd, "\n");
asd = null;
print(asd, "\n");
asd = a{};
print(asd, "\n");
test(unref asd);
print(asd, "\n");
test(asd);
print(asd, "\n");
asd = a{};
test(ref asd);
print(asd, "\n");

println('\n');
def test2(b) {
	b = 5;
}
var a = 10;
println(a);
test2(a);
println(a, '\n');
test2(ref a);
println(a, '\n');
