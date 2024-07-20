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
    // print("\nto_array:\n");
    var arr[list.size]: any = {null};
    // print(arr);
    var curr_node = list.first;
    for (var i = 0; i < list.size; i++) {
        // print(typeof(List) + "\n");
        // print(typeof(curr_node.value) + "\n");
        if (typeof(curr_node.value) == typeof(List)) {
            arr[i] = to_array(curr_node.value);
        } else {
            arr[i] = curr_node.value;
        }
        curr_node = curr_node.next;
    }
    println("array: ", arr);
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
