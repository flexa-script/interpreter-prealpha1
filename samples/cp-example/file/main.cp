struct Node {
    var value: any;
    var next: Node;
};

struct List {
    var first: Node;
    var size: int;
};

var list: List = List{};

list.first = Node{value=10, next=null};
list.size = 1;

println(list);
