/*
 * button.h - user input button for RP2040
 * 12-19-21 E. Brombaugh
 */

#ifndef __button__
#define __button__

#include "main.h"

void button_init(void);
uint8_t button_get(void);
uint8_t button_fe(void);
uint8_t button_re(void);

#endif
