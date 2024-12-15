// types-access.cp

var str : string = "foo";
var strAny = "bar";
var strArray[] : string = {"is", "a", "test"};
var boolValue : bool = false;
var intValue : int = 10;
var floatValue : float = 9.9;
var charValue : char = 'A';
var arrValue[] : int = {1,2,3};

print("str\n");
print(str);
print("\n");
print(str[0]);
print("\n");
print(str[0]);
print(str[1]);
print(str[2]);
print("\n\n");

print("strAny\n");
print(strAny);
print("\n");
print(strAny[2]);
print("\n");
print(strAny[0]);
print(strAny[1]);
print(strAny[2]);
print("\n\n");

print("strArray\n");
print(strArray[0]);
print(' ');
print(strArray[1]);
print(' ');
print(strArray[2]);
print("\n");
print(strArray[2][1]);
print("\n\n");

print("arrValue\n");
print(arrValue);
print("\n");
print(arrValue[0]);
print(arrValue[1]);
print(arrValue[2]);
print("\n\n");

// teste: erro, posições inválidas
//print(str[3]);
//print(strAny[4]);
//print(strArray[3]);
//print(strArray[3][5]);
//print(strArray[3][0]);
//print(strArray[2][4]);

// teste: erro pois não é array
//print(boolValue[0]);
//print(intValue[0]);
//print(floatValue[0]);
//print(charValue[0]);
