// typeof.cp

struct Foo {
  var bar : int;
};

var boolValue : bool;
var intValue : int;
var floatValue : float;
var charValue : char;
var stringValue : string;
var arrayValue[3] : int;
var structValue : Foo;

print(type(boolValue));
print("\n");
print(type(intValue));
print("\n");
print(type(floatValue));
print("\n");
print(type(charValue));
print("\n");
print(type(stringValue));
print("\n");
print(type(arrayValue));
print("\n");
print(type(structValue));
print("\n\n");
