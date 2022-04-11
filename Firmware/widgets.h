/*
 * widgets.c - UI graphics widgets for ESP32S2 Audio
 * 03-20-22 E. Brombaugh
 */

#ifndef __widgets__
#define __widgets__

void widg_bargraphH(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t v);
uint8_t widg_gradient_init(int16_t width);
void widg_bargraphHG(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t v);
void widg_sliderH(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t v);

#endif