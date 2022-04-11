/*
 * fx_cdl.c -  Clean Delay effect for RP2040_Audio
 * 03-30-22 E. Brombaugh
 */
 
#include "fx_cdl.h"

#define XFADE_BITS 11

typedef struct 
{
	uint8_t type;			/* algo type */
	uint8_t rng;			/* short/med/long range */
	uint16_t rng_raw;		/* raw range from ADC param */
	int16_t *dlybuf;		/* external delay buffer address */
	uint32_t len;			/* buffer length */
	uint8_t init;			/* initial pass flag for cleanout */
	uint32_t wptr;			/* write pointer */
	uint32_t roff1, roff2;	/* read offsets - main and xfade */
	uint16_t xflen, xfcnt;	/* Cross-fade length and counter */
	int16_t dly;			/* delay value w/ hysteresis */
	int32_t dcb[2];			/* dc block on feedback */
	int16_t fb[2];
} fx_cdl_blk;

const char *cd_param_names[] =
{
	"DlyAmt",
	"Feedbk",
	"Range ",
};

const char *cd_ranges[] =
{
	"Short",
	"Medium",
	"Long",
};

/*
 * Clean Delay common init
 */
void * fx_cd_common_Init(uint32_t *mem, uint8_t type)
{
	/* set up instance in mem area provided */
	fx_cdl_blk *blk = (fx_cdl_blk *)mem;
	mem += sizeof(fx_cdl_blk)/sizeof(uint32_t);
	
	/* set type / range */
	blk->type = type>>2;
	blk->rng = 1+(type&0x3);
	blk->rng_raw = 0;
	
	/* init delay buffering */
	blk->dlybuf = (int16_t *)mem;
	blk->len = (FX_MAX_MEM-sizeof(fx_cdl_blk)) / (2*sizeof(int16_t));	// length in samples
	blk->init = 1;
	blk->wptr = 0;
	blk->roff1 = 1;
	blk->roff2 = 0;
	blk->xfcnt = 0;
	blk->xflen = 1<<XFADE_BITS;
	blk->dly = 0;
	blk->dcb[0] = blk->dcb[1] = 0;
	blk->fb[0] = blk->fb[1] = 0;
		
	/* return pointer */
	return (void *)blk;
}

/*
 * Clean Delay Range init
 */
void * fx_cdr_Init(uint32_t *mem)
{
	return fx_cd_common_Init(mem, 4);
}

/*
 * Clean Delay audio process
 */
void __not_in_flash_func(fx_cd_common_Proc)(void *vblk, int16_t *dst, int16_t *src, uint16_t sz)
{
	fx_cdl_blk *blk = vblk;
	uint16_t i;
	int16_t fb_lvl;
	int32_t rptr;
	int32_t mix;
	int16_t out;
	uint8_t chl;
	
	/* update delay parameters if not already crossfading */
	if(!blk->xfcnt)
	{
		uint8_t rng_upd = 0;
		
		/* set range realtime if type == 1 */
		if(blk->type)
		{
			rng_upd = dsp_ratio_hyst_arb(&blk->rng_raw, ADC_param[3], 2);
			blk->rng = 1+blk->rng_raw;
		}
		
		/* get raw delay value and apply hysteresis */
		if(dsp_gethyst(&blk->dly, ADC_param[1]) || rng_upd)
		{
			/* compute next delay and start crossfade */
			blk->roff2 = (blk->dly<<blk->rng) + 1;
			blk->roff2 = blk->roff2 > blk->len-2 ? blk->len-2 : blk->roff2;
			blk->xfcnt = blk->xflen;
		}
	}
	
	/* get the feedback value */
	fb_lvl = ADC_param[2];
	
	/* loop over the buffers */
	for(i=0;i<sz;i++)
	{
		for(chl=0;chl<2;chl++)
		{
			/* mix feedback into write buffer */
			mix = (*(src++)<<12) + blk->fb[chl] * fb_lvl;
			blk->dlybuf[2*blk->wptr+chl] = dsp_ssat16(mix>>12);
			
			/* get main tap */
			rptr = blk->wptr-blk->roff1;
			rptr = rptr < 0 ? blk->len + rptr : rptr;
			rptr = rptr >= blk->len ? rptr - blk->len : rptr;
			if(!blk->init || (rptr < blk->wptr))
				out = blk->dlybuf[2*rptr+chl];
			else
				out = 0;
			
			/* process crossfade */
			if(blk->xfcnt)
			{
				/* do crossfade mix */
				rptr = blk->wptr-blk->roff2;
				rptr = rptr < 0 ? blk->len + rptr : rptr;
				rptr = rptr >= blk->len ? rptr - blk->len : rptr;
				mix  = (int32_t)out * blk->xfcnt;
				if(!blk->init || (rptr < blk->wptr))
					mix += blk->dlybuf[2*rptr+chl] * (blk->xflen - blk->xfcnt);
				out = dsp_ssat16(mix>>XFADE_BITS);
				
				/* update crossfade */
				blk->xfcnt--;
				if(blk->xfcnt == 0)
				{
					/* update current delay */
					blk->roff1 = blk->roff2;
				}
			}
			
			/* dc block on feedback */
			mix = (int32_t)out - (blk->dcb[chl]>>8); 
			blk->dcb[chl] += mix;
			blk->fb[chl] = dsp_ssat16(mix);
			
			/* output */
			*dst++ = out;
		}
	
		/* update write pointer */
		rptr = blk->wptr;
		blk->wptr = (blk->wptr + 1) % blk->len;
		
		/* clear init flag if write ptr wrapped */
		if(rptr > blk->wptr)
			blk->init = 0;
	}
}

/*
 * Render parameter for clean delay - either delay in ms or feedback %
 */
void fx_cdl_Render_Parm(void *vblk, uint8_t idx)
{
	fx_cdl_blk *blk = vblk;
	char txtbuf[32];
	uint32_t ms;
	GFX_RECT rect =
	{
		.x0 = 65,
		.y0 = idx*10+10,
		.x1 = 158,
		.y1 = idx*10+17
	};
	
	if(idx == 0)
		return;
	
	switch(idx)
	{
		case 1:	// Delay
			ms = (blk->dly<<blk->rng) + 1;
			ms = ms > blk->len-2 ? blk->len-2 : ms;
			ms = ms / (SAMPLE_RATE/1000);
			sprintf(txtbuf, "%6d ms ", ms);
			break;
		
		case 3: // Range
			sprintf(txtbuf, "%s ", cd_ranges[blk->rng_raw]);
			fx_cdl_Render_Parm(vblk, 1);	// update Delay too
			break;
		
		case 2:	// Feedback
		default:
			sprintf(txtbuf, "%2d%% ", ADC_param[idx]/41);
			break;
	}
	gfx_drawstrrect(&rect, txtbuf);
}

/*
 * clean delay range struct
 */
fx_struct fx_cdr_struct =
{
	"ClnDly",
	3,
	cd_param_names,
	fx_cdr_Init,
	fx_bypass_Cleanup,
	fx_cd_common_Proc,
	fx_cdl_Render_Parm,
};

