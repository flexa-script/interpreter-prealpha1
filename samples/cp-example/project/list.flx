/**
 * Lib: list.cp
 * Author: Carlos Machado
 * Date: 29/04/2024
 */

namespace list;

println(this);

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
    } else {
        var prev_node: Node = Node{value=null, next=null};
        var curr_node = list.first;

        while (curr_node.next != null) {
            prev_node = curr_node;
            curr_node = curr_node.next;
        }

        curr_node.next = Node{value=value, next=null};
        list.size++;
    }
}

// fun add(list: List, index: int, value: any): bool {
//     if (index >= list.size) {
//         return false;
//     }

//     if (index == 0) {
//         var new_node = Node{value=value, next=list.first};
//         list.first = new_node;
//     } else {
//         var prev_node;
//         var curr_node = list.first;

//         for (var i = 0; i < index; i++) {
//             prev_node = curr_node;
//             curr_node = curr_node.next;
//         }

//         var new_node = Node{value=value, next=curr_node};
//         prev_node.next = new_node;
//         list.size++;
//     }

//     return true;
// }

fun remove(list: List, index: int): bool {
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

fun get(list: List, index: int): any {
    if (index >= list.size) {
        return null;
    }

    var node = list.first;

    for (var i = 0; i < index; i++) {
        node = node.next;
    }

    return node.value;
}

fun clear(list: List) {
    list = create();
}

fun is_empty(list: List): bool {
    return list.size == 0;
}

fun to_array(list: List): any[] {
    var arr[list.size]: any = {null};
    var curr_node = list.first;
    for (var i = 0; i < list.size; i++) {
        if (typeof(curr_node.value) == typeof(List)) {
            arr[i] = to_array(curr_node.value);
        } else {
            arr[i] = curr_node.value;
        }
        curr_node = curr_node.next;
    }
    return arr;
}

fun to_string(list: List): string {
    if (list.first == null) {
        return "[]";
    }

    var str: string = "[";
    var node = list.first;
    while (node != null) {
        str += string(node.value);
        if (node.next != null) {
            str += ",";
        }
        node = node.next;
    }
    str += "]";

    return str;
}
