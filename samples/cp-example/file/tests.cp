fun foo(){
  println("foo: ", this);
}

fun bar(){
  println("bar: ", this);
}

foo();
bar();

var qux = foo;

foo();
bar();
qux();

var bar = foo;

bar();

fun aaa(foo: function){
  foo();
}

aaa(fun () {
  println("aaa");}
);

// struct Node {
//     var value: any;
//     var next: Node;
// };

// struct List {
//     var first: Node;
// };

// var list: List = List{first=Node{value=1, next=Node{value=2, next=null}}};

// var node = list.first;
// while (node != null) {
//     node = node.next;
// }

// println(list);

// var arr = {{{0,1},{0,1},{0,1}},{{0,1},{0,1},{0,1}},{{0,1},{0,1},{0,1}}};
// var arr2[3][3][2]: int = {0};

// println(len(arr));
// println(len(arr[0]));
// println(len(arr[1]));
// println(len(arr[2]));

// println(typeof(arr));
// println(typeof(arr[0]));
// println(typeof(arr[0][0]));
// println(typeof(arr[0][0][0]));

// println();

// println(len(arr2));
// println(len(arr2[0]));
// println(len(arr2[1]));
// println(len(arr2[2]));

// println(typeof(arr2));
// println(typeof(arr2[0]));
// println(typeof(arr2[0][0]));
// println(typeof(arr2[0][0][0]));
