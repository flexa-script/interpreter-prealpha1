// using cp.std.random;

const RAND_MAX = 4294967295;

var _next: int = 1;

fun _rand(): int {
    _next = (_next * 1103515245 + 12345) % RAND_MAX;
		return _next;
}

fun randf_range(from: float, to: float): float {
	return from + _rand() * (to - from) / RAND_MAX;
}

fun randi_range(from: int, to: int): int {
	return int(randf_range(float(from), float(to)));
}

for(var i = 0; i < 100; i++){
  // println(cp::randi_range(0, 100));
  println(randi_range(0, 100));
}
