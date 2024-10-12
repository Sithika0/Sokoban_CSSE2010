/*
 * timer1.h
 *
 * Author: Peter Sutton
 *
 * Timer 1 skeleton.
 */

#ifndef TIMER1_H_
#define TIMER1_H_

#include <stdint.h>

/// <summary>
/// Skeletal timer 1 initialisation function.
/// </summary>
void init_timer1(void);
void reset_timer1(void);
uint16_t get_current_time_sec(void);

#endif /* TIMER1_H_ */
