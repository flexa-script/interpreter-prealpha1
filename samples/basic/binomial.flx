var pi : float = 3.1415926535897932;

// Positive integer power
def pow(x : float, n : int) : float {
	var y : float = 1;
	while (n > 0) {
		y = y * x;
		n = n - 1;
	}
	return y;
}

// Factorial
def fac(n : int) : int {
	if (n == 0) {
		return 1;
	} else {
		return n * fac(n - 1);
	}
}

// Cosine function
def cos(x : float) : float {
	var k : int = 0;
	var cos_x : float = 0;
	while (k < 8) {
		cos_x = cos_x + pow(-1., k) * pow(x, 2 * k) / fac(2 * k);
		k = k + 1;
	}
	return cos_x;
}

print(cos(pi));
