struct Node {
    var value: any;
    var next: Node;
};

struct List {
    var first: Node;
    var size: int;
};

def list_add(list: List, value: any) {
    if (list.first == null) { 
        list.first = Node{value=value, next=null};
        list.size = 1;
    } else {
        var prev_node: Node = Node{};
        var curr_node = list.first;

        while (curr_node.next != null) {
            prev_node = curr_node;
            curr_node = curr_node.next;
        }

        curr_node.next = Node{value=value};
        list.size++;
    }
}

def list_print(list : List) {
    if (list.first == null) {
        print("[]");
        
    } else {
        var node = list.first;
        print("[");
        while (node != null) {
            print(string(node.value));
            if (node.next != null) {
                print(",");
            }
            node = node.next;
        }
        print("]");
    }
}

var numbers: List = List{first=null, size=0};

list_add(numbers, 10);
list_add(numbers, 9);
list_add(numbers, 8);
list_add(numbers, 7);

print(numbers);
list_print(numbers);
