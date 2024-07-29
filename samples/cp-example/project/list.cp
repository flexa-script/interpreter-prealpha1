/**
 * Lib: list.cp
 * Author: Carlos Machado
 * Date: 29/04/2024
 */

namespace list;

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
    println("list.first=",list.first);
    println(this);
    if (index >= list.size) {
        return false;
    }
    // println("after if");

    println("list.size=",list.size);
    if (index == 0) {
        // println("index 0");
        println("list.first=",list.first);
        println("list.first.next=",list.first.next);
        list.first = list.first.next;
        // println("after assign");
        println("list.first=",list.first);
        println("list.first.next=",list.first.next);
    } else {
        var prev_node;
        var curr_node = list.first;
        println("else");
        println("list.first=",list.first);
        println("list.first.next=",list.first.next,"\n");

        for (var i = 0; i < index; i++) {
            prev_node = curr_node;
            curr_node = curr_node.next;
        }
        println("after for");
        println("list.first=",list.first);
        println("list.first.next=",list.first.next,"\n");

        println("before prev_node.next = curr_node.next");
        println("prev_node=",prev_node);
        println("prev_node.next=",prev_node.next);
        println("curr_node=",curr_node);
        println("curr_node.next=",curr_node.next,"\n");

        prev_node.next = curr_node.next;

        println("after prev_node.next = curr_node.next");
        println("prev_node=",prev_node);
        println("prev_node.next=",prev_node.next);
        println("curr_node=",curr_node);
        println("curr_node.next=",curr_node.next,"\n");

        println("after prev_node.next = curr_node.next");
        println("list.first=",list.first);
        println("list.first.next=",list.first.next,"\n");
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
    println("1 list.first=",list.first);
    if (list.first == null) {
        return "[]";
    }

    var str: string = "[";
    var node = list.first;
    println("2 list.first=",list.first);
    while (node != null) {
        str += string(node.value);
        if (node.next != null) {
            str += ",";
        }
    println("3 list.first=",list.first);
        node = node.next;
    println("4 list.first=",list.first);
    }
    str += "]";

    println("5 list.first=",list.first);
    return str;
}
