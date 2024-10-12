/*
 * timer1.c
 *
 * Author: Peter Sutton
 * One second timer
 * 0011 1101 0000 1000 = 15,624
 * pre-scaler = 256
 * ctc mode
 */

#include "timer1.h"
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

static volatile uint16_t clock_ticks_sec;

void init_timer1(void)
{
	// Setup timer 1.
	TCNT1 = 0;
	OCR1A = 31249;
	TCCR1A = 0;
	TCCR1B = (1 << WGM12) | (1 << CS12);
	TIMSK1 |= (1 << OCIE1A);
	TIFR1 = (1 << OCF1A);
	
}

void reset_timer1(void) {
	clock_ticks_sec = 0;
}

uint16_t get_current_time_sec(void)
{
	// Disable interrupts so we can be sure that the interrupt doesn't
	// fire when we've copied just a couple of bytes of the value.
	// Interrupts are re-enabled if they were enabled at the start.
	uint8_t interrupts_were_enabled = bit_is_set(SREG, SREG_I);
	cli();
	uint16_t result = clock_ticks_sec;
	if (interrupts_were_enabled)
	{
		sei();
	}
	return result;
}

ISR(TIMER1_COMPA_vect)
{
	// Increment our clock tick count.
	clock_ticks_sec++;
}
