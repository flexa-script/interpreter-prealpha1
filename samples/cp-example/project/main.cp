using core.person;
using list;
using util.println;


if (this == "main") {
    // var numbers: List;
    // list_init(numbers); // DEVERIA PREENCHER O VALOR
    // var numbers: List = list_create(); // A LISTA NÃO É PREENCHIDA
    var numbers: List = List{first=null, size=0};
    
    //print("numbers: " + string(numbers) + "\n");
    io::println("numbers: " + string(numbers));
    print("List init: size: " + string(numbers.size) + io::NEW_LINE);

    list_add(numbers, 10);
    // list_print(numbers);
    // print("\n");
    // print("add 10: size: " + string(numbers.size) + "\n");
    list_add(numbers, 20);
    // list_print(numbers);
    // print("\n");
    // print("add 20: size: " + string(numbers.size) + "\n");
    list_add(numbers, 30);
    // list_print(numbers);
    // print("\n");
    list_add(numbers, 40);
    // list_print(numbers);
    // print("\n");
    list_add(numbers, 50);
    // list_print(numbers);
    // print("\n");
    list_add(numbers, 60);
    // list_print(numbers);
    // print("\n");
    // print("antes print: size: " + string(numbers.size) + "\n");
    
    // print(numbers);
    // print("\n");
    // print(numbers.first);
    // print("\n");
    // print(numbers.first.next);
    // print("\n");
    // print(numbers.first.next.next);
    list_print(numbers);
    print("\n\n\n");

    print("size: " + string(numbers.size) + "\n");
    
    print(string(list_get(numbers, 0)) + "\n");
    print(string(list_get(numbers, 1)) + "\n");
    print(string(list_get(numbers, 2)) + "\n");
    print(string(list_get(numbers, 3)) + "\n");
    print(string(list_get(numbers, 4)) + "\n");
    print(string(list_get(numbers, 5)) + "\n");
    print("\n");
    
    // print("remove: size: " + string(numbers.size) + "\n");
    list_print(numbers);
    print("\n");
    list_remove(numbers, 0);
    list_print(numbers);
    print("\n");
    list_remove(numbers, 1);
    list_print(numbers);
    print("\n");
    list_remove(numbers, 3);
    list_print(numbers);
    print("\n");
    list_remove(numbers, 1);
    list_print(numbers);
    print("\n");
    // print("fim: size: " + string(numbers.size) + "\n");
    list_remove(numbers, 1);
    list_print(numbers);
    print("\n");
    list_remove(numbers, 0);
    list_print(numbers);
    print("\n");

    // print(list_iterator(numbers));
}
