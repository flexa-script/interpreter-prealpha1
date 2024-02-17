// string-known-errors.cp

var strArray[] : string = {"is", "a", "test"};

print(strArray[0]); // deveria printar "is" e não "i"
print("\n\n");

var strArray[] : string = {"is", "a", "test"};

print(strArray[0][1]); // deveria printar "is" e não "i"
print("\n\n");
