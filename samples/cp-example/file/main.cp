
struct Node {
    var value: any;
    var next: Node;
};

struct List {
    var first: Node;
};

var list: List = List{first=Node{value=1, next=Node{value=2, next=null}}};

var node = list.first;
while (node != null) {
    println("3 list.first=",list.first);
    node = node.next;
    println("4 list.first=",list.first);
}

println(list);
