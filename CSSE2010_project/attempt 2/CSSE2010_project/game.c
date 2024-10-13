/*
 * game.c
 *
 * Authors: Jarrod Bennett, Cody Burnett, Bradley Stone, Yufeng Gao
 * Modified by: Sithika Mannakkara
 *
 * Game logic and state handler.
 */ 

#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "ledmatrix.h"
#include "terminalio.h"


// ========================== NOTE ABOUT MODULARITY ==========================

// The functions and global variables defined with the static keyword can
// only be accessed by this source file. If you wish to access them in
// another C file, you can remove the static keyword, and define them with
// the extern keyword in the other C file (or a header file included by the
// other C file). While not assessed, it is suggested that you develop the
// project with modularity in mind. Exposing internal variables and functions
// to other .C files reduces modularity.


// ============================ GLOBAL VARIABLES =============================

// The game board, which is dynamically constructed by initialise_game() and
// updated throughout the game. The 0th element of this array represents the
// bottom row, and the 7th element of this array represents the top row.
static uint8_t board[MATRIX_NUM_ROWS][MATRIX_NUM_COLUMNS];

// The location of the player.
static uint8_t player_row;
static uint8_t player_col;

static uint8_t num_boxes_in_target;

// A flag for keeping track of whether the player is currently visible.
static bool player_visible;

static const uint8_t TERMINAL_GAME_ROW = 12;
static const uint8_t TERMINAL_GAME_COL = 15;
static const uint8_t TERMINAL_E_ROW = 5;
static const uint8_t TERMINAL_E_COL = 5;

static uint8_t coordinate_history[6][2];
static uint8_t hist_idx;



// ========================== GAME LOGIC FUNCTIONS ===========================

// This function paints a square based on the object(s) currently on it.
static void paint_square(uint8_t row, uint8_t col)
{
	switch (board[row][col] & OBJECT_MASK)
	{
		case ROOM:
			ledmatrix_update_pixel(row, col, COLOUR_BLACK);
			break;
		case WALL:
			ledmatrix_update_pixel(row, col, COLOUR_WALL);
			break;
		case BOX:
			ledmatrix_update_pixel(row, col, COLOUR_BOX);
			break;
		case TARGET:
			ledmatrix_update_pixel(row, col, COLOUR_TARGET);
			break;
		case BOX | TARGET:
			ledmatrix_update_pixel(row, col, COLOUR_DONE);
			break;
		default:
			break;
	}
}

// This function initialises the global variables used to store the game
// state, and renders the initial game display.
void initialise_game(void)
{
	hist_idx = 1;
	coordinate_history[0][0] = 5;
	coordinate_history[0][1] = 2;
	coordinate_history[1][0] = 0xFF;
	coordinate_history[1][1] = 0xFF;
	coordinate_history[2][0] = 0xFF;
	coordinate_history[2][1] = 0xFF;
	coordinate_history[3][0] = 0xFF;
	coordinate_history[3][1] = 0xFF;
	coordinate_history[4][0] = 0xFF;
	coordinate_history[4][1] = 0xFF;
	coordinate_history[5][0] = 0xFF;
	coordinate_history[5][1] = 0xFF;
	num_boxes_in_target = 0;
	// Short definitions of game objects used temporarily for constructing
	// an easier-to-visualise game layout.
	#define _	(ROOM)
	#define W	(WALL)
	#define T	(TARGET)
	#define B	(BOX)
	

	// The starting layout of level 1. In this array, the top row is the
	// 0th row, and the bottom row is the 7th row. This makes it visually
	// identical to how the pixels are oriented on the LED matrix, however
	// the LED matrix treats row 0 as the bottom row and row 7 as the top
	// row.
	static const uint8_t lv1_layout[MATRIX_NUM_ROWS][MATRIX_NUM_COLUMNS] =
	{
		{ _, W, _, W, W, W, _, W, W, W, _, _, W, W, W, W },
		{ _, W, T, W, _, _, W, T, _, B, _, _, _, _, T, W },
		{ _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _ },
		{ W, _, B, _, _, _, _, W, _, _, B, _, _, B, _, W },
		{ W, _, _, _, W, _, B, _, _, _, _, _, _, _, _, _ },
		{ _, _, _, _, _, _, T, _, _, _, _, _, _, _, _, _ },
		{ _, _, _, W, W, W, W, W, W, T, _, _, _, _, _, W },
		{ W, W, _, _, _, _, _, _, W, W, _, _, W, W, W, W }
	};

	// Undefine the short game object names defined above, so that you
	// cannot use use them in your own code. Use of single-letter names/
	// constants is never a good idea.
	#undef _
	#undef W
	#undef T
	#undef B

	// Set the initial player location (for level 1).
	player_row = 5;
	player_col = 2;

	// Make the player icon initially invisible.
	player_visible = false;

	// Copy the starting layout (level 1 map) to the board array, and flip
	// all the rows.
	for (uint8_t row = 0; row < MATRIX_NUM_ROWS; row++)
	{
		for (uint8_t col = 0; col < MATRIX_NUM_COLUMNS; col++)
		{
			board[MATRIX_NUM_ROWS - 1 - row][col] =
				lv1_layout[row][col];
		}
	}

	// Draw the game board (map).
	for (uint8_t row = 0; row < MATRIX_NUM_ROWS; row++)
	{
		for (uint8_t col = 0; col < MATRIX_NUM_COLUMNS; col++)
		{
			paint_square(row, col);
		}
	}
}

void add_to_history(uint8_t row, uint8_t col) {
	coordinate_history[hist_idx][0] = row;
	coordinate_history[hist_idx][1] = col;
	if (hist_idx == 5) {
		hist_idx = 0;
		} else {
		hist_idx++;
	}
}

uint8_t get_previous_row() {
	if (hist_idx == 5) {
		return coordinate_history[0][0];
	} else if (hist_idx == 0) {
		return coordinate_history[5][0];
	} else {
		return coordinate_history[hist_idx - 1][0];
	}
}

uint8_t get_previous_col() {
	if (hist_idx == 5) {
		return coordinate_history[0][1];
		} else if (hist_idx == 0) {
		return coordinate_history[5][1];
		} else {
		return coordinate_history[hist_idx - 1][1];
	}
}

// This function flashes the player icon. If the icon is currently visible, it
// is set to not visible and removed from the display. If the player icon is
// currently not visible, it is set to visible and rendered on the display.
// The static global variable "player_visible" indicates whether the player
// icon is currently visible.
void flash_player(void)
{
	player_visible = !player_visible;
	if (player_visible)
	{
		// The player is visible, paint it with COLOUR_PLAYER.
		ledmatrix_update_pixel(player_row, player_col, COLOUR_PLAYER);
	}
	else
	{
		// The player is not visible, paint the underlying square.
		paint_square(player_row, player_col);
	}
}

// This function handles player movements.
// @requires -1 <= delta_row <= 1
// @requires -1 <= delta_col <= 1
bool move_player(int8_t delta_row, int8_t delta_col)
{
	//                    Implementation Suggestions
	//                    ==========================
	//
	//    Below are some suggestions for how to implement the first few
	//    features. These are only suggestions, you are absolutely not
	//   required to follow them if you know what you're doing, they're
	//     just here to help you get started. The suggestions for the
	//       earlier features are more detailed than the later ones.
	//
	// +-----------------------------------------------------------------+
    // |            Move Player with Push Buttons/Terminal               |
	// +-----------------------------------------------------------------+
	// | 1. Remove the display of the player icon from the current       |
	// |    location.                                                    |
	// |      - You may find the function flash_player() useful.         |
	// | 2. Calculate the new location of the player.                    |
	// |      - You may find creating a function for this useful.        |
	// | 3. Update the player location (player_row and player_col).      |
	// | 4. Draw the player icon at the new player location.             |
	// |      - Once again, you may find the function flash_player()     |
	// |        useful.                                                  |
	// | 5. Reset the icon flash cycle in the caller function (i.e.,     |
	// |    play_game()).                                                |
	// +-----------------------------------------------------------------+
	//
	// +-----------------------------------------------------------------+
	// |                      Game Logic - Walls                         |
	// +-----------------------------------------------------------------+
	// | 1. Modify this function to return a flag/boolean for indicating |
	// |    move validity - you do not want to reset icon flash cycle on |
	// |    invalid moves.                                               |
	// | 2. Modify this function to check if there is a wall at the      |
	// |    target location.                                             |
	// | 3. If the target location contains a wall, print one of your 3  |
	// |    'hit wall' messages and return a value indicating an invalid |
	// |    move.                                                        |
	// | 4. Otherwise make the move, clear the message area of the       |
	// |    terminal and return a value indicating a valid move.         |
	// +-----------------------------------------------------------------+
	//
	// +-----------------------------------------------------------------+
	// |                      Game Logic - Boxes                         |
	// +-----------------------------------------------------------------+
	// | 1. Modify this function to check if there is a box at the       |
	// |    target location.                                             |
	// | 2. If the target location contains a box, see if it can be      |
	// |    pushed. If not, print a message and return a value           |
	// |    indicating an invalid move.                                  |
	// | 3. Otherwise push the box and move the player, then clear the   |
	// |    message area of the terminal and return a valid indicating a |
	// |    valid move.                                                  |
	// +-----------------------------------------------------------------+
	uint8_t previous_row = get_previous_row();;
	uint8_t previous_col = get_previous_col();
 
	uint8_t infront_next_row = player_row;
	uint8_t infront_next_col = player_col;
	
	uint8_t next_row = player_row;
	uint8_t next_col = player_col;
	// if there is a wall on the next positon the player must not move to next position.
	// if there is a box on the next position then the player and the box must move together.
	// if there is a wall infront of the box, then the player and the box must not move together.
	
	if (delta_row) {
		next_row += delta_row;
		if (next_row > 200) { // player reaches row 0 and goes down
			next_row = 7;
			infront_next_row = 6;
		} else if (next_row > 7) { // player reaches row 7 and goes up
			next_row = 0;
			infront_next_row = 1; 
		} else if (next_row == 0) {
			infront_next_row = 7;	
		} else if (next_row == 7) {
			infront_next_row = 0;
		} else {
			infront_next_row = next_row + delta_row;
		}	
	}
	
	if (delta_col) {
		next_col += delta_col;
		if (next_col > 200) { // player reaches column 0 and goes left
			next_col = 15;
			infront_next_col = 14;
		} else if (next_col > 15) { // player reaches column 15 and goes right
			next_col = 0;
			infront_next_col = 1;
		} else if (next_col == 0) {
			infront_next_col = 15;
		} else if (next_col == 15) {
			infront_next_col = 0;
		} else {
			infront_next_col = next_col + delta_col;	
		}	
	}
	// If the next row or column is a wall the move is invalid
	if (board[next_row][next_col] == WALL) {
		move_terminal_cursor(TERMINAL_E_ROW, TERMINAL_E_COL);
		clear_to_end_of_line();
		int random_num = rand() % 3;
		if (random_num == 0) {
			printf_P(PSTR("The player hit a wall!"));
		} else if (random_num == 1) {
			printf_P(PSTR("Player can't move through walls."));
		} else {
			printf_P(PSTR("The wall is obstructing you."));
		}
		flash_player();
		return false; // don't move

	// If the next row/column is a box or box target.
	// Player cant move box through walls or boxes.
	// There is a box or a box in a target in front of the player.
	// Player can move box out of target or move box into target.
	} else if (board[next_row][next_col] == BOX || board[next_row][next_col] == (BOX | TARGET)) {
		if (board[infront_next_row][infront_next_col] == WALL) {
			move_terminal_cursor(TERMINAL_E_ROW, TERMINAL_E_COL);
			printf_P(PSTR("You can't push a box through a wall!"));
			return false; // don't move
		} else if (board[infront_next_row][infront_next_col] == BOX || board[infront_next_row][infront_next_col] == (BOX | TARGET)) {
			move_terminal_cursor(TERMINAL_E_ROW, TERMINAL_E_COL);
			printf_P(PSTR("You can't push two boxes at once!"));
			return false; // don't move
		} else {
			// player and box move
			if (board[infront_next_row][infront_next_col] == TARGET) {
				board[next_row][next_col] = ROOM;
				board[infront_next_row][infront_next_col] = (BOX | TARGET);
				set_complete_terminal(infront_next_row, infront_next_col);
				move_terminal_cursor(TERMINAL_E_ROW, TERMINAL_E_COL);
				printf_P(PSTR("Box was moved to target."));	
				num_boxes_in_target++;
			} else if (board[next_row][next_col] == (BOX | TARGET)) {
				board[next_row][next_col] = TARGET;
				board[infront_next_row][infront_next_col] = BOX;	
				move_box_terminal(infront_next_row, infront_next_col);
				num_boxes_in_target--;
			} else {
				board[next_row][next_col] = ROOM;
				board[infront_next_row][infront_next_col] = BOX;
				move_box_terminal(infront_next_row, infront_next_col);
			}
			paint_square(next_row, next_col);
			paint_square(infront_next_row, infront_next_col);
		}
	}
	
	// Move the player
	if (board[infront_next_row][infront_next_col] != (BOX | TARGET)) {
		move_terminal_cursor(TERMINAL_E_ROW, TERMINAL_E_COL);
		clear_to_end_of_line();
	}
	paint_square(player_row, player_col);
	delete_old_terminal(player_row, player_col);
	if (board[player_row][player_col] == TARGET) {
		set_target_terminal(player_row, player_col);
	}
	player_row = next_row;
	player_col = next_col;
	move_player_terminal(player_row, player_col);
	flash_player();
	add_to_history(player_row, player_col);

	// testing
	move_terminal_cursor(0, 60);
	clear_to_end_of_line();
	printf_P(PSTR("row %d, col %d"), coordinate_history[0][0], coordinate_history[0][1]);
	move_terminal_cursor(1, 60);
	clear_to_end_of_line();
	printf_P(PSTR("row %d, col %d"), coordinate_history[1][0], coordinate_history[1][1]);
	move_terminal_cursor(2, 60);
	clear_to_end_of_line();
	printf_P(PSTR("row %d, col %d"), coordinate_history[2][0], coordinate_history[2][1]);
	move_terminal_cursor(3, 60);
	clear_to_end_of_line();
	printf_P(PSTR("row %d, col %d"), coordinate_history[3][0], coordinate_history[3][1]);
	move_terminal_cursor(4, 60);
	clear_to_end_of_line();
	printf_P(PSTR("row %d, col %d"), coordinate_history[4][0], coordinate_history[4][1]);
	move_terminal_cursor(5, 60);
	clear_to_end_of_line();
	printf_P(PSTR("row %d, col %d"), coordinate_history[5][0], coordinate_history[5][1]);
	move_terminal_cursor(6, 60);
	clear_to_end_of_line();
	printf_P(PSTR("pre row %d, pre col %d"), previous_row, previous_col);
	move_terminal_cursor(7, 60);
	clear_to_end_of_line();
	printf_P(PSTR("b in t: %d"), num_boxes_in_target);

	return true;	
}

void delete_old_terminal(uint8_t row, uint8_t col) {
	move_terminal_cursor(TERMINAL_GAME_ROW + (-row) + 7 , TERMINAL_GAME_COL + col);
	set_display_attribute(BG_BLACK);
	putchar(' ');
	set_display_attribute(TERM_RESET);
}

void move_player_terminal(uint8_t next_row, uint8_t next_col) {
	move_terminal_cursor(TERMINAL_GAME_ROW + (-next_row) + 7 , TERMINAL_GAME_COL + next_col);
	set_display_attribute(BG_WHITE);
	putchar(' ');
	set_display_attribute(TERM_RESET);
}

void move_box_terminal(uint8_t next_row, uint8_t next_col) {
	move_terminal_cursor(TERMINAL_GAME_ROW + (-next_row) + 7 , TERMINAL_GAME_COL + next_col);
	set_display_attribute(BG_CYAN);
	putchar(' ');
	set_display_attribute(TERM_RESET);
}

void set_target_terminal(uint8_t next_row, uint8_t next_col) {
	move_terminal_cursor(TERMINAL_GAME_ROW + (-next_row) + 7 , TERMINAL_GAME_COL + next_col);
	set_display_attribute(BG_RED);
	putchar(' ');
	set_display_attribute(TERM_RESET);
}

void set_complete_terminal(uint8_t next_row, uint8_t next_col) {
	move_terminal_cursor(TERMINAL_GAME_ROW + (-next_row) + 7 , TERMINAL_GAME_COL + next_col);
	set_display_attribute(BG_GREEN);
	putchar(' ');
	set_display_attribute(TERM_RESET);
}

void display_board_terminal() {
	normal_display_mode();

	// show player	
	for (uint8_t row = 0; row < MATRIX_NUM_ROWS; row++)
	{
		for (uint8_t col = 0; col < MATRIX_NUM_COLUMNS; col++)
		{
			move_terminal_cursor(TERMINAL_GAME_ROW + row, TERMINAL_GAME_COL + col);
			if (board[MATRIX_NUM_ROWS - 1 - row][col] == ROOM) {
				set_display_attribute(BG_BLACK); // room is black

			} else if (board[MATRIX_NUM_ROWS - 1 - row][col] == WALL) {
				set_display_attribute(BG_YELLOW); // wall is yellow

			} else if (board[MATRIX_NUM_ROWS - 1 - row][col] == TARGET) {
				set_display_attribute(BG_RED); // target is red

			} else if (board[MATRIX_NUM_ROWS - 1 - row][col] == BOX) {
				set_display_attribute(BG_CYAN); // box is cyan
			}
			putchar(' ');
		}
	}
	
	move_terminal_cursor(TERMINAL_GAME_ROW + (-player_row + 7) , TERMINAL_GAME_COL + player_col);
	set_display_attribute(BG_WHITE);
	putchar(' ');
	set_display_attribute(TERM_RESET);
}

// This function checks if the game is over (i.e., the level is solved), and
// returns true iff (if and only if) the game is over.
bool is_game_over(void)
{
	if (num_boxes_in_target == 5) {
		paint_square(player_row, player_col);
		return true;
	}
	return false;
	
}
