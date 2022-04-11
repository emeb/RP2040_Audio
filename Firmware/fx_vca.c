/*
 * fx_vca.c -  VCA effect for rp2040_audio
 * 03-30-22 E. Brombaugh
 */
 
#include "fx_vca.h"

typedef struct 
{
	int16_t gain;
} fx_vca_blk;

const char *vca_param_names[] =
{
	"Gain  ",
	"",
	"",
};

/*
 * VCA init
 */
void * fx_vca_Init(uint32_t *mem)
{
	/* set up instance in mem area provided */
	fx_vca_blk *blk = 	(fx_vca_blk *)mem;
	
	/* initialize gain slewing */
	blk->gain = 0;
	
	/* return pointer */
	return (void *)blk;
}

/*
 * VCA audio process
 */
void __not_in_flash_func(fx_vca_Proc)(void *vblk, int16_t *dst, int16_t *src, uint16_t sz)
{
	fx_vca_blk *blk = vblk;
	int16_t next_gain, gain_slope;
	int32_t mix;
	
	/* get the gain value & calc slew */
	next_gain = ADC_param[1];
	gain_slope = (next_gain - blk->gain)/sz;
	
	/* loop over the buffer */
	while(sz--)
	{
		mix = *src++ * blk->gain;
		*dst++ = dsp_ssat16(mix>>12);
		mix = *src++ * blk->gain;
		*dst++ = dsp_ssat16(mix>>12);
		blk->gain += gain_slope;
	}
}

fx_struct fx_vca_struct =
{
	"VCA",
	1,
	vca_param_names,
	fx_vca_Init,
	fx_bypass_Cleanup,
	fx_vca_Proc,
	fx_bypass_Render_Parm,
};

