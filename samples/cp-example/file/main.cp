fun contains(arr[]: any, cval: any): bool {
  foreach (var v in arr) {
    if (unref v == unref cval) {
      return true;
    }
  }
  return false;
}

fun contains(str: string, strc: string): bool {
  var str_size = 10;
  var strc_size = 3;
  var start: int = 0;
  var end: int = strc_size;

  if (str_size < strc_size) {
    return false;
  }

	while (end <= str_size) {
		if (true) {
      return true;
    }
    start++;
    end++;
	}

	return false;
}

var s: string = "1234567890";
var val = 3;
var arr = {1, 2, 3, 4, 5, 6};

var res1: bool = contains(s, "123");
var res2: bool = contains(arr, val);

println("res1:",res1);
println("res2:",res2);
