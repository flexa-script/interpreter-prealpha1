struct Node {
    var value: any;
    var next: Node;
};

struct List {
    var first: Node;
    var size: int;
};


fun create(): List {
    return List{first=null, size=0};
}

fun init(list: List) {
    list = create();
}

fun add(list: List, value: any) {
    if (list.first == null) {
        list.first = Node{value=value, next=null};
        list.size = 1;
        // print("added to 1st list pos: "+string(list)+"\n");
    } else {
        var prev_node: Node = Node{value=null, next=null};
        var curr_node = list.first;

        while (curr_node.next != null) {
            prev_node = curr_node;
            curr_node = curr_node.next;
        }

        curr_node.next = Node{value=value, next=null};
        list.size++;

        // print("added to nth list pos: "+string(list)+"\n");
    }
}