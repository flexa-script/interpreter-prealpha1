using core.person;
using list;
using util.println;


if (this == "main") {
    				var numbers: List = List{};

    list_add(numbers, 10);
    list_add(numbers, 20);
    list_add(numbers, 40);
    list_add(numbers, 50);
    
    list_print(numbers);
    print("\n\n\n");
    
    print(string(list_at(numbers, 0)) + "\n\n");
    print(string(list_at(numbers, 1)) + "\n\n");
    print(string(list_at(numbers, 2)) + "\n\n");
    print(string(list_at(numbers, 3)) + "\n\n");
    print("\n\n");
    
    list_remove(numbers, 1);
    list_print(numbers);
    print("\n\n");
    list_remove(numbers, 2);
    list_print(numbers);
    print("\n\n");
    list_remove(numbers, 0);
    list_print(numbers);
    print("\n\n");
    list_remove(numbers, 1);
    list_print(numbers);
    print("\n\n");
}
