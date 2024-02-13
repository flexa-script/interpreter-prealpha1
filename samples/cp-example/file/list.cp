// list.cp

struct Node {
  var has_value : bool;
  var value : any;
  var next : Node;
};

struct List {
  var first : Node;
};

def list_add(list : List, value : any) {
  if (not list.first.has_value) {
    var newNode : Node;
    newNode.has_value = true;
    newNode.value = value;
    list.first = newNode;
  } else {
    var prevNode : Node;
    var currNode = list.first;

    while (currNode.has_value) {
      prevNode = currNode;
      currNode = currNode.next;
    }

    var newNode : Node;
    newNode.has_value = true;
    newNode.value = value;

    currNode.next = newNode;
  }
}

def list_print(list : List) {
  if (not list.first.has_value) {
    print("[]");

  } else {
    var node = list.first;
    print("[");
    while (node.has_value) {
      print(string(node.value));
      node = node.next;
      if (node.next.has_value) {
        print(",");
      }
    }
    print("]");
  }
}

var list : List = List {
  first = Node {
    has_value = false,
    value = 0
  }
};

list_add(list, 10);
list_add(list, 9);
list_add(list, 8);
list_add(list, 7);

print(list);
print('\n');
list_print(list);
print('\n');
