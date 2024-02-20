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
- [X] Add 'this' token to get current function name
- [X] Add const support
- [X] Add float type
- [X] Add char type
- [X] Add any type
- [X] Add array type
  - [X] Make array accessor accepts expression
- [X] Add struct support
    - [X] Add struct array-like constructor
    - [X] Add Java class behavior
- [X] Add null support
- [X] Add type convert support
- [X] Remove mandatory variable type definition
- [X] Fix cp dependencies/libs loader
- [X] Add string array-like accessor
- [ ] Improve error system
- [ ] Add statements:
    - [ ] for
    - [ ] else if
    - [ ] switch
    - [ ] try catch
    - [ ] enum
    - [ ] immediate if?
- [ ] Add built in funcs:
    - [X] len: returns size of array/string
    - [X] round: round a float number
    - [X] type: returns type of object
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
    - [ ] Move some verifications to semantic analisys:
        - [ ] Array size check from interpreter
- [ ] Add CP library:
    - [ ] String
    - [ ] Math
    - [ ] Collections
        - [ ] List
        - [ ] Queue
        - [ ] Stack
    - [ ] Util

## Rules

### Arrays
- If an array don't have your size defined, it can be assigned and reassigned of all sizes. Altought must respect the dimension
- If an array has your size defined indeclaration, it can just assigned and reassigned with a same size and dimension array expression
