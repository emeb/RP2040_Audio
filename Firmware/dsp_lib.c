/*
 * dsp_lib.h - miscellaneous DSP stuff for ESP32S2 Audio
 * 03-13-22 E. Brombaugh
 */
#include <stdio.h>
#include <string.h>
#include "dsp_lib.h"

#define HYST_THRESH 16

/*
 * apply hysteresis and check for change
 */
uint8_t dsp_gethyst(int16_t *oldval, int16_t newval)
{
	uint8_t flag = 0;
	int16_t diff = newval-*oldval;
	
	/* abs */
	diff = diff < 0 ? -diff : diff;
	
	if((diff>HYST_THRESH) || (newval == 0) || (newval == 4095))
	{
		/* update hyst value and flag as new */
		if(*oldval != newval)
		{
			flag = 1;
			*oldval = newval;
		}
	}		

	return flag;
}

/*
 * quantize 12-bit CV into arbitrary ranges with hysteresis
 */
uint8_t dsp_ratio_hyst_arb(uint16_t *old, uint16_t in, uint8_t range)
{
	uint8_t result = 0;
	int16_t diff, guard, scale, rnd;
    
    /* compute scaling to achieve range */
    scale = 4096/range;
    rnd = scale/2;
    
    /* set guard band */
    guard = 2*scale/3;
    guard = (guard == 0) ? 1 : guard;
    
	/* Special case - don't guardband at endpoints */
	if((in == 0) || (in == 0xFFF))
	{
		diff = (in+rnd) / scale;
		if(diff == *old)
			return 0;
		else
		{
			*old = diff;
			return 1;
		}
	}
	
	/* find abs val of diff */
	diff = (int16_t)(*old * scale) - (int16_t)in;
	if(diff<0)
		diff = -diff;
	
	/* exceeds guardband ? */
	if(diff > guard)
	{
		/* yes so update prev */
		*old  = (in+rnd) / scale;
		result = 1;
	}
	
	return result;
}

