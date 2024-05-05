	

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define BOARD_SIZE 8


typedef enum {
    STATE_UNINITIALIZED,
    STATE_RUNNING,
    STATE_NO_GAME
} GameState;

typedef struct {
    char board[BOARD_SIZE][BOARD_SIZE][3];
    char current_player;
    GameState state;
} ChessGame;





typedef struct {
    int from_x, from_y;
    int to_x, to_y;
} Move;

typedef struct {
    Move moves[1000];
    int count;
} MoveList;





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
    for (int i = BOARD_SIZE - 1; i >= 0; i--) {
        printf("%d ", i + 1);
        for (int j = 0; j < BOARD_SIZE; j++) {
            printf("%s ", board[i][j]);
        }
        printf("\n");
    }
}
void initialize_game(ChessGame *game, char player) {
    initialize_board(game->board);
    game->current_player = player;
    game->state = STATE_RUNNING;
    printf("New game started. Player %c begins.\n", player);
    print_board(game->board);
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
int move_pawn_promote(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y, int capture_bool, char promotion_piece) {
    // Determine the step direction based on the pawn's color
    int step = 0;
    if (color == 'W') {
        // w goes up
        step = 1;  
    } else {
        // b moves down
        step = -1; 
    }

    int promotion_row;
    if (color == 'W') {
        // white pawnpromote on the 8th row
        promotion_row = 7;  
    } else {
        // black pawn promote on the 1st row
        promotion_row = 0;  
    }

    // check if destination pos is at promotion row 
    if (to_y == promotion_row) {
        // check if move is either forward or diagonal capture
        if ((from_x == to_x && from_y + step == to_y && !capture_bool && board[to_y][to_x][0] == '*') ||
            (capture_bool && abs(from_x - to_x) == 1 && from_y + step == to_y && board[to_y][to_x][0] != '*' && board[to_y][to_x][0] != color)) {
            // free the space from beofre
            strncpy(board[from_y][from_x], "**", 3);

            // set up new pos with the new promoted piece
            board[to_y][to_x][0] = color;
            board[to_y][to_x][1] = promotion_piece;
            board[to_y][to_x][2] = '\0';

            // capture output
            if (capture_bool) {
                printf("Captured %s and promoted to %c.\n", board[to_y][to_x], promotion_piece);
            } else {
                printf("Pawn promoted to %c.\n", promotion_piece);
            }

            return 0; 
        } else {
            // illmove
            printf(" Move must be forward to an empty square or a diagonal capture.");
            return -1; // Invalid move
        }
    } else {
        // pawn did not reach the promotion row
        printf("Invalid move pawn did not reach promotion row");
        return -1; 
    }
}


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
        for(int i = 1; i < tot_steps ; i++){
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
        else if (!capture_bool && (board[to_y][to_x][0] == '*' )){
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
int move_rook(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y, int capture_bool) {
    // check to see if the move is either hozizontal or vertical
    if(from_x == to_x || from_y == to_y){
        // must check if the path is clear or obstructed
        int x_steps = 0;
        int y_steps = 0;
        // check to see if moving hozontally 
        if( from_x != to_x){
            if ( to_x > from_x){
                // moving in the right direction
                x_steps = 1;
            }
            else{
                // moving left
                x_steps = -1;
            }
        }
        // check to see if moving vertically
        else if (from_y != to_y){
            if( to_y > from_y){
                // moving up 
                y_steps = 1;
            }
            else {
                // moving down 
                y_steps = -1;
            }
        }
        // check path of rook in front of it
        int curr_x = from_x + x_steps;
        int curr_y = from_y + y_steps;
 
        // iterate up untill the pos right beofre the destination
        while (curr_x != to_x || curr_y != to_y){
            if( board[curr_y][curr_x][0] != '*'){
                printf("Path is blocked when moving the bishop");
                return -1;
            }
            curr_x += x_steps;
            curr_y+= y_steps;

        }
        // check destination space
        // check if capture move and check if last move is not same color and is not free
        if( capture_bool && (board[to_y][to_x][0] != '*' && board[to_y][to_x][0] != color)){
            // print captured piece
            printf("Captured: %s\n", board[to_y][to_x]);
            // make move
            strcpy(board[to_y][to_x], board[from_y][from_x]);
            strcpy(board[from_y][from_x], "**");
            return 0;
        }
        // make regular move if pos is free and filled with the same color
        else if( !capture_bool && (board[to_y][to_x][0] == '*' )){
            strcpy(board[to_y][to_x], board[from_y][from_x]);
            strcpy(board[from_y][from_x], "**");
            return 0;
        }
        else{
            printf( " attempted an incorrect capture or wrong rook move");
            return -1;
        }
    }
    else{
        // illmove
        printf(" Rook can only move horizontal or vertical");
        return -1;
    }
}

int move_queen(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y, int capture_bool) {
    // get move distcance in x and y direction
    int distance_x = abs(to_x - from_x);
    int distance_y = abs(from_y - to_y);
    int x_steps = 0;
    int y_steps = 0;
    // move can be diagnoal, horizontal or vertical as long its path is clear
    if( distance_x == distance_y || from_x == to_x || from_y == to_y){
        if( to_x > from_x){
            // moving right
            x_steps = 1;
        }
        else if ( to_x < from_x){
            // moving left
            x_steps = -1;
        }
        if (to_y > from_y){
            // moving up
            y_steps = 1;
        }
        else if (to_y < from_y){
            //moving down
            y_steps = -1;
        }
        // check path of queen in front of it
        int curr_x = from_x + x_steps;
        int curr_y = from_y + y_steps;
 
        // iterate up untill the pos right beofre the destination
        while (curr_x != to_x || curr_y != to_y){
            if( board[curr_y][curr_x][0] != '*'){
                printf("Path is blocked when moving the queen");
                return -1;
            }
            curr_x += x_steps;
            curr_y+= y_steps;

        }
        // check destination space
        // check if capture move and check if last move is not same color and is not free
        if( capture_bool && (board[to_y][to_x][0] != '*' && board[to_y][to_x][0] != color)){
            // print captured piece
            printf("Captured: %s\n", board[to_y][to_x]);
            // make move
            strcpy(board[to_y][to_x], board[from_y][from_x]);
            strcpy(board[from_y][from_x], "**");
            return 0;
        }
        // make regular move if pos is free and filled with the same color
        else if( !capture_bool && (board[to_y][to_x][0] == '*' )){
            strcpy(board[to_y][to_x], board[from_y][from_x]);
            strcpy(board[from_y][from_x], "**");
            return 0;
        }
        else{
            printf( " attempted an incorrect capture move for the queen");
            return -1;
        }
    }
    else{
        // illmove
        printf(" Made incorrect move for the queen");
        return -1;
    }
}
int move_king(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y, int capture_bool) {

    int distance_x = abs(to_x - from_x);
    int distance_y = abs(from_y - to_y);
    // checking the distance is at most one space. can only be on space up, one space left or right or one space left or right 
    if(distance_x <= 1 && distance_y <= 1 ){
        // check if capturing destination pos has opposite peice
        if( board[to_y][to_x][0] != color && capture_bool && board[to_y][to_x][0] != '*' ){
            // print captured piece
            printf("Captured: %s\n", board[to_y][to_x]);
            // make move
            strcpy(board[to_y][to_x], board[from_y][from_x]);
            strcpy(board[from_y][from_x], "**");
            return 0;
        }
        // making a normal move, making sure the pos is free
        else if ( !capture_bool && board[to_y][to_x][0] == '*'){
            strcpy(board[to_y][to_x], board[from_y][from_x]);
            strcpy(board[from_y][from_x], "**");
            return 0;
        }
        else{
            // attempted to move to a piece that is blocked 
            //illmove
            printf("Attempted to move to a block piece or try to capture incorrectly with the king");
            return -1;
        }
    }
    else{
        //illmove, incorrect move action 
        printf(" King can only move one space at a time");
        return -1;
    }
  
}


int handle_command(char board[BOARD_SIZE][BOARD_SIZE][3], const char *command) {
    char color, piece, from_col, to_col, promotion = '\0', promotion_piece = '\0';
    char captured_piece_type = '\0', capture = '\0';
    int from_row, to_row;

    // Buffer to hold command for manipulation
    char buf[32];
    strncpy(buf, command, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0'; // Ensure null termination

    int offset = 0;
    color = buf[offset++];
    piece = buf[offset++];
    from_col = buf[offset++];
    from_row = buf[offset++] - '0';
    if (buf[offset] == '-') offset++; // Skip dash
    to_col = buf[offset++];
    to_row = buf[offset++] - '0';

    // check for additional actions for captures or promotions
while (buf[offset]) {
    // capture
    if (buf[offset] == 'x') { 
        capture = 'x';
        captured_piece_type = buf[++offset];
        // promotion
    } else if (buf[offset] == 'y') { 
        promotion = 'y';
        promotion_piece = buf[++offset];
    }

    offset++;
}

    // convert column label to 0 based array index ( a = 0, b = 1)
    int from_x = from_col - 'a';
    int to_x = to_col - 'a';

    // convert row numbers (1-8) to array indices (0-7), where 1 maps to 7, 2 to 6
    int from_y = from_row - 1; 
    int to_y = to_row - 1;

    int capture_bool = (capture == 'x');
    int promotion_bool = (promotion == 'y' && (promotion_piece == 'Q' || promotion_piece == 'R' ||
                                               promotion_piece == 'B' || promotion_piece == 'N'));

    printf("Parsed move: %c%c from %c%d to %c%d\n", color, piece, from_col, from_row, to_col, to_row);
    if (capture_bool) {
        printf("Capture indicated with piece type %c\n", captured_piece_type);
    }
    if (promotion_bool) {
        printf("Promotion indicated to %c\n", promotion_piece);
    }
    //convert to upper piece
    char upper_piece = toupper(piece); 

    // Check which piece type and perform corresponding move
    if (upper_piece == 'P') {
        return move_pawn(board, color, from_x, from_y, to_x, to_y, capture_bool);
    } else if (upper_piece == 'N') {
        return move_knight(board, color, from_x, from_y, to_x, to_y, capture_bool);
    } else if (upper_piece == 'B') {
        return move_bishop(board, color, from_x, from_y, to_x, to_y, capture_bool);
    } else if (upper_piece == 'R') {
        return move_rook(board, color, from_x, from_y, to_x, to_y, capture_bool);
    } else if (upper_piece == 'Q') {
        return move_queen(board, color, from_x, from_y, to_x, to_y, capture_bool);
    } else if (upper_piece == 'K') {
        return move_king(board, color, from_x, from_y, to_x, to_y, capture_bool);
    } else {
       
        printf("Invalid move");
        return -1;
    }

    return -1; 
}





int is_path_clear(char board[BOARD_SIZE][BOARD_SIZE][3], int from_x, int from_y, int to_x, int to_y) {
    // make sure movement is there 
    if (from_x == to_x && from_y == to_y) {
        return 1;
    }

    int x_step = 0;
    int y_step = 0;

    if (to_x > from_x) {
        x_step = 1;
    } else if (to_x < from_x) {
        x_step = -1;
    }

    if (to_y > from_y) {
        y_step = 1;
    } else if (to_y < from_y) {
        y_step = -1;
    }

    int x = from_x + x_step;
    int y = from_y + y_step;

    // stop beofre the final pos
    while (x != to_x || y != to_y) {
        if (board[y][x][0] != '*') {  
            printf("Path blocked at (%d, %d)\n", x, y);
            return 0; 
        }
        // increment
        x += x_step;
        y += y_step;
    }

    return 1; // Path is clear
}

int is_valid_move(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y) {
    if (board[from_y][from_x][0] != color) {
        printf("Invalid move: not player's piece.\n");
        return 0;
    }

    char piece = board[from_y][from_x][1];
    int dx = to_x - from_x;
    int dy = to_y - from_y;
    int abs_dx = abs(dx);
    int abs_dy = abs(dy);

    if (piece == 'P') {  // Pawn
        int forward;
        if (color == 'W') {
            forward = 1;
        } else {
            forward = -1;
        }
        if (dx == 0 && dy == forward && board[to_y][to_x][0] == '*') {
            return 1;
        } else if (dx == 0 && dy == 2 * forward && (from_y == 1 || from_y == 6) && board[to_y][to_x][0] == '*' && board[from_y + forward][from_x][0] == '*') {
            return 1;
        } else if (abs_dx == 1 && dy == forward && board[to_y][to_x][0] != '*' && board[to_y][to_x][0] != color) {
            return 1;
        }
    } else if (piece == 'N') {  // Knight
        if ((abs_dx == 2 && abs_dy == 1) || (abs_dx == 1 && abs_dy == 2)) {
            return 1;
        }
    } else if (piece == 'B') {  // Bishop
        if (abs_dx == abs_dy) {
            return is_path_clear(board, from_x, from_y, to_x, to_y);
        }
    } else if (piece == 'R') {  // Rook
        if (dx == 0 || dy == 0) {
            return is_path_clear(board, from_x, from_y, to_x, to_y);
        }
    } else if (piece == 'Q') {  // Queen
        if (abs_dx == abs_dy || dx == 0 || dy == 0) {
            return is_path_clear(board, from_x, from_y, to_x, to_y);
        }
    } else if (piece == 'K') {  // King
        if (abs_dx <= 1 && abs_dy <= 1) {
            return 1;
        }
    } else {
        printf("Invalid move: unrecognized piece type.\n");
        return 0;
    }

    printf("Invalid move attempted by %c from (%d, %d) to (%d, %d)\n", piece, from_x, from_y, to_x, to_y);
    return 0;
}


void collect_valid_moves(char board[BOARD_SIZE][BOARD_SIZE][3], char color, MoveList *move_list) {
    // reset the list
    move_list->count = 0;  

    // travel through the board
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            // change the correct color side
            if (board[y][x][0] == color) {  
                for (int dy = 0; dy < BOARD_SIZE; dy++) {
                    for (int dx = 0; dx < BOARD_SIZE; dx++) {
                        if (is_valid_move(board, color, x, y, dx, dy)) {
                            if (move_list->count < 1000) {
                                move_list->moves[move_list->count].from_x = x;
                                move_list->moves[move_list->count].from_y = y;
                                move_list->moves[move_list->count].to_x = dx;
                                move_list->moves[move_list->count].to_y = dy;
                                move_list->count++;
                            }
                        }
                    }
                }
            }
        }
    }
}
void execute_cpu_move(char board[BOARD_SIZE][BOARD_SIZE][3], char color) {
    MoveList move_list;
    // possible moves
    collect_valid_moves(board, color, &move_list);

    if (move_list.count > 0) {
        // randomness
        srand(time(NULL));  
        int index = rand() % move_list.count;  
        Move selected_move = move_list.moves[index];

        // perform the move
        board[selected_move.to_y][selected_move.to_x][0] = board[selected_move.from_y][selected_move.from_x][0];
        board[selected_move.to_y][selected_move.to_x][1] = board[selected_move.from_y][selected_move.from_x][1];
        board[selected_move.from_y][selected_move.from_x][0] = '*';
        board[selected_move.from_y][selected_move.from_x][1] = '*';

    } else {
        printf("No valid moves available for CPU.\n");
    }
}



int process_command(ChessGame *game, const char *command) {

    if (strncmp(command, "00 ", 3) == 0) {
        if (command[3] == 'W' || command[3] == 'B') {
            initialize_game(game, command[3]);
            return 0;
        }
        return -1; // Malformed command
    } else if (strcmp(command, "01\n") == 0) {
        if (game->state != STATE_RUNNING) {
            printf("NOGAME\n");
        } else {
            print_board(game->board);
        }
        return 0;
    } else if (strncmp(command, "02 ", 3) == 0) {
        if (game->state != STATE_RUNNING) {
            printf("NOGAME\n");
            return -1;
        }
        // actual move command starts after "02 "
        if (handle_command(game->board, command + 3) == 0) {
            print_board(game->board);
        } else {
            printf("ILLMOVE\n");
        }
        return 0;
    } else if (strcmp(command, "03\n") == 0) {
        
        if (game->state != STATE_RUNNING) {
            printf("NOGAME\n");
            return -1;
        }else{
            if(game->current_player == 'W'){
                execute_cpu_move(game->board,'B' );
            }
            else{
                execute_cpu_move(game->board, 'W');
            }
         
        }
        return 0;
     
        
    } else if (strcmp(command, "04\n") == 0) {
        if (game->state != STATE_RUNNING) {
            printf("NOGAME\n");
            return -1;
        }
        game->state = STATE_NO_GAME;
        printf("Game resigned. CPU wins.\n");
        return 0;
    }
    return -1; // Unknown or malformed command
}



int main() {
    ChessGame game;
    game.state = STATE_UNINITIALIZED; // set initial state to uninitialized
    char command[100]; // buffer for user commands

    printf("Chess game started. Enter commands to play.\n");
    printf("Available commands:\n");
    printf("  00 W/B - Start new game as White (W) or Black (B)\n");
    printf("  01 - Get current game state\n");
    printf("  02 [move] - Make a move (e.g., WPe2-e4xP)\n");
    printf("  03 - CPU makes a move\n");
    printf("  04 - Resign game\n");
    printf("  exit - Exit game\n");

   
    while (1) {
        printf("Enter command: ");
        // read data in
        if (fgets(command, sizeof(command), stdin)) { // read data 
            if (strcmp(command, "exit\n") == 0) { 
                break; 
            }
            if (process_command(&game, command) == 0) {
                printf("Command processed successfully.\n");
            } else {
                printf("Error processing command. Please try again.\n");
            }
        }
    }

    printf("Exiting chess game. Goodbye!\n");
    return 0;
}

