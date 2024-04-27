//print("Hello World!\n");

// TODO: fix function return has value in semantic analysis
// TODO: fix function check array dimensions

def foo() : int[] {
    return { 1, 2, 3 };
}

def bar() : int[][] { // check dimensions
    return { 1, 2, 3 };
}

def doe() : int[][] { // check dimensions
    return { { 1, 2, 3 }, { 1, 2, 3 }, { 1, 2, 3 } };
}

struct AAA {
    var asd : int;
};

def jon() : AAA {
    return AAA {asd=5};
}


print(foo());
print(bar());
print(doe());
print(jon());
