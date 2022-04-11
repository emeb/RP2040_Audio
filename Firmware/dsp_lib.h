/*
 * dsp_lib.h - miscellaneous DSP stuff for RP2040 Audio
 * 03-27-22 E. Brombaugh
 */

#ifndef __dsp_lib__
#define __dsp_lib__

#include "main.h"

uint8_t dsp_gethyst(int16_t *oldval, int16_t newval);
uint8_t dsp_ratio_hyst_arb(uint16_t *old, uint16_t in, uint8_t range);


/*
 * signed saturation to 16-bit
 */
/* as regular code */
inline int16_t dsp_ssat16(int32_t in)
{
	in = in > 32767 ? 32767 : in;
	in = in < -32768 ? -32768 : in;
	return in;
}

#endif

