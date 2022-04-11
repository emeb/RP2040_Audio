/*
 * ST7735.h - interface routines for ST7735 LCD.
 * shamelessly ganked from Adafruit_ST7735 library
 * 08-28-20 E. Brombaugh
 * 10-21-20 E. Brombaugh - updated for f405_codec_v2
 * 10-28-20 E. Brombaugh - updated for f405 feather + tftwing
 * 10-11-21 E. Brombaugh - updated for RP2040
 */

#ifndef __st7735__
#define __st7735__

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "gfx.h"

// dimensions for LCD on tiny TFT wing
#define ST7735_TFTWIDTH 80
#define ST7735_TFTHEIGHT 160

extern GFX_DRIVER ST7735_drvr;

void ST7735_init(void);
void ST7735_setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void ST7735_drawPixel(int16_t x, int16_t y, uint16_t color);
void ST7735_fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
	uint16_t color);
uint16_t ST7735_Color565(uint32_t rgb24);
uint32_t ST7735_ColorRGB(uint16_t color16);
void ST7735_bitblt(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *buf);
void ST7735_setRotation(uint8_t m);
void ST7735_setBacklight(uint8_t ena);

#endif
