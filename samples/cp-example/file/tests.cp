
// 2024-08-09 --------------------------------------------------------------------------
// var v[]: string;
// // v = {"12", 34, '5', 6789f};
// v = {"12", "34", "5", "6789f"};

// var v2[]: string = {"12", "34", "5", "6789f"};

// try {
// var v[]: string = {"12", 34, '5', 6789f};
// }
// catch (var [error]) {
// println(error);
// }

// try {
//     var v[]: string;
//     v = {"12", 34, '5', 6789f};
// }
// catch (var [error]) {
// println(error);
// }

// var v[]: any = {"12", "34", "5", "6789"};
// v += {"12", 34, '5', 6789f};
// v += {"a", "b", "'c'", "d"};
// println(v);


// fun join(...args: any): string {
//   var ss: string = "";
//   foreach (var a in args) {
//     ss += string(a);
//   }
//   return ss;
// }

// var res: string = join("12", 34, '5', 6789f);


// 2024-08-04 --------------------------------------------------------------------------
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
