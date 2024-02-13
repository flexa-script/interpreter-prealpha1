struct Element {
  var has_value : bool;
  var value : any;
  var next : Element;
};

struct List{
  var root : Element;
};

var list : List;

list.root = Element {
  has_value = true,
  value = 10
};

print(list.root.value);
