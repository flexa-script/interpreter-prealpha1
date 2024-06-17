// def foo(bar = 10){
//     print(bar);
// }
// foo();

def pow(x: float, n: float): float {
	//return exp(log(x) * n);

	if (int(n) == 0) {
		return 1.0;
	}

	if (int(n) % 2 == 0) {
		return pow(x, n / 2) * pow(x, n / 2);
	}
	
	return x * pow(x, n / 2) * pow(x, n / 2);
}

print(pow(2f, 3f));
