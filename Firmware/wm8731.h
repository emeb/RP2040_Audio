/*
 * wm8731.h - WM8731 codec I2C control port driver for RP2040
 * 10-28-21 E. Brombaugh
 */

#ifndef __wm8731__
#define __wm8731__

int32_t WM8731_Init(void);
int32_t WM8731_Reset(void);
void WM8731_Mute(uint8_t enable);
void WM8731_HPVol(uint8_t vol);
void WM8731_InSrc(uint8_t src);
void WM8731_InVol(uint8_t vol);
void WM8731_MicBoost(uint8_t boost);

#endif
