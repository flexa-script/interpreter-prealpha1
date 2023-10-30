//using problems;


def main() {
    // problem1();
    // problem2();

    // print(10 % 3);
    // print("String true: " + string(true) + "\n");
    // print("String false: " + string(false) + "\n");
    // print("String int: " + string(10) + "\n");
    // print("String float: " + string(5.) + "\n");

    // print("Float int: ");
    // print(float(10));
    // print("\n");
    // print("Float string: ");
    // print(float("10.555"));
    // print("\n");

    // print("Int float: ");
    // print(int(10.5));
    // print("\n");
    // print("Int string: ");
    // print(int("10"));
    // print("\n");

    if (6 % 3 == 0 or 10 % 5 == 0) {
        print("enter\n");
    }

    if (true and false or false and true) {
        print("ops\n");
    }

    if (true or true and false) {
        print("ops\n");
    }

    // && has higher precedence than ||
    var a: bool = true or false and false;
    var b: bool = false and false or true;

    if (a == b) { // different precedence
        if (a == true) {
            print("&& has higher precedence than || \n");
        } else { // a == false
            print("|| has higher precedence than && \n");
        }
    } else { // a != b, same precedence
        if (a == true) { // and b == false
            print("&& and || have equal precedence, and are executed right to left. \n");
        } else { // a == false, b == true
            print("&& and || have equal precedence, and are executed left to right. \n");
        }
    }
}

main();
