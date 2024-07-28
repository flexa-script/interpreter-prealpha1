using core.person;
using list;
using util.println;

// as namespace io;

fun new_person(name, age): axe::Person {
    return axe::Person{name=name, age=age};
}

if (this == "main") {
    var numbers: list::List = list::create();
    
    // print(new_person("Carlos", 29), "\n");

    // io::println("numbers: " + string(numbers));
    print("list init: size: " + string(numbers.size) + io::NEW_LINE);

    list::add(numbers, 10);
    list::add(numbers, 20);
    list::add(numbers, 30);
    list::add(numbers, 40);
    list::add(numbers, 50);
    list::add(numbers, 60);

    print("size: " + string(numbers.size) + "\n");
    print(list::to_string(numbers));
    print("\n");
    
    print(string(list::get(numbers, 0)) + "\n");
    print(string(list::get(numbers, 1)) + "\n");
    print(string(list::get(numbers, 2)) + "\n");
    print(string(list::get(numbers, 3)) + "\n");
    print(string(list::get(numbers, 4)) + "\n");
    print(string(list::get(numbers, 5)) + "\n");
    print("\n");

    print("to_array return: \n" + string(list::to_array(numbers)));
    print("\n\n");

    print(list::to_string(numbers));
    print("\n");
    list::remove(numbers, 0);
    print(list::to_string(numbers));
    print("\n");
    list::remove(numbers, 1);
    print(list::to_string(numbers));
    print("\n");
    list::remove(numbers, 3);
    print(list::to_string(numbers));
    print("\n");
    list::remove(numbers, 1);
    print(list::to_string(numbers));
    print("\n");
    list::remove(numbers, 1);
    print(list::to_string(numbers));
    print("\n");
    println("0");
    list::remove(numbers, 0);
    println("1");
    print(list::to_string(numbers));
    println("2");
    print("\n");

    print("\nto_array return: \n" + string(list::to_array(numbers)));
}
