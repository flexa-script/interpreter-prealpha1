using lib1;
using folder.lib2 as list;

def foo() : string {
    return "main foo";
}

if (this == "main") {
  print(string(lib1.TEST) + '\n');
  print(lib1.foo() + '\n');
  print(foo() + '\n');

  var persons : list.List = list.List{ };
  list.add(persons, lib1.Person { name = "Carlos", age = 29});
}

// print(foo() + '\n');
