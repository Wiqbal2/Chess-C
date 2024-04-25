
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define BOARD_SIZE 8

// board size 8x8 and 3 for the W or B and piece as well as null term
void initialize_board(char board[BOARD_SIZE][BOARD_SIZE][3]) {
    // put * in the middle 4 rows
    for (int i = 2; i < 6; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j][0] = '*';
            board[i][j][1] = '*';
            board[i][j][2] = '\0';
        }
    }
     // put all the specialty pieces in the back in their correct order
    char pieces[] = {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'};
    for (int j = 0; j < BOARD_SIZE; j++) {
        board[0][j][0] = 'W';
        board[0][j][1] = pieces[j];
        board[0][j][2] = '\0';
        board[7][j][0] = 'B';
        board[7][j][1] = pieces[j];
        board[7][j][2] = '\0';
    }

    // put pawns
    for (int j = 0; j < BOARD_SIZE; j++) {
        board[1][j][0] = 'W';
        board[1][j][1] = 'P';
        board[1][j][2] = '\0';
        board[6][j][0] = 'B';
        board[6][j][1] = 'P';
        board[6][j][2] = '\0';
    }

   
}

//change the loop to fo from 0-7
void print_board(char board[BOARD_SIZE][BOARD_SIZE][3]) {
    printf("  a  b  c  d  e  f  g  h\n");
    for (int i = 7; i >= 0; i--) {
        printf("%d ", i + 1);
        for (int j = 0; j < BOARD_SIZE; j++) {
            printf("%s ", board[i][j]);
        }
        printf("\n");
    }
}

int move_pawn(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y) {

    // check if user move is trying to go vertical 
    if (from_x == to_x) {
        int step;
        int start_row;

        if (color == 'W') {
            // moves up the board if white piece 
            step = 1;
            // Pawns start 1 row past the bottom 
            start_row = 1; 
        } else {
            //black moves down the board 
            step = -1; 
            // Pawns start one row below the very top
            start_row = 6; 
        }
        int distance = abs(to_y - from_y);
        // Check if moving forward one or two squares
        if ((distance == 1 || (distance == 2 && from_y == start_row)) && board[to_y][to_x][0] == '*') {
            if (distance == 2 && board[from_y + step][from_x][0] != '*') {
                return -1;  // Blocked path
            }
            // Move the pawn
            board[to_y][to_x][0] = color;
            board[to_y][to_x][1] = 'P';
            board[to_y][to_x][2] = '\0';
            board[from_y][from_x][0] = '*';
            board[from_y][from_x][1] = '*';
            board[from_y][from_x][2] = '\0';
            return 0;
        }
    }
    // check for diagonal capture
     else if (abs(from_x - to_x) == 1 && abs(from_y - to_y) == 1) {  
        if (board[to_y][to_x][0] != '*' && board[to_y][to_x][0] != color) {
            // Capture the piece
            board[to_y][to_x][0] = color;
            board[to_y][to_x][1] = 'P';
            board[to_y][to_x][2] = '\0';
            board[from_y][from_x][0] = '*';
            board[from_y][from_x][1] = '*';
            board[from_y][from_x][2] = '\0';
            return 0;
        }
    }

    return -1;  // Illegal move
}
int is_path_clear(char board[BOARD_SIZE][BOARD_SIZE][3], int start_x, int start_y, int end_x, int end_y) {
   return 0;
}

// Function to move a knight piece on the chessboard
int move_knight(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y) {
    return 0;
}

int move_bishop(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y) {
    return 0;
}
int move_rook(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y) {
    return 0;
}
int move_queen(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y) {
    return 0;
}
int move_king(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y) {
    return 0;
}
int handle_command(char board[BOARD_SIZE][BOARD_SIZE][3], const char *command) {
    char color, piece, from_col, to_col;
    int from_row, to_row;
    int scanned = sscanf(command, "%c%c%c%d-%c%d", &color, &piece, &from_col, &from_row, &to_col, &to_row);

    // invalid format given 
    if (scanned != 6) {
        return -1;  
    }

    // convert column label to 0 based array index ( a = 0, b = 1)
    int from_x = from_col - 'a';
    int to_x = to_col - 'a';
    // convert row numbers (1-8) to array indices (0-7), where 1 maps to 7, 2 to 6
    int from_y = 7 - (from_row - 1);
    int to_y = 7 - (to_row - 1);

    char upper_piece = toupper(piece);

    if (upper_piece == 'P') {
        return move_pawn(board, color, from_x, from_y, to_x, to_y);
    } else if (upper_piece == 'N') {
        return move_knight(board, color, from_x, from_y, to_x, to_y);
    } else if (upper_piece == 'B') {
        return move_bishop(board, color, from_x, from_y, to_x, to_y);
    } else if (upper_piece == 'R') {
        return move_rook(board, color, from_x, from_y, to_x, to_y);
    } else if (upper_piece == 'Q') {
        return move_queen(board, color, from_x, from_y, to_x, to_y);
    } else if (upper_piece == 'K') {
        return move_king(board, color, from_x, from_y, to_x, to_y);
    } else {
        return -1;  
    }
}



int main() {
    char board[BOARD_SIZE][BOARD_SIZE][3];
    char command[20];
    initialize_board(board);
    print_board(board);

    // user input loop
    printf("Enter your move (e.g 'WPe2-e4'): ");
    while (fgets(command, sizeof(command), stdin)) {
        if (strcmp(command, "quit\n") == 0) {
            break;
        }
        if (handle_command(board, command) == 0) {
            print_board(board);
        } else {
            printf("Invalid move . Try again.\n");
        }
        printf("Enter your move (or quit): ");
    }

    return 0;
}

	

