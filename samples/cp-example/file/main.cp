// var x[3]: int = {1,2,3};
// println(x);

// var t = 2;

// switch (t) {
// case 1:
// 	print(1);
// 	break;
// case 2:
// 	print(2);
// case 3:
// 	print(3);
// 	break;
// case 4:
// 	print(4);
// 	break;
// default:
// 	print("def");
// }

fun sort(arr[]: any, comparator: function = null): any[] {
	var arr_size = len(arr);
	for (var j = 0; j < arr_size; j++) {
		for (var i = 0; i < arr_size - 1; i++) {
			var res = false;

			if (comparator != null) {
				res = comparator(arr[i], arr[i + 1]);
			} else {
				res = arr[i] > arr[i + 1];
			}

			if (res) {
				var aux = arr[i + 1];
				arr[i + 1] = arr[i];
				arr[i] = aux;
			}
		}
	}
	return arr;
}

var arr = {3,1,5,0,2};

print(sort(arr));
