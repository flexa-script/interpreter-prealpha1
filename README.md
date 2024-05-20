# CP Interpreter

## TODO
- [X] Add load file from param
- [X] Rewrite the lexer
- [X] Rewrite EBNF
- [X] Implements new EBNF commands
- [X] Change expression evaluation to left to right
- [X] Add and precedence
- [X] Add void support to function
- [X] Add void return
- [X] Add mod (%) operation support
- [X] Add library support
- [X] Add collum on errors
- [X] Add 'this' token to get current function name
- [X] Add const support
- [X] Add char type
- [X] Add any type
- [X] Add array type
  - [X] Make array accessor accepts expression
- [X] Add struct support
    - [X] Add struct array-like constructor
    - [X] Add Java class pointer-like behavior
- [X] Add null support
- [X] Add type convert support
    - [X] Add array and struct support (based on print)
- [X] Remove mandatory variable type definition
- [X] Fix cp dependencies/libs loader
- [X] Add string array-like accessor
- [ ] Add statements:
    - [X] for
    - [X] foreach
    - [X] else if
    - [X] switch
    - [X] break
    - [X] continue
    - [X] +=,-=,*=,/=,%= assignment
    - [X] namespace
    <!-- - [0] instaceof (typeof was adapted to accept types) -->
    - [ ] as namespace
    - [ ] exit
    - [ ] enum
    - [ ] try catch
- [ ] Add expression operators:
    - [X] ++,--
    <!-- - [ ] Add array(size) initializer to build custom sized arrays -->
    <!-- - [ ] Add { init_value } to initilize all positions with same value -->
    - [ ] Add 'in' operator to check if anything is in array/string
    - [ ] immediate if
- [X] Add built in funcs:
    - [X] len: returns size of array/string
    - [X] round: round a float number
    - [X] typeof: returns type of object
- [X] Let functions return array
- [X] Add lib scope lib.(method|properties|struct)
- [X] Guanrantee that a lib is loaded once
- [X] Flex function return (currently it's mandatory specificate the return type)
- [X] Remove struct as bool in expression and change this type of verification to void value comparation (eg: var == null)
- [X] Move function body parser to caller node (as interpreter), to check parameters of each caller
- [ ] Improve error system (based on C)
- [ ] Improve REPL:
    - [ ] Now expressions can be executed in statements, dont need more to execute expression parser directly
- [ ] Add a default interface to implements core libs
- [ ] Add core library:
    - [ ] File I/O
    - [ ] Graphics
    - [ ] Mouse
    - [ ] Sound
    - [ ] Keyboard
    - [ ] Time
    - [ ] Web/API
    - [ ] Util
- [ ] General otimizations:
    - [ ] Remove duplicate codes
        - [ ] Type checks
    - [X] Move some verifications to semantic analisys:
        - [X] Array size check from interpreter
- [ ] Add CP library:
    - [ ] String
    - [ ] Math
    - [ ] Collections
        - [ ] List
        - [ ] Queue
        - [ ] Stack
    - [ ] Util

## Errors
- Errors has CP prefix, eg. CP9999
- Token errors init with 1
- Lexer errors init with 2
- Semantic scope errors init with 3
- Semantic analyser errors init with 4
- Interpreter scope errors init with 5
- Interpreter errors init with 6

## Rules

### Arrays
- If an array don't have your size defined, it can be assigned and reassigned of all sizes. Altought must respect the dimension.
- If an array has your size defined in declaration, it can just assigned and reassigned with a same size and dimension array expression.

### Types
- If an variable is not defined, it's undef, and it can be initialized with null or a value.
- Variable has a type, if it is defined, it is fixed, else it will be any and the type can change.
- If a variable is not an array, it can not be assined, even if it be any.