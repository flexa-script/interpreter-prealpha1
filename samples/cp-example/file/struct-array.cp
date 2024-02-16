struct Element {
  var arr[3] : int;
  var value : any;
};

var element : Element = Element {
  has_value = { 1, 2, 3 },
  value = 10
};

//element.has_value = true;
//element.value = 10;

print(element.arr[0]);
print("\n");
print(element.arr[1]);
print("\n");
print(element.arr[2]);
print("\n");
print(element.value);
print("\n");
print("\n");

element.arr[0] = 10;
element.arr[1] = 12;
element.arr[2] = element.arr[0] + element.arr[1];

print(element.arr[0]);
print("\n");
print(element.arr[1]);
print("\n");
print(element.arr[2]);
print("\n");
print(element.value);
print("\n");
print("\n");
