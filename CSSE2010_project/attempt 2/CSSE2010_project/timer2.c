/*
 * timer2.c
 * Step counter
 * Author: Peter Sutton
 */

#include "timer2.h"
#include <avr/io.h>
#include <avr/interrupt.h>

/* Seven segment display segment values for 0 to 9 */
static const uint8_t seven_seg_data[10] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111};
uint8_t digit0 = 0; // ranges from 0 to 9
uint8_t digit1 = 0;// ranges from 0 to 9
volatile uint8_t seven_seg_cc = 0;
void init_timer2(void)
{
	// Setup timer 2.
	TCNT2 = 0;
	DDRC = 0xFF;
	OCR2A = 49999;
	TCCR2A = (1<<WGM21); /* CTC */
	TCCR2B = (1<<CS21); /* Divide clock by 8 */
	TIMSK2 = (1<<OCIE2A);

	/* Ensure interrupt flag is cleared */
	TIFR2 = (1<<OCF2A);

	/* Turn on global interrupts */
}

void increment_digit_SSD(void) {
	if (digit0 >= 9) {
		digit0 = 0;
		if(digit1 >= 9) {
			digit1 = 0;
		} else {
			digit1++;
		}		
	} else {
		digit0++;
	}
}

ISR(TIMER2_COMPA_vect) {
	seven_seg_cc = 1 ^ seven_seg_cc;
	/* Display a digit */
	if(seven_seg_cc == 0) {
		/* Display rightmost digit - tenths of seconds */
		PORTC = seven_seg_data[digit0];
	} else {
		/* Display leftmost digit - seconds */
		PORTC = seven_seg_data[digit1];
	}
	/* Output the digit selection (CC) bit */
	PORTC |= (seven_seg_cc << 7);
}
