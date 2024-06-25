using cp.core.pair;

struct Bar {
	var x: int;
	var j: bool;
};

struct Foo {
	var name: string;
	var age: int;
	// var bar: bool;
	var bar: Bar;
};

var f = Foo{name="Carlos",age=29,bar=Bar{x=10,j=false}};

def print_str(f){
	foreach (var it in f) {
		if(typeof(it.value)=="Bar"){
			print_str(it.value);
		}
		else{
			print("key: ", it.key, "\nvalue: ", it.value, "\ntype: ", typeof(it), '\n');
		}
	}
}

print_str(f);
