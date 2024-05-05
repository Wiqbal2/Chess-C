
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>  
#include <linux/random.h>
#define DEVICE_NAME "chessgame"
#define CLASS_NAME "chess"
#define BOARD_SIZE 8
static int majorNumber;
static struct class* chessClass = NULL;
static struct device* chessDevice = NULL;
static struct cdev c_dev;

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

// store moves for cpu to choose
typedef struct {
    Move moves[1000]; // to hold moves
    int count;        // num of moves 
} MoveList;

static ChessGame game;

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char __user *, size_t, loff_t *);

static struct file_operations fops = {
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
};

// board size 8x8 and 3 for the W or B and piece as well as null term
void initialize_board(char board[BOARD_SIZE][BOARD_SIZE][3]) {
    // put * in the middle 4 rows
    int i = 0;
    int j = 0;
    for ( i = 2; i < 6; i++) {
        for ( j = 0; j < BOARD_SIZE; j++) {
            board[i][j][0] = '*';
            board[i][j][1] = '*';
            board[i][j][2] = '\0';
        }
    }
    // put all the specialty pieces in the back in their correct order
    char pieces[] = {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'};
    for ( j = 0; j < BOARD_SIZE; j++) {
        board[0][j][0] = 'W';
        board[0][j][1] = pieces[j];
        board[0][j][2] = '\0';
        board[7][j][0] = 'B';
        board[7][j][1] = pieces[j];
        board[7][j][2] = '\0';
    }
    // put pawns
    for ( j = 0; j < BOARD_SIZE; j++) {
        board[1][j][0] = 'W';
        board[1][j][1] = 'P';
        board[1][j][2] = '\0';
        board[6][j][0] = 'B';
        board[6][j][1] = 'P';
        board[6][j][2] = '\0';
    }
}


void print_board(char board[BOARD_SIZE][BOARD_SIZE][3]) {
    int i = 0;
    int j = 0;
    // print letters and numbers to its own row/col
    printk(KERN_INFO "  a  b  c  d  e  f  g  h\n");
    for ( i = BOARD_SIZE - 1; i >= 0; i--) {
        printk(KERN_INFO "%d ", i + 1);
        for ( j = 0; j < BOARD_SIZE; j++) {
            printk(KERN_CONT "%s ", board[i][j]);
        }
        printk(KERN_CONT "\n");
    }
}

// initialize game and print the board
void initialize_game(ChessGame *game, char player) {
    initialize_board(game->board);
    game->current_player = player;
    game->state = STATE_RUNNING;
    printk(KERN_INFO "New game started. Player %c begins.\n", player);
    print_board(game->board);
}

// check if user move is trying to go vertical 
int move_pawn(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y, int capture_bool) {
    int step;
    int start_row;
    if (color == 'W') {
        // moves up the board if white piece 
        step = 1;
        // pawns start 1 row past the bottom 
        start_row = 1; 
    } else {
        // black moves down the board 
        step = -1;
        // pawns start one row below the very top
        start_row = 6;
    }

    int distance = abs(to_y - from_y);
    // check if destination is empty
    if (board[to_y][to_x][0] == '*') {
        if (distance == 1) {
            // move pawn forward by one square
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
                // move pawn forward by two squares
                board[to_y][to_x][0] = color;
                board[to_y][to_x][1] = 'P';
                board[to_y][to_x][2] = '\0';
                board[from_y][from_x][0] = '*';
                board[from_y][from_x][1] = '*';
                board[from_y][from_x][2] = '\0';
                return 0;
            } else {
                return -1; // path is blocked, cannot move two squares
            }
        }
        // check for diagonal capture
        //from x and to x must have a different of show for diagnoal and y can only have a difference of 1
    } else if (capture_bool && (abs(from_x - to_x) == 1 && abs(from_y - to_y) == 1)) {
        if (board[to_y][to_x][0] != '*' && board[to_y][to_x][0] != color) {
            // capture the piece
            board[to_y][to_x][0] = color;
            board[to_y][to_x][1] = 'P';
            board[to_y][to_x][2] = '\0';
            board[from_y][from_x][0] = '*';
            board[from_y][from_x][1] = '*';
            board[from_y][from_x][2] = '\0';
            printk(KERN_INFO "Captured: %s\n", board[to_y][to_x]); 
            return 0;
        }
    }
    return -1;
}

// determine the step direction based on the pawn's color
int move_pawn_promote(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y, int capture_bool, char promotion_piece) {
    int step;
    int promotion_row;

    if (color == 'W') {
        step = 1;  
        promotion_row = 7;
    } else {
        step = -1;
        promotion_row = 0;
    }

    // check if destination pos is at promotion row
    if (to_y == promotion_row) {
        if ((from_x == to_x && from_y + step == to_y && !capture_bool && board[to_y][to_x][0] == '*') ||
            (capture_bool && abs(from_x - to_x) == 1 && from_y + step == to_y && board[to_y][to_x][0] != '*' && board[to_y][to_x][0] != color)) {
            board[from_y][from_x][0] = '*';
            board[from_y][from_x][1] = '*';
            board[from_y][from_x][2] = '\0';
            board[to_y][to_x][0] = color;
            board[to_y][to_x][1] = promotion_piece;
            board[to_y][to_x][2] = '\0';

            if (capture_bool) {
                printk(KERN_INFO "Captured %s and promoted to %c.\n", board[to_y][to_x], promotion_piece);
            } else {
                printk(KERN_INFO "Pawn promoted to %c.\n", promotion_piece);
            }
            return 0;
        } else {
            printk(KERN_INFO "Move must be forward to an empty square or a diagonal capture.\n");
            return -1; // invalid move
        }
    } else {
        printk(KERN_INFO "Invalid move pawn did not reach promotion row\n");
        return -1;
    }
}

int move_knight(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y, int capture_bool) {
    // horizontal and vertical distance
    int diagx = abs(to_x - from_x);
    int diagy = abs(to_y - from_y);

    // check if move is attempting an L shape
    if ((diagx == 2 && diagy == 1) || (diagx == 1 && diagy == 2)) {
        // check if a move is possible when given move command
        if (capture_bool) {
            if (board[to_y][to_x][0] != '*' && board[to_y][to_x][0] != color) {
                // print capture move
                printk(KERN_INFO "Captured: %s\n", board[to_y][to_x]);  
                memcpy(board[to_y][to_x], board[from_y][from_x], 3);
                memset(board[from_y][from_x], '*', 2);
                board[from_y][from_x][2] = '\0';
                return 0;
            } else {
                // print ill move
                printk(KERN_INFO "Attempted to capture the same color or no piece to capture, invalid move");
                return -EINVAL; // -EINVAL typically used for invalid arguments in the kernel
            }
        } else {
            if (board[to_y][to_x][0] == '*') {
                // perform the move:
                // copy the knight from the starting square to the destination square
                memcpy(board[to_y][to_x], board[from_y][from_x], 3);
                // set the starting square to empty after the move
                memset(board[from_y][from_x], '*', 2);
                board[from_y][from_x][2] = '\0';

                // indicate a successful move
                return 0;
            }
        }
    }

    // if the move is not valid or the destination square is not capturable, return -EINVAL
    printk(KERN_INFO "Attempted to move in incorrect move, must be L shape");
    return -EINVAL;
}
int move_bishop(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y, int capture_bool) {
    int diagx = abs(from_x - to_x);
    int diagy = abs(from_y - to_y);

    if (diagx == diagy) {
        // must be the same when moving diagonally
        int x_steps, y_steps;
        if (to_x > from_x) {
             // moving in the right direction
            x_steps = 1;
        } else {
            // left
            x_steps = -1;
        }
        if (to_y > from_y) {
            // up
            y_steps = 1;
        } else {
            // down
            y_steps = -1;
        }

        // check path of bishop in front of it
        int curr_x = from_x + x_steps;
        int curr_y = from_y + y_steps;
        int tot_steps = diagx;

        // check all positions except for the last pos, need different case for the last pos
        int i = 0;
        for ( i = 1; i < tot_steps; i++) {
            if (board[curr_y][curr_x][0] != '*') {
                printk(KERN_INFO "Path is blocked when moving the bishop\n");
                return -EINVAL;
            }
            curr_x += x_steps;
            curr_y += y_steps;
        }

        if (capture_bool && board[to_y][to_x][0] != '*' && board[to_y][to_x][0] != color) {
            // print captured piece
            printk(KERN_INFO "Captured: %s\n", board[to_y][to_x]);
            // make move
            memcpy(board[to_y][to_x], board[from_y][from_x], 3);
            memset(board[from_y][from_x], '*', 2);
            board[from_y][from_x][2] = '\0';
            return 0;
        } else if (!capture_bool && board[to_y][to_x][0] == '*') {
            memcpy(board[to_y][to_x], board[from_y][from_x], 3);
            memset(board[from_y][from_x], '*', 2);
            board[from_y][from_x][2] = '\0';
            return 0;
        } else {
            printk(KERN_INFO "Attempting to make incorrect capture with bishop or location is friendly piece\n");
            return -EINVAL;
        }
    } else {
        // ill move
        printk(KERN_INFO "Did not move diagonally, ill move\n");
        return -EINVAL;
    }
}

int move_rook(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y, int capture_bool) {
    // check to see if the move is either horizontal or vertical
    if (from_x == to_x || from_y == to_y) {
        // must check if the path is clear or obstructed
        int x_steps = 0;
        int y_steps = 0;
        // check to see if moving horizontally
        if (from_x != to_x) {
            if (to_x > from_x) {
                // moving in the right direction
                x_steps = 1;
            } else {
                // moving left
                x_steps = -1;
            }
        }
        // check to see if moving vertically
        if (from_y != to_y) {
            if (to_y > from_y) {
                // moving up
                y_steps = 1;
            } else {
                // moving down
                y_steps = -1;
            }
        }

        // check path of rook in front of it
        int curr_x = from_x + x_steps;
        int curr_y = from_y + y_steps;

        // iterate up until the pos right before the destination
        while ((x_steps != 0 && curr_x != to_x) || (y_steps != 0 && curr_y != to_y)) {
            if (board[curr_y][curr_x][0] != '*') {
                printk(KERN_INFO "Path is blocked when moving the rook\n");
                return -EINVAL;
            }
            curr_x += x_steps;
            curr_y += y_steps;
        }

        // check destination space
        // check if capture move and check if last move is not same color and is not free
        if (capture_bool && board[to_y][to_x][0] != '*' && board[to_y][to_x][0] != color) {
            // print captured piece
            printk(KERN_INFO "Captured: %s\n", board[to_y][to_x]);
            // make move
            memcpy(board[to_y][to_x], board[from_y][from_x], 3);
            memset(board[from_y][from_x], '*', 2);
            board[from_y][from_x][2] = '\0';
            return 0;
        }
        // make regular move if pos is free and filled with the same color
        else if (!capture_bool && board[to_y][to_x][0] == '*') {
            memcpy(board[to_y][to_x], board[from_y][from_x], 3);
            memset(board[from_y][from_x], '*', 2);
            board[from_y][from_x][2] = '\0';
            return 0;
        } else {
            printk(KERN_INFO "Attempted an incorrect capture or wrong rook move\n");
            return -EINVAL;
        }
    } else {
        // ill move
        printk(KERN_INFO "Rook can only move horizontal or vertical\n");
        return -EINVAL;
    }
}

int move_queen(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y, int capture_bool) {
    // get move distance in x and y direction
    int distance_x = abs(to_x - from_x);
    int distance_y = abs(from_y - to_y);
    int x_steps = 0;
    int y_steps = 0;

    // move can be diagonal, horizontal or vertical as long its path is clear
    if (distance_x == distance_y || from_x == to_x || from_y == to_y) {
        if (to_x > from_x) {
            // moving right
            x_steps = 1;
        } else if (to_x < from_x) {
            // moving left
            x_steps = -1;
        }
        if (to_y > from_y) {
            // moving up
            y_steps = 1;
        } else if (to_y < from_y) {
            // moving down
            y_steps = -1;
        }

        // check path of queen in front of it
        int curr_x = from_x + x_steps;
        int curr_y = from_y + y_steps;

        // iterate up until the position right before the destination
        while (curr_x != to_x || curr_y != to_y) {
            if (board[curr_y][curr_x][0] != '*') {
                printk(KERN_INFO "Path is blocked when moving the queen\n");
                return -EINVAL;
            }
            curr_x += x_steps;
            curr_y += y_steps;
        }

        // check destination space
        // check if capture move and check if last move is not same color and is not free
        if (capture_bool && board[to_y][to_x][0] != '*' && board[to_y][to_x][0] != color) {
            // print captured piece
            printk(KERN_INFO "Captured: %s\n", board[to_y][to_x]);
            // make move
            memcpy(board[to_y][to_x], board[from_y][from_x], 3);
            memset(board[from_y][from_x], '*', 2);
            board[from_y][from_x][2] = '\0';
            return 0;
        }
        // make regular move if pos is free and filled with the same color
        else if (!capture_bool && board[to_y][to_x][0] == '*') {
            memcpy(board[to_y][to_x], board[from_y][from_x], 3);
            memset(board[from_y][from_x], '*', 2);
            board[from_y][from_x][2] = '\0';
            return 0;
        } else {
            printk(KERN_INFO "Attempted an incorrect capture move for the queen\n");
            return -EINVAL;
        }
    } else {
        // ill move
        printk(KERN_INFO "Made incorrect move for the queen\n");
        return -EINVAL;
    }
}
int move_king(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y, int capture_bool) {
    int distance_x = abs(to_x - from_x);
    int distance_y = abs(from_y - to_y);

    // checking the distance is at most one space. can only be on space up, one space left or right or one space left or right
    if (distance_x <= 1 && distance_y <= 1) {
        if (board[to_y][to_x][0] != color && capture_bool && board[to_y][to_x][0] != '*') {
            // print captured piece
            printk(KERN_INFO "Captured: %s\n", board[to_y][to_x]);
            // make move
            memcpy(board[to_y][to_x], board[from_y][from_x], 3);
            memset(board[from_y][from_x], '*', 2);
            board[from_y][from_x][2] = '\0';
            return 0;
        } else if (!capture_bool && board[to_y][to_x][0] == '*') {
            memcpy(board[to_y][to_x], board[from_y][from_x], 3);
            memset(board[from_y][from_x], '*', 2);
            board[from_y][from_x][2] = '\0';
            return 0;
        } else {
            // attempted to move to a piece that is blocked
            // ill move
            printk(KERN_INFO "Attempted to move to a block piece or try to capture incorrectly with the king\n");
            return -EINVAL;
        }
    } else {
        // ill move, incorrect move action
        printk(KERN_INFO "King can only move one space at a time\n");
        return -EINVAL;
    }
}

static inline char upperCase(char c){
	// convert to uppercase 
	if( c >= 'a' && c <= 'z'){
		return c - 'a' + 'A';
	}
	return c;
}

int handle_command(char board[BOARD_SIZE][BOARD_SIZE][3], const char *command) {
    char color, piece, from_col, to_col, promotion = '\0', promotion_piece = '\0';
    char captured_piece_type = '\0', capture = '\0';
    int from_row, to_row;

    // buffer to hold command for manipulation
    char buf[32];
    strncpy(buf, command, sizeof(buf) - 1);
    // make null term
    buf[sizeof(buf) - 1] = '\0'; 

    int offset = 0;
    // go through the command 
    color = buf[offset++];
    piece = buf[offset++];
    from_col = buf[offset++];
    from_row = buf[offset++] - '0';
    if (buf[offset] == '-') offset++; // skip dash
    to_col = buf[offset++];
    to_row = buf[offset++] - '0';

    // check for additional actions for captures or promotions
    while (buf[offset]) {
        if (buf[offset] == 'x') { 
            capture = 'x';
            captured_piece_type = buf[++offset];
        } else if (buf[offset] == 'y') { 
            promotion = 'y';
            promotion_piece = buf[++offset];
        }
        offset++;
    }

    // convert column label to 0 based array index (a = 0, b = 1)
    int from_x = from_col - 'a';
    int to_x = to_col - 'a';

    // convert row numbers (1-8) to array indices (0-7), where 1 maps to 7, 2 to 6
    int from_y = from_row - 1; 
    int to_y = to_row - 1;

    int capture_bool = (capture == 'x');
    int promotion_bool = (promotion == 'y' && (promotion_piece == 'Q' || promotion_piece == 'R' ||
                                               promotion_piece == 'B' || promotion_piece == 'N'));

    printk(KERN_INFO "Parsed move: %c%c from %c%d to %c%d\n", color, piece, from_col, from_row, to_col, to_row);
    if (capture_bool) {
        printk(KERN_INFO "Capture indicated with piece type %c\n", captured_piece_type);
    }
    if (promotion_bool) {
        printk(KERN_INFO "Promotion indicated to %c\n", promotion_piece);
    }

    char upper_piece = upperCase(piece); // convert piece character to upper case

    // check which piece type and perform corresponding move
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
        printk(KERN_INFO "Invalid piece type.\n");
        return -EINVAL; 
    }

    return -EINVAL; 
}

// checks the path made for cpu 
int is_path_clear(char board[BOARD_SIZE][BOARD_SIZE][3], int from_x, int from_y, int to_x, int to_y) {
    if (from_x == to_x && from_y == to_y) {
        return 1; // if the start and end are the same, no movement, thus path is 'clear'
    }

    int x_step = 0;
    int y_step = 0;

    if (to_x > from_x) {
        x_step = 1;
    } else if (to_x < from_x) {
        x_step = -1;
    } else {
        x_step = 0;
    }

    if (to_y > from_y) {
        y_step = 1;
    } else if (to_y < from_y) {
        y_step = -1;
    } else {
        y_step = 0;
    }

    int x = from_x + x_step;
    int y = from_y + y_step;

    while (x != to_x || y != to_y) {
        if (board[y][x][0] != '*') {  // assuming '*' indicates an empty square
            printk(KERN_INFO "path blocked at (%d, %d)\n", x, y);
            return 0; // path is blocked
        }
        x += x_step;
        y += y_step;
    }

    return 1; // path is clear
}

// helper for cpu
int is_valid_move(char board[BOARD_SIZE][BOARD_SIZE][3], char color, int from_x, int from_y, int to_x, int to_y) {
    // check if the piece at the starting position belongs to the player
    if (board[from_y][from_x][0] != color) {
        printk(KERN_INFO "invalid move: not player's piece.\n");
        return 0;
    }
	printk("Entering is valid move");
    char piece = board[from_y][from_x][1]; 
    // get distance
    int dx = to_x - from_x; 
    int dy = to_y - from_y; 
    
    int abs_dx = abs(dx); 
    int abs_dy = abs(dy); 

    // check different types of pieces and their valid moves
    if (piece == 'P') {
        int forward;
	if (color == 'W') {
		forward = 1;
	    } else {
		forward = -1;
	    }
        if (dx == 0) {
            if (dy == forward && board[to_y][to_x][0] == '*') {
                return 1; // pawn can move one square forward if destination is empty
            } // Check if the pawn can move two squares forward from the starting position if both squares are empty
else if (dy == 2 * forward && (from_y == 1 || from_y == 6)) {
    if (board[to_y][to_x][0] == '*' && board[from_y + forward][from_x][0] == '*') {
        return 1;
    }
}
        } else if (abs_dx == 1 && dy == forward && board[to_y][to_x][0] != '*' && board[to_y][to_x][0] != color) {
            return 1; // pawn can capture diagonally
        }
    } else if (piece == 'N' && ((abs_dx == 2 && abs_dy == 1) || (abs_dx == 1 && abs_dy == 2))) {
        return 1; // knight can move in an L-shape
    } else if (piece == 'B' && abs_dx == abs_dy) {
        return is_path_clear(board, from_x, from_y, to_x, to_y); // bishop can move diagonally
    } else if (piece == 'R' && (dx == 0 || dy == 0)) {
        return is_path_clear(board, from_x, from_y, to_x, to_y); // rook can move horizontally or vertically
    } else if (piece == 'Q' && (abs_dx == abs_dy || dx == 0 || dy == 0)) {
        return is_path_clear(board, from_x, from_y, to_x, to_y); // queen can move diagonally, horizontally, or vertically
    } else if (piece == 'K' && abs_dx <= 1 && abs_dy <= 1) {
        return 1; // king can move one square in any direction
    } else {
        printk(KERN_INFO "invalid move attempted by %c from (%d, %d) to (%d, %d)\n", piece, from_x, from_y, to_x, to_y);
        return 0; // invalid move for any other cases
    }

    return 0; // default return
}

//helper for cpu
void collect_valid_moves(char board[BOARD_SIZE][BOARD_SIZE][3], char color, MoveList *move_list) {
    // reset the list of moves
    move_list->count = 0;
    int y,dy,dx,x = 0;
	printk("Entering collect valid move");
    // iterate over the board to find the player's pieces
    for ( y = 0; y < BOARD_SIZE; y++) {
        for ( x = 0; x < BOARD_SIZE; x++) {

            if (board[y][x][0] == color) {
              
                for ( dy = 0; dy < BOARD_SIZE; dy++) {
                    for ( dx = 0; dx < BOARD_SIZE; dx++) {
                        // check if the move drom dx dy is valid
                        if (is_valid_move(board, color, x, y, dx, dy)) {
                            // if the move is valid, add it to the move list
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

    
    collect_valid_moves(board, color, &move_list);
	printk("Entering is execute move");
    // Check if there are any valid moves available
    if (move_list.count > 0) {
        unsigned int index;
        // generate a random index within the range of valid moves
        get_random_bytes(&index, sizeof(index));
        index %= move_list.count; // checkthe index is within the range of valid moves

    
        Move selected_move = move_list.moves[index];
        // update move
        board[selected_move.to_y][selected_move.to_x][0] = board[selected_move.from_y][selected_move.from_x][0];
        board[selected_move.to_y][selected_move.to_x][1] = board[selected_move.from_y][selected_move.from_x][1];
        board[selected_move.from_y][selected_move.from_x][0] = '*'; // empty pos
        board[selected_move.from_y][selected_move.from_x][1] = '*';

        // print the move
        printk(KERN_INFO "CPU moved from (%d, %d) to (%d, %d)\n",
               selected_move.from_x, selected_move.from_y,
               selected_move.to_x, selected_move.to_y);
    } else {
        // no valid moves are available, print a message to the kernel log
        printk(KERN_INFO "No valid moves available for CPU.\n");
    }
}
int process_command(ChessGame *game, const char *command) {

    // check if it's a new game command
    if (strncmp(command, "00 ", 3) == 0) {
        if (command[3] == 'W' || command[3] == 'B') {
            initialize_game(game, command[3]);
            return 0;
        }
        printk(KERN_INFO "malformed command\n");
        return -EINVAL; // malformed command
    } else if (strncmp(command, "01\n",3) == 0) {
        // check if there's a game running to display its state
        if (game->state != STATE_RUNNING) {
            printk(KERN_INFO "NOGAME\n");
        } else {
            print_board(game->board);
        }
        return 0;
    } else if (strncmp(command, "02 ", 3) == 0) {
        // check game state before making a move
        if (game->state != STATE_RUNNING) {
            printk(KERN_INFO "NOGAME\n");
            return -EINVAL;
        }
        // process a move command
        if (handle_command(game->board, command + 3) == 0) {
            print_board(game->board);
        } else {
            printk(KERN_INFO "ILLMOVE\n");
        }
        return 0;
    } else if (strcmp(command, "03\n") == 0) {
    printk(KERN_INFO"Entering number 3");
        if (game->state != STATE_RUNNING) {
            printk(KERN_INFO "NOGAME\n");
            return -EINVAL;
        }
        else{
	if(game->current_player == 'W'){
		execute_cpu_move(game->board,'B');
	}
	else{
		execute_cpu_move(game->board,'W');
	}
        }
        return 0;
    } 
    else if (strncmp(command, "04\n",3) == 0) {
        // handle game resignation
        if (game->state != STATE_RUNNING) {
            printk(KERN_INFO "NOGAME\n");
            return -EINVAL;
        }
        game->state = STATE_NO_GAME;
        printk(KERN_INFO "Game resigned. CPU wins.\n");
        return 0;
    }
    printk(KERN_INFO "unknown command\n");
    return -EINVAL; 
}

static int __init chess_init(void) {
    printk(KERN_INFO "Chess: Initializing the Chess \n");
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        printk(KERN_ALERT "Chess failed to register a major number\n");
        return majorNumber;
    }

    chessClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(chessClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        return PTR_ERR(chessClass);
    }

    chessDevice = device_create(chessClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(chessDevice)) {
        class_destroy(chessClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        return PTR_ERR(chessDevice);
    }

    cdev_init(&c_dev, &fops);
    cdev_add(&c_dev, MKDEV(majorNumber, 0), 1);
    printk(KERN_INFO "Chess Device class created correctly\n");
    return 0;
}

static void __exit chess_exit(void) {
    cdev_del(&c_dev);
    device_destroy(chessClass, MKDEV(majorNumber, 0));
    class_destroy(chessClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "Chess end!\n");
}

static int dev_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "Chess Device has been opened\n");
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "Chess Device successfully closed\n");
    return 0;
}


static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    static const char *msg = "Current game state\n";
    int msg_size = strlen(msg);

    if (*offset >= msg_size) return 0; // EOF

    if (len > msg_size - *offset) len = msg_size - *offset;

    if (copy_to_user(buffer, msg + *offset, len)) return -EFAULT;

    *offset += len;
    return len; // Return the number of bytes read
}

static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
	// allocate mem and intiialize it to 0
    char *kbuf = kzalloc(len+1, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    if (copy_from_user(kbuf, buffer, len)) {
        kfree(kbuf);
        return -EFAULT;
    }
// null the string
    kbuf[len] = '\0'; 
    printk(KERN_INFO "Chess: Received command - %s\n", kbuf);
   
    int ret = process_command(&game, kbuf);
    kfree(kbuf);
    if (ret != 0) {
        return ret;  //the error or specific non-zero return value
    } else {
        return len;  //the number of bytes processed if no errors
    }
}
module_init(chess_init);
module_exit(chess_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Waleed");
MODULE_DESCRIPTION("Chess Game");

