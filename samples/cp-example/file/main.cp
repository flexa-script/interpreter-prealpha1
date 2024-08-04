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
//     println("3 list.first=",list.first);
//     node = node.next;
//     println("4 list.first=",list.first);
// }

// println(list);

// var arr = {{0,1,2},{0,1,2},{0,1,2}};

var arr = {{{0,1},{0,1},{0,1}},{{0,1},{0,1},{0,1}},{{0,1},{0,1},{0,1}}};

// println(arr);
// println(len(arr));
println(len(arr));
println(len(arr[0]));
println(len(arr[1]));
println(len(arr[2]));
// println(arr[0]);
// println(arr[0][0]);

/*

[1
  [2
    [3
      1,2,3 0
    ]2
  ],
  [
    [
      1,2,3
    ]
  ],
  [
    [
      1,2,3
    ]
  ]
]

*/
