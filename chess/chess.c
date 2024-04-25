
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

int move_pawn(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y, int capture_bool) {
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
        // check if destination is empty
        if (board[to_y][to_x][0] == '*') { 
            if (distance == 1) {
                // Move pawn forward by one square
                board[to_y][to_x][0] = color;
                board[to_y][to_x][1] = 'P';
                board[to_y][to_x][2] = '\0';
                board[from_y][from_x][0] = '*';
                board[from_y][from_x][1] = '*';
                board[from_y][from_x][2] = '\0';
                return 0;

            } 
            // check if the intial 2 step move is starting from position at  start of the game
            else if (distance == 2 && from_y == start_row) {
                // check if there is no pieces in the two postions infront of each pawn
                if (board[from_y + step][from_x][0] == '*' && board[to_y][to_x][0] == '*') {
                    // Move pawn forward by two squares
                    board[to_y][to_x][0] = color;
                    board[to_y][to_x][1] = 'P';
                    board[to_y][to_x][2] = '\0';
                    board[from_y][from_x][0] = '*';
                    board[from_y][from_x][1] = '*';
                    board[from_y][from_x][2] = '\0';
                    return 0;
                } else {
                    return -1;  // Path is blocked, cannot move two squares
                }
            }
        }
    }
    // check for diagonal capture
    //from x and to x must have a different of show for diagnoal and y can only have a difference of 1
     else if (capture_bool && (abs(from_x - to_x) == 1 && abs(from_y - to_y) == 1)) {  
        if (board[to_y][to_x][0] != '*' && board[to_y][to_x][0] != color) {
            // Capture the piece
            board[to_y][to_x][0] = color;
            board[to_y][to_x][1] = 'P';
            board[to_y][to_x][2] = '\0';
            board[from_y][from_x][0] = '*';
            board[from_y][from_x][1] = '*';
            board[from_y][from_x][2] = '\0';
            //print captured piece
            printf("Captured: %s\n", board[to_y][to_x]);  

            return 0;
        }
    }

    return -1;  
}

// Function to move a knight piece on the chessboard
int move_knight(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y, int capture_bool) {
    // horizontal and verticl distance
    int diagx = abs(to_x - from_x);
    int diagy = abs(to_y - from_y);

    // check if move is attemptyin an L shape
    if ((diagx == 2 && diagy == 1) || (diagx == 1 && diagy == 2)) {
        // check if a move is possible when given move command
        if( capture_bool){
            if ( board[to_y][to_x][0] != '*' &&  board[to_y][to_x][0] != color){
                // print capture move
                printf("Captured: %s\n", board[to_y][to_x]);  
                strcpy(board[to_y][to_x], board[from_y][from_x]);
                strcpy(board[from_y][from_x], "**");
                return 0;
            }
            else{
                //print illmove
                printf("Attempted to capture the same color or no piece to capture, invalid move");
                return -1;

            }
        }
        else {
            if (board[to_y][to_x][0] == '*') {
                // Perform the move:
                // Copy the knight from the starting square to the destination square
                strcpy(board[to_y][to_x], board[from_y][from_x]);
                // Set the starting square to empty after the move
                strcpy(board[from_y][from_x], "**");

                // Indicate a successful move
                return 0;
            }
        }
    }

    // If the move is not valid or the destination square is not capturable, return -1
    printf("Attemted to move in incorrect move, must be L shape");
    return -1;
}


int move_bishop(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y, int capture_bool) {

    int diagx = abs(from_x - to_x);
    int diagy = abs(from_y - to_y);
    if(diagx == diagy){
        // must be the same when moving diagnoally
        int x_steps = 0;
        if ( to_x > from_x){
            // moving in the right direction
            x_steps = 1;
        }
        else{
            // moving left
            x_steps = -1;
        }
        int y_steps = 0;
        if( to_y > from_y){
            // moving up 
            y_steps = 1;
        }
        else {
            // moving down 
            y_steps = -1;
        }
        // check path of bishop in front of it
        int curr_x = from_x + x_steps;
        int curr_y = from_y + y_steps;
        int tot_steps = diagx;
        // check all postions except for the last pos, need different case for the last pos
        for(int i = 1; i < diagx ; i++){
            if( board[curr_y][curr_x][0] != '*'){
                printf("Path is blocked when moving the bishop");
                return -1;
            }
            curr_x += x_steps;
            curr_y+= y_steps;

        }
        if( capture_bool && board[to_y][to_x][0]!= '*' && board[to_y][to_x][0] != color ){
            // print captured piece
            printf("Captured: %s\n", board[to_y][to_x]);
            // make move
            strcpy(board[to_y][to_x], board[from_y][from_x]);
            strcpy(board[from_y][from_x], "**");
            return 0;
        }
        // not a capture move, still need to check if last destination location is * and not a friendlt piece
        else if (!capture_bool && (board[to_y][to_x][0] == '*' && board[to_y][to_x][0] != color)){
            strcpy(board[to_y][to_x], board[from_y][from_x]);
            strcpy(board[from_y][from_x], "**");
            return 0;

        }
        else{
            printf("attempting to make incorrect capture with bishsop or location is friendly piece");
            return -1;
        }
    }
    else{
        //illmove
        printf("Did not mot move diagnolly, illmove");
        return -1;
    }

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
    char color, piece, from_col, to_col, capture;
    char captured_piece = '\0';
    int from_row, to_row;
    int scanned = sscanf(command, "%c%c%c%d-%c%d%c%c", &color, &piece, &from_col, &from_row, &to_col, &to_row, &capture, &captured_piece);


    // convert column label to 0 based array index ( a = 0, b = 1)
    int from_x = from_col - 'a';
    int to_x = to_col - 'a';
    // convert row numbers (1-8) to array indices (0-7), where 1 maps to 7, 2 to 6
    int from_y = from_row - 1; 
    int to_y = to_row - 1;


    int capture_bool = 0;
    if(capture == 'x' && captured_piece != '\0'){
        capture_bool = 1;
    }
    char upper_piece = toupper(piece);

    if (upper_piece == 'P') {
        return move_pawn(board, color, from_x, from_y, to_x, to_y, capture_bool);
    } else if (upper_piece == 'N') {
        return move_knight(board, color, from_x, from_y, to_x, to_y, capture_bool);
    } else if (upper_piece == 'B') {
        return move_bishop(board, color, from_x, from_y, to_x, to_y, capture_bool);
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

	

