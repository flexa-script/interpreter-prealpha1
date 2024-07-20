// struct Node {
//     var value: any;
//     var next: Node;
// };

// struct List {
//     var first: Node;
//     var size: int;
// };

// var list: List = List{};

// list.first = Node{value=10, next=null};
// list.size = 0;
// list.size++;

// println(list);

// var arr[list.size]: any = {null};
// arr[0] = list.first;

// println(arr);

struct Person {
    var name: string;
    var age: int;
};

// var p = Person{name="asda", age=10};

// println(p);

fun new_person(name, age): Person {
    return Person{name=name, age=age};
}
