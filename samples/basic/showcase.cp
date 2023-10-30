
def square(x : float) : float {
    return x * x;
}

/* Repeats string s, n times */
def repeat_string(s : string, n : int) : string {
    var s_rep : string;
    while(n > 0){
        s_rep = s_rep + s;
        n = n - 1;
    }
    return s_rep;
}

/* Recursive factorial */
def fac(n : int) : int {
    if (n == 0) {
       return 1;
    } else {
       return n * fac(n - 1);
    }
}

print(fac(5) - square(5.0)); // 95

print("\n");

print(repeat_string("Hello world!\n", 10));

print(square(fac(5) + 0.)); // 14400

print("\n");

def g(x : int) : int {
    // the x outside is shadowed by the argument x
    x = 2 * x;
    return x + 3;
}

def f(x : int) : int {
    return g(x) + 7;
}

print(f(5)); // 20
