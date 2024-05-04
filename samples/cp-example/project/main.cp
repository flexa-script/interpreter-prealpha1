using core.person;
using list;
using util.println;


if (this == "main") {
    var numbers: List; // DEVERIA GERAR ERRO DE ACESSO
    list_init(numbers); // DEVERIA PREENCHER O VALOR
    // var numbers: List = list_create(); // A LISTA NÃO É PREENCHIDA
    // var numbers: List = List{};
    
    print("numbers: " + string(numbers) + "\n");

    // print("List init: size: " + string(numbers.size) + "\n");

    list_add(numbers, 10);
    // print("add 10: size: " + string(numbers.size) + "\n");
    list_add(numbers, 20);
    // print("add 20: size: " + string(numbers.size) + "\n");
    list_add(numbers, 40);
    list_add(numbers, 50);
    list_add(numbers, 60);
    // print("antes print: size: " + string(numbers.size) + "\n");
    
    list_print(numbers);
    print("\n\n\n");

    // print("at: size: " + string(numbers.size) + "\n");
    
    print(string(list_get(numbers, 0)) + "\n\n");
    print(string(list_get(numbers, 1)) + "\n\n");
    print(string(list_get(numbers, 2)) + "\n\n");
    print(string(list_get(numbers, 3)) + "\n\n");
    print(string(list_get(numbers, 4)) + "\n\n");
    print("\n");
    
    // print("remove: size: " + string(numbers.size) + "\n");
    list_remove(numbers, 0);
    list_print(numbers);
    print("\n\n");
    list_remove(numbers, 1);
    list_print(numbers);
    print("\n\n");
    list_remove(numbers, 2);
    list_print(numbers);
    print("\n\n");
    list_remove(numbers, 1); // deveria gerar erro
    list_print(numbers);
    print("\n\n");
    // print("fim: size: " + string(numbers.size) + "\n");
    list_remove(numbers, 0);
    list_print(numbers);
    print("\n\n");

    print(list_iterator(numbers));
}
