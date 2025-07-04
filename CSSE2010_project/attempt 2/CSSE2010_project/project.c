/*
 * project.c
 *
 * Authors: Peter Sutton, Luke Kamols, Jarrod Bennett, Cody Burnett,
 *          Bradley Stone, Yufeng Gao
 * Modified by: Sithika Mannakkara
 *
 * Main project event loop and entry point.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define F_CPU 8000000UL
#include <util/delay.h>

#include "game.h"
#include "startscrn.h"
#include "ledmatrix.h"
#include "buttons.h"
#include "serialio.h"
#include "terminalio.h"
#include "timer0.h"
#include "timer1.h"
#include "timer2.h"


// Function prototypes - these are defined below (after main()) in the order
// given here.
void initialise_hardware(void);
void start_screen(void);
void new_game(void);
void play_game(void);
void handle_game_over(void);

bool valid_move;
uint16_t start_time;
uint16_t num_valid_moves;
/////////////////////////////// main //////////////////////////////////
int main(void)
{
	// Setup hardware and callbacks. This will turn on interrupts.
	initialise_hardware();

	// Show the start screen. Returns when the player starts the game.
	start_screen();

	// Loop forever and continuously play the game.
	while (1)
	{
		new_game();
		play_game();
		handle_game_over();
	}
}

void initialise_hardware(void)
{
	init_ledmatrix();
	init_buttons();
	init_serial_stdio(19200, false);
	init_timer0();
	init_timer1();
	init_timer2();

	// Turn on global interrupts.
	sei();
}

void start_screen(void)
{
	// Hide terminal cursor and set display mode to default.
	hide_cursor();
	normal_display_mode();

	// Clear terminal screen and output the title ASCII art.
	clear_terminal();
	display_terminal_title(3, 5);
	move_terminal_cursor(11, 5);
	// Change this to your name and student number. Remember to remove the
	// chevrons - "<" and ">"!
	printf_P(PSTR("CSSE2010/7201 Project by Sithika Mannakkara - 48016722"));

	// Setup the start screen on the LED matrix.
	setup_start_screen();

	// Clear button presses registered as the result of powering on the
	// I/O board. This is just to work around a minor limitation of the
	// hardware, and is only done here to ensure that the start screen is
	// not skipped when you power cycle the I/O board.
	clear_button_presses();

	// Wait until a button is pushed, or 's'/'S' is entered.
	while (1)
	{
		// Check for button presses. If any button is pressed, exit
		// the start screen by breaking out of this infinite loop.
		if (button_pushed() != NO_BUTTON_PUSHED)
		{
			srand(get_current_time());
			break;
		}

		// No button was pressed, check if we have terminal inputs.
		if (serial_input_available())
		{
			// Terminal input is available, get the character.
			int serial_input = fgetc(stdin);

			// If the input is 's'/'S', exit the start screen by
			// breaking out of this loop.
			if (serial_input == 's' || serial_input == 'S')
			{
				srand(get_current_time());
				break;
			}
		}

		// No button presses and no 's'/'S' typed into the terminal,
		// we will loop back and do the checks again. We also update
		// the start screen animation on the LED matrix here.
		update_start_screen();
	}
}

void new_game(void)
{
	// Clear the serial terminal.
	hide_cursor();
	clear_terminal();

	// Initialise the game and display.
	initialise_game();
	valid_move = false;
	start_time = 0;
	num_valid_moves = 0;
	// Clear all button presses and serial inputs, so that potentially
	// buffered inputs aren't going to make it to the new game.
	clear_button_presses();
	clear_serial_input_buffer();
}

void play_game(void)
{
	display_board_terminal();
	reset_timer1();
	uint32_t last_flash_time = get_current_time();
	
	uint16_t current_time;
	// We play the game until it's over.
	while (!is_game_over())
	{
		current_time = get_current_time_sec();
		if (current_time - start_time >= 1) {
			start_time++;
			move_terminal_cursor(4, 5);
			printf_P(PSTR("Time elapsed : %d"), start_time);
		}
		
		// We need to check if any buttons have been pushed, this will
		// be NO_BUTTON_PUSHED if no button has been pushed. If button
		// 0 has been pushed, we get BUTTON0_PUSHED, and likewise, if
		// button 1 has been pushed, we get BUTTON1_PUSHED, and so on.
		ButtonState btn = button_pushed();
		
		// Move the player, see move_player(...) in game.c.
		// Also remember to reset the flash cycle here.
		if (btn == BUTTON0_PUSHED) {
			valid_move = move_player(0, 1);

		} else if (btn == BUTTON1_PUSHED) {
			valid_move = move_player(-1, 0);

		} else if (btn == BUTTON2_PUSHED) {
			valid_move = move_player(1, 0);

		} else if (btn == BUTTON3_PUSHED) {
		valid_move = move_player(0, -1);

		} else if (serial_input_available()) {
			int serial_input = fgetc(stdin);
			if (serial_input == 'd' || serial_input == 'D') valid_move = move_player(0, 1);
			else if (serial_input == 's' || serial_input == 'S') valid_move = move_player(-1, 0);
			else if (serial_input == 'w' || serial_input == 'W') valid_move = move_player(1, 0);
			else if (serial_input == 'a' || serial_input == 'A') valid_move = move_player(0, -1);
		}
		
		// for counting valid moves.
		if (valid_move) {
			increment_digit_SSD();
			num_valid_moves++;
			valid_move = false;
		}
		
		// Move box if player is on box

		uint32_t current_time = get_current_time();
		if (current_time >= last_flash_time + 200)
		{
			// 200ms (0.2 seconds) has passed since the last time
			// we flashed the player icon, flash it now.
			flash_player();

			// Update the most recent icon flash time.
			last_flash_time = current_time;
		}
	}
	// We get here if the game is over.
}
// Score = max(200 � S, 0) � 20 + max(1200 � T, 0)
uint16_t get_score(void) {
	uint16_t time_score = 0;
	uint16_t move_score = 0;
	if (start_time < 1200) {
		time_score = 1200 - start_time;
	}
	if (num_valid_moves < 200) {
		move_score = 200 - num_valid_moves;
	}
	return move_score + time_score;
}

void handle_game_over(void)
{
	clear_terminal();
	move_terminal_cursor(14, 10);
	printf_P(PSTR("GAME OVER"));
	move_terminal_cursor(15, 10);
	uint16_t score = get_score();
	printf_P(PSTR("Your score: %d"), score);
	move_terminal_cursor(16, 10);
	printf_P(PSTR("Steps taken: %d\tTime: %d"), num_valid_moves, start_time);
	move_terminal_cursor(17, 10);
	printf_P(PSTR("Press 'r'/'R' to restart, or 'e'/'E' to exit"));

	// Do nothing until a valid input is made.
	while (1)
	{
		// Get serial input. If no serial input is ready, serial_input
		// would be -1 (not a valid character).
		int serial_input = -1;
		if (serial_input_available())
		{
			serial_input = fgetc(stdin);
		}

		// Check serial input.
		if (toupper(serial_input) == 'R') {	
			new_game();
			play_game();
			handle_game_over();
		} else if (toupper(serial_input) == 'E') {
			main();
		}
		// Now check for other possible inputs.
		
	}
}
