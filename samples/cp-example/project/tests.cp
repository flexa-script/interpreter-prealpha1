using core.person;
using list;
using util.println;


if (this == "main") {
    				var persons: List = List{};

    list_add(persons, Person{name="John Doe", age=30});
    list_add(persons, Person{name="Joana Doe", age=35});
    list_add(persons, Person{name="Jack Smith", age=28});
    list_add(persons, Person{name="Mary Smith", age=25});
    
    list_print(persons);
    print("\n\n\n");
    
    print(string(list_at(persons, 0)) + "\n\n");
    print(string(list_at(persons, 1)) + "\n\n");
    print(string(list_at(persons, 2)) + "\n\n");
    print(string(list_at(persons, 3)) + "\n\n");
    print("\n\n");
    
    list_remove(persons, 1);
    list_print(persons);
    print("\n\n");
    list_remove(persons, 2);
    list_print(persons);
    print("\n\n");
    list_remove(persons, 0);
    list_print(persons);
    print("\n\n");
    list_remove(persons, 1);
    list_print(persons);
    print("\n\n");
}