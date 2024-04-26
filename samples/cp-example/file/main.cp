//print("Hello World!\n");

// TODO: fix function return has value in semantic analysis
// TODO: implements function array return
// TODO: fix array and struct in foreach interpreter

// switchcase

def foo() : int {
    if (1==10){
        return 10;
    }else if(1==1){
        return 5;
    }
    return 0;
}

print(foo());
