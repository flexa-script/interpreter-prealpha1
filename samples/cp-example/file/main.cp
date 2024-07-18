// var a = 1;

// // fun ai(x: float){
// // 	x+=x;
// // }

// fun ai(x: int){
// 	x+=x;
// }

// for (var i = 0; i < 50; i++){
// 	ai(ref a);
// 	println(a);
// }

const PRINT_TOP_LINE = "+---------------------------------------+";
const PRINT_BLK_LINE = "                                       ";

fun print_board(board[4][4]: int) {
    system("cls");
    print(PRINT_TOP_LINE);
    for (var i: int = 0; i < 4; i++) {
        print("\n|" + PRINT_BLK_LINE + "|\n|" + PRINT_BLK_LINE + "|\n|");
        for (var j: int = 0; j < 4; j++) {
            print("\t" + string(board[i][j]));
        }
        print("\t|\n|" + PRINT_BLK_LINE + "|");
    }
    print("\n|" + PRINT_BLK_LINE + "|\n");
    print(PRINT_TOP_LINE);
}

fun moves_up(board[4][4]: int) {
    for (var j: int = 0; j < 4; j++) {
        // moves everything close together
        for (var l: int = 0; l < 3; l++) {
            for (var k: int = 0; k < 3; k++) {
                if (board[k][j] == 0 and board[k][j] != board[k + 1][j]) {
                    board[k][j] = board[k + 1][j];
                    board[k + 1][j] = 0;
					println("j",j,"l",l,"k",k);
                }
            }
        }

        // adds numbers together
        var i: int = 0;
        if (board[i][j] == board[i + 1][j] and board[i][j] != 0) {
            board[i][j] = 2 * board[i][j];
            board[i + 1][j] = board[i + 2][j];
            board[i + 2][j] = board[i + 3][j];
            board[i + 3][j] = 0;
        }

        i = 1;
        if (board[i][j] == board[i + 1][j] and board[i][j] != 0) {
            board[i][j] = 2 * board[i][j];
            board[i + 1][j] = board[i + 2][j];
            board[i + 2][j] = 0;
        }

        i = 2;
        if (board[i][j] == board[i + 1][j] and board[i][j] != 0) {
            board[i][j] = 2 * board[i][j];
            board[i + 1][j] = 0;
        }
    }
}


var grid[4][4]: int = { 0 };
grid[3][3] = 2;
moves_up(ref grid);
print_board(grid);
