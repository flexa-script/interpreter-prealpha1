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

def list_add(list: List, value: any) {
    if (not list.first) { 
        list.first = Node{value=value, next=null};
        list.size = 0;
    } else {
        var prev_node: Node = Node{};
        var curr_node = list.first;

        while (curr_node) {
            prev_node = curr_node;
            curr_node = curr_node.next;
        }

        curr_node.next = Node{value=value};
        list.size++;
    }
}

def list_remove(list: List, index: int): bool {
    if (index >= list.size) {
        return false;
    }
    
    if (index == 0) { 
        list.first = list.first.next;
    } else {
        var prev_node;
        var curr_node = list.first;

        for (var i = 0; i < index; i++) {
            prev_node = curr_node;
            curr_node = curr_node.next;
        }

        prev_node.next = curr_node.next;
    }
    list.size--;
    
    return true;
}

def list_at(list: List, index: int): any {
    if (index >= list.size) {
        return null;
    }
    
    var node = list.first;
    
    for (var i = 0; i < index; i++) {
        node = node.next;
    }
    
    return node.value;
}

def list_print(list : List) {
    if (not list.first) {
        print("[]");
        
    } else {
        var node = list.first;
        print("[");
        while (node) {
            print(string(node.value));
            if (node.next) {
                print(",");
            }
            node = node.next;
        }
        print("]");
    }
}
