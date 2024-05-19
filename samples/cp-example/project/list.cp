/**
 * Lib: list.cp
 * Author: Carlos Machado
 * Date: 29/04/2024
 */


struct Node {
    var value: any;
    var next: Node;
};

struct List {
    var first: Node;
    var size: int;
};


def list_create(): List {
    return List{first=null, size=0};
}

def list_init(list: List) {
    list = list_create();
}

def list_add(list: List, value: any) {
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

def list_remove(list: List, index: int): bool {
    // print("curr list: "+string(list)+"\n");
    // print("list.first="+string(list.first)+"\n");
    // print("list.first.next="+string(list.first.next)+"\n");
    // print("list.first.next.next="+string(list.first.next.next)+"\n");
    // print("list.size: " + string(list.size) + "\nindex: "+string(index)+"\n");
    // print("(index >= list.size)="+string(index >= list.size)+"\n");
    if (index >= list.size) {
        // print("entrou (index >= list.size)\n");
        return false;
    }

    // print("(index == 0)="+string(index == 0)+"\n");
    if (index == 0) {
        // print("list.first="+string(list.first)+"\n");
        // print("list.first.next="+string(list.first.next)+"\n");
        list.first = list.first.next;
        // print("list.first="+string(list.first)+"\n");
        // print("list.first.next="+string(list.first.next)+"\n");
    } else {
        var prev_node;
        var curr_node = list.first;

        for (var i = 0; i < index; i++) {
            prev_node = curr_node;
            // print("curr_node="+string(curr_node)+"\n");
            // print("curr_node.next="+string(curr_node.next)+"\n");
            curr_node = curr_node.next;
            // print("curr_node="+string(curr_node)+"\n");
            // print("curr_node.next="+string(curr_node.next)+"\n");
        }

        prev_node.next = curr_node.next;
    }
    list.size--;

    return true;
}

def list_get(list: List, index: int): any {
    if (index >= list.size) {
        return null;
    }

    var node = list.first;

    for (var i = 0; i < index; i++) {
        node = node.next;
    }

    return node.value;
}

def list_is_empty(list: List): bool {
    return list.size == 0;
}

def list_iterator(list: List): any[] {
    var arr[]: any = {null};
    // arr[0] = list.first.value;
    return arr;
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
