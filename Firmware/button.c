/*
 * button.c - user input button for RP2040
 * 12-19-21 E. Brombaugh
 */

#include <stdio.h>
#include "debounce.h"
#include "button.h"

#define BUTTON_PIN 14

debounce_state btn_dbs;
uint8_t btn_fe, btn_re;
struct repeating_timer btn_rt;

/*
 * button scanning callback
 */
bool button_callback(repeating_timer_t *rt)
{
	debounce(&btn_dbs, (!gpio_get(BUTTON_PIN)));
	btn_fe |= btn_dbs.fe;
	btn_re |= btn_dbs.re;
    return true;
}

/*
 * init button scanning
 */
void button_init(void)
{
	/* button is input w/ pullup */
	gpio_init(BUTTON_PIN);
	gpio_set_dir(BUTTON_PIN, GPIO_IN);
	gpio_set_pulls(BUTTON_PIN, 1, 0);
	
	/* init debounce */
	init_debounce(&btn_dbs, 15);
	btn_fe = 0;
	btn_re = 0;
	
	/* start 1ms button timer */
	add_repeating_timer_ms(
		1,	// 1ms
		button_callback,
		NULL,
		&btn_rt);
}

/*
 * check status of button
 */
uint8_t button_get(void)
{
	return btn_dbs.state;
}

/*
 * check for falling edge of button
 */
uint8_t button_fe(void)
{
	uint8_t result = btn_fe;
	btn_fe = 0;
	return result;
}

/*
 * check for rising edge of button
 */
uint8_t button_re(void)
{
	uint8_t result = btn_re;
	btn_re = 0;
	return result;
}
