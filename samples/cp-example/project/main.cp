using core.person;
using list;
using util.println;

// as namespace io;

def my_list(): list::List {
    return list::List{first=null, size=0};
}

if (this == "main") {
    // var numbers: List;
    // list::init(numbers); // DEVERIA PREENCHER O VALOR
    var numbers: list::List = list::create(); // A LISTA NÃO É PREENCHIDA
    // var numbers: List = List{first=null, size=0};
    
    print(string(my_list())+"\n");

    //print("numbers: " + string(numbers) + "\n");
    io::println("numbers: " + string(numbers));
    print("List init: size: " + string(numbers.size) + io::NEW_LINE);

    list::add(numbers, 10);
    // list::print(numbers);
    // print("\n");
    // print("add 10: size: " + string(numbers.size) + "\n");
    list::add(numbers, 20);
    // list::print(numbers);
    // print("\n");
    // print("add 20: size: " + string(numbers.size) + "\n");
    list::add(numbers, 30);
    // list::print(numbers);
    // print("\n");
    list::add(numbers, 40);
    // list::print(numbers);
    // print("\n");
    list::add(numbers, 50);
    // list::print(numbers);
    // print("\n");
    list::add(numbers, 60);
    // list::print(numbers);
    // print("\n");
    // print("antes print: size: " + string(numbers.size) + "\n");
    
    // print(numbers);
    // print("\n");
    // print(numbers.first);
    // print("\n");
    // print(numbers.first.next);
    // print("\n");
    // print(numbers.first.next.next);
    print(string(numbers.size) + "\n");
    print(list::to_string(numbers));
    print("\n\n\n");

    print("size: " + string(numbers.size) + "\n");
    
    print(string(list::get(numbers, 0)) + "\n");
    print(string(list::get(numbers, 1)) + "\n");
    print(string(list::get(numbers, 2)) + "\n");
    print(string(list::get(numbers, 3)) + "\n");
    print(string(list::get(numbers, 4)) + "\n");
    print(string(list::get(numbers, 5)) + "\n");
    print("\n");

    print("\n\nto_array return: \n" + string(list::to_array(numbers)));
    print("\n\n");
    
    // print("remove: size: " + string(numbers.size) + "\n");
    // print(string(numbers));
    // print("\n");
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
    // print("fim: size: " + string(numbers.size) + "\n");
    list::remove(numbers, 1);
    print(list::to_string(numbers));
    print("\n");
    list::remove(numbers, 0);
    print(list::to_string(numbers));
    print("\n");

    print("\n\nto_array return: \n" + string(list::to_array(numbers)));
}
