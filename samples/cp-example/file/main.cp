//print("Hello World!\n");

// TODO: fix function return has value in semantic analysis
// TODO: implements function array return
// TODO: fix array and struct in foreach interpreter

// switchcase


if (1 == 2) {
  print(1);
} else if (1 == 1) {
  print(2);
} else if (6 == 1) {
  print(4);
} else if (1 == 1) {
  print(5);
} else {
  print(3);
}


def foo() : int {
    if (1==10){
        //return 10;
    }else if(1==1){
        // return 5;
    }
    return 0;
}

print(foo());
