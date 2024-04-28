struct Node {
    var value : any;
    var next : Node;
};

struct List {
    var value : Node;
};

def add(list : List, value : any) : List {
    print("added " + string(value) + " to list\n");
    return list;
}
