/*
 * nvs.h - non-volatile storage API for RP2040
 * 04-01-2022 E. Brombaugh
 */

#ifndef __NVS__
#define __NVS__

#include "main.h"

uint8_t nvs_init(void);
uint8_t nvs_get_tag(uint8_t tag, int16_t *value);
void nvs_put_tag(uint8_t tag, int16_t value);
uint8_t nvs_commit(void);

#endif