using cp.core.console;
using cp.std.random;

as namespace cp;

const PRINT_HORIZONTAL_LINE = "+---------------------------------------+";
const PRINT_BLANK_LINE = "                                       ";

fun set_color(val: int) {
    if (val == 2) {
        set_console_color(CL_BLACK, CL_BLUE);
    } else if (val == 4) {
        set_console_color(CL_BLACK, CL_GREEN);
    } else if (val == 8) {
        set_console_color(CL_BLACK, CL_AQUA);
    } else if (val == 16) {
        set_console_color(CL_BLACK, CL_RED);
    } else if (val == 32) {
        set_console_color(CL_BLACK, CL_PURPLE);
    } else if (val == 64) {
        set_console_color(CL_BLACK, CL_YELLOW);
    } else if (val == 128) {
        set_console_color(CL_BLACK, CL_LIGHT_BLUE);
    } else if (val == 256) {
        set_console_color(CL_BLACK, CL_LIGHT_GREEN);
    } else if (val == 512) {
        set_console_color(CL_BLACK, CL_LIGHT_AQUA);
    } else if (val == 1024) {
        set_console_color(CL_BLACK, CL_LIGHT_RED);
    } else if (val == 2048) {
        set_console_color(CL_BLACK, CL_LIGHT_PURPLE);
    } else if (val == 4096) {
        set_console_color(CL_BLACK, CL_LIGHT_YELLOW);
    } else if (val == 8192) {
        set_console_color(CL_BLACK, CL_GRAY);
    } else {
        set_console_color(CL_BLACK, CL_WHITE);
    }
}

fun print_board(board[4][4]: int) {
    system("cls");
    print(PRINT_HORIZONTAL_LINE);
    for (var i: int = 0; i < 4; i++) {
        print("\n|" + PRINT_BLANK_LINE + "|\n|" + PRINT_BLANK_LINE + "|\n|");
        for (var j: int = 0; j < 4; j++) {
			set_color(board[i][j]);
            print("\t" + string(board[i][j]));
        }
        set_console_color(CL_BLACK, CL_WHITE);
        print("\t|\n|" + PRINT_BLANK_LINE + "|");
    }
    print("\n|" + PRINT_BLANK_LINE + "|\n");
    print(PRINT_HORIZONTAL_LINE);
}

fun moves_up(board[4][4]: int) {
    for (var j: int = 0; j < 4; j++) {
        // moves everything close together
        for (var l: int = 0; l < 3; l++) {
            for (var k: int = 0; k < 3; k++) {
                if (board[k][j] == 0 and board[k][j] != board[k + 1][j]) {
                    board[k][j] = board[k + 1][j];
                    board[k + 1][j] = 0;
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

fun moves_down(board[4][4]: int) {
    for (var j: int = 0; j < 4; j++) {
        // moves everything close together
        for (var l: int = 0; l < 3; l++) {
            for (var k: int = 3; k > 0; k--) {
                if (board[k][j] == 0 and board[k][j] != board[k - 1][j]) {
                    board[k][j] = board[k - 1][j];
                    board[k - 1][j] = 0;
                }
            }
        }

        // adds numbers together
        var i: int = 3;
        if (board[i][j] == board[i - 1][j] and board[i][j] != 0) {
            board[i][j] = 2 * board[i][j];
            board[i - 1][j] = board[i - 2][j];
            board[i - 2][j] = board[i - 3][j];
            board[i - 3][j] = 0;
        }

        i = 2;
        if (board[i][j] == board[i - 1][j] and board[i][j] != 0) {
            board[i][j] = 2 * board[i][j];
            board[i - 1][j] = board[i - 2][j];
            board[i - 2][j] = 0;
        }

        i = 1;
        if (board[i][j] == board[i - 1][j] and board[i][j] != 0) {
            board[i][j] = 2 * board[i][j];
            board[i - 1][j] = 0;
        }
    }
}

fun moves_left(board[4][4]: int) {
    for (var i: int = 0; i < 4; i++) {
        // moves everything close together
        for (var l: int = 0; l < 3; l++) {
            for (var k: int = 0; k < 3; k++) {
                if (board[i][k] == 0 and board[i][k] != board[i][k + 1]) {
                    board[i][k] = board[i][k + 1];
                    board[i][k + 1] = 0;
                }
            }
        }

        // adds numbers together
        var j: int = 0;
        if (board[i][j] == board[i][j + 1] and board[i][j] != 0) {
            board[i][j] = 2 * board[i][j];
            board[i][j + 1] = board[i][j + 2];
            board[i][j + 2] = board[i][j + 3];
            board[i][j + 3] = 0;
        }

        j = 1;
        if (board[i][j] == board[i][j + 1] and board[i][j] != 0) {
            board[i][j] = 2 * board[i][j];
            board[i][j + 1] = board[i][j + 2];
            board[i][j + 2] = 0;
        }

        j = 2;
        if (board[i][j] == board[i][j + 1] and board[i][j] != 0) {
            board[i][j] = 2 * board[i][j];
            board[i][j + 1] = 0;
        }
    }
}

fun moves_right(board[4][4]: int) {
    for (var i: int = 0; i < 4; i++) {
        // moves everything close together
        for (var l: int = 0; l < 3; l++) {
            for (var k: int = 3; k > 0; k--) {
                if (board[i][k] == 0 and board[i][k] != board[i][k - 1]) {
                    board[i][k] = board[i][k - 1];
                    board[i][k - 1] = 0;
                }
            }
        }

        // adds numbers together
        var j: int = 3;
        if (board[i][j] == board[i][j - 1] and board[i][j] != 0) {
            board[i][j] = 2 * board[i][j];
            board[i][j - 1] = board[i][j - 2];
            board[i][j - 2] = board[i][j - 3];
            board[i][j - 3] = 0;
        }

        j = 2;
        if (board[i][j] == board[i][j - 1] and board[i][j] != 0) {
            board[i][j] = 2 * board[i][j];
            board[i][j - 1] = board[i][j - 2];
            board[i][j - 2] = 0;
        }

        j = 1;
        if (board[i][j] == board[i][j - 1] and board[i][j] != 0) {
            board[i][j] = 2 * board[i][j];
            board[i][j - 1] = 0;
        }
    }
}

fun is_valid_move(v: bool, board[4][4]: int, boardcopy[4][4]: int): bool {
    for (var i: int = 0; i < 4; i++) {
        for (var j: int = 0; j < 4; j++) {
            if (board[i][j] != boardcopy[i][j]) {
                v = true;
            }
        }
    }

    return v;
}

fun counts_zero(board[4][4]: int): int {
    var n: int = 0;
    for (var i: int = 0; i < 4; i++) {
        for (var j: int = 0; j < 4; j++) {
            if (board[i][j] == 0) { n++; }
        }
    }

    return n;
}

fun spawn_random(n: int, board[4][4]: int) {
    var spawn: int = 1 + (cp::randi() % n);
    var p: int = 0;
    var spawn2: bool = true;
    for (var i: int = 0; i < 4; i++) {
        for (var j: int = 0; j < 4; j++) {
            if (board[i][j] == 0) {
                p++;
            }
            if (p == spawn and spawn2 == true) {
                board[i][j] = 2;
                spawn2 = false;
            }
        }
    }
}

fun is_end_game(c1: bool, c2: bool, board[4][4]: int): bool {
    c1 = false;
    c2 = true;
    for (var i: int = 0; i < 4; i++) {
        for (var j: int = 0; j < 4; j++) {
            if (j > 0 and board[i][j] == board[i][j - 1]) {
                c1 = true;
            }
            else if (i > 0 and board[i][j] == board[i - 1][j]) {
                c1 = true;
            }
            c2 = c1;
        }
    }

    return c2;
}

if (this == "2048") {
    //srand(time(NULL));
    
    var grid[4][4]: int = {0};

    grid[3][3] = 2;

    print_board(grid);

    var direction: string;
    var end: bool = false;

    while (not end) {
        var gridcopy[4][4]: int = {0};
        
        for (var i: int = 0; i < 4; i++) {
            for (var j: int = 0; j < 4; j++) {
                gridcopy[i][j] = grid[i][j];
            }
        }
        
        direction = readch();
        
        if (direction == "w") {
            moves_up(ref grid);
        }
        else if (direction == "s") {
            moves_down(ref grid);
        }
        else if (direction == "a") {
            moves_left(ref grid);
        }
        else if (direction == "d") {
            moves_right(ref grid);
        }

        var valid: bool = false;
        valid = is_valid_move(valid, grid, gridcopy);

        if (valid)  {
            var z: int = counts_zero(grid);

            if (z > 0) {
                spawn_random(z, ref grid);
            }

            print_board(grid);
            z = counts_zero(grid);
            var cont: bool = false;
            var cont2: bool = true;

            if (z == 0) {
                cont2 = is_end_game(cont, cont2, grid);
            }
            
            if (not cont2) {
                end = true;
                print("GAME OVER\n");
                // exit(0);
            }
        }
    }

}
