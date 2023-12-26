struct Element {
  var has_value : bool;
  var value : any;
};

struct Node {
  var element : Element;
  var next : Element;
};

struct List {
  var first : Node;
};

def list_add(list : List, value : any) : List {
  if (not list.first.element.has_value) {
    var newElement : Element;
    newElement.has_value = true;
    newElement.value = value;
    list.first.element = newElement;

  } else {
    var prevNode : Node;
    var node = list.first;

    while (node.element.has_value) {
      prevNode = node;
      node = node.next;
    }

    var newElement : Element;
    newElement.has_value = true;
    newElement.value = value;

    node.element = newElement;
    prevNode.next = node;
  }

  return list;
}

def list_print(list : List) {
  if (not list.first.element.has_value) {
    print("[]");

  } else {
    var node = list.first;
    print("[");
    while (node.element.has_value) {
      print(string(node.element.value));
      node = node.next;
      if (node.next.has_value) {
        print(",");
      }
    }
    print("]");
  }
}


var list : List;

list = list_add(list, 10);
list = list_add(list, 9);
list = list_add(list, 8);
list = list_add(list, 7);

print(list);
print('\n');
list_print(list);
print('\n');
