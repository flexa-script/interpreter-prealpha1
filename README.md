# CP Interpreter

## TODO
- [X] Add load file from param
- [X] Rewrite the lexer
- [X] Rewrite EBNF
- [X] Implements new EBNF existent commands
- [X] Change expression evaluation to left to right
- [X] Add and precedence
- [X] Add void support to function
- [X] Add void return
- [X] Add mod (%) operation support
- [X] Add library support
- [X] Add collum on errors
- [X] Add 'this' token to get current function name (future an object?)
- [X] Add const support
- [X] Add float type
- [X] Add char type
- [X] Add any type
- [X] Add array type
- [X] Add struct support
    - [X] Add struct array-like constructor
    - [X] Add Java class behavior
- [X] Add null support?
- [X] Add type convert support
- [X] Remove mandatory variable type definition
- [X] Fix cp dependencies/libs loader
- [ ] Add string array-like accessor
- [ ] Add a default interface to implements core libs
- [ ] Add core library:
    - [ ] File I/O
    - [ ] Graphics
    - [ ] Mouse
    - [ ] Sound
    - [ ] Keyboard
    - [ ] Text
    - [ ] Time
    - [ ] Web/API
    - [ ] Util
- [ ] Add CP library:
    - [ ] Math
    - [ ] Util

## The CP Language

CP is an strongly-typed programming language. The language supports C-style comments, that is, `//...` for single-line comments and `/*...*/` for multiline comments. There are four datatypes in the language: `int` for integer values, `float` for floating point numbers, `bool` for boolean values and `string` for string literals. The language detects when integer values are assigned to real variables and performs an automatic typecast, except for the case of function parameters. For example, the function call `f(1)` would be invalid if the `f` expects a `float` argument, so we must write `f(1.0)` or `f(1.)`.  In turn, this strictness allows for multiple functions of the same name to be defined within the same scope, provided they have different signature. MiniLang supports variable shadowing.

The following code listing showcases some of MiniLang's syntax.

```
def square(x : float) : float {
    return x * x;
}

def repeat_string(s : string, n : int) : string {
    /* Repeats string s, n times */
    var s_rep : string;
    while(n > 0){
        s_rep = s_rep + s;
        n = n - 1;
    }
    return s_rep;
}

def fac(n : int) : int {
    /* Recursive factorial */
    if (n == 0) {
       return 1;
    } else {
       return n * fac(n - 1);
    }
}

print(fac(5) - square(5.0));               // 95
print(repeat_string("Hello", 10));
print(square(fac(5) + 0.));                // 14400

def g(x : int) : int {
    // the x outside is shadowed by the argument x
    x = 2 * x;
    return x + 3;
}

def f(x : int) : int {
    return g(x) + 7;
}

print(f(5));                              // 20
```
