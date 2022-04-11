/*
 * fx.c - Algorithm access for rp2040_audio
 * 03-29-22 E. Brombaugh
 */
 
#include "fx.h"
#include "audio.h"
#include "fx_vca.h"
#include "fx_cdl.h"

/* pre-allocated internal memory for DSP */
uint32_t *fx_mem;

/* pointer to the fx data structure is void and recast inside the fns */
void *fx;

/* currently active algo */
uint8_t fx_algo;


/**************************************************************************/
/******************* Bypass algo definition *******************************/
/**************************************************************************/

const char *bypass_param_names[] =
{
	"p1    ",
	"p2    ",
	"p3    ",
};

/*
 * Bypass init
 */
void * fx_bypass_Init(uint32_t *mem)
{
	/* no init - just return pointer */
	return (void *)mem;
}

/*
 * Bypass Cleanup
 */
void fx_bypass_Cleanup(void *dummy)
{
	/* does nothing */
}

/*
 * Bypass audio process is just in-out loopback / bypass
 */
void __not_in_flash_func(fx_bypass_Proc)(void *dummy, int16_t *dst, int16_t *src, uint16_t sz)
{
	while(sz--)
	{
		*dst++ = 0;//*src++;
		*dst++ = 0;//*src++;
	}
}

/*
 * Bypass render parameter is just simple percentage
 */
void fx_bypass_Render_Parm(void *vblk, uint8_t idx)
{
	char txtbuf[32];
	GFX_RECT rect =
	{
		.x0 = 65,
		.y0 = idx*10+10,
		.x1 = 158,
		.y1 = idx*10+17
	};
	
	if(idx == 0)
		return;

	sprintf(txtbuf, "%2d%% ", ADC_param[idx]/41);
	gfx_drawstrrect(&rect, txtbuf);
}

/*
 * Bypass init
 */
const fx_struct fx_bypass_struct =
{
	"Bypass",
	3,
	bypass_param_names,
	fx_bypass_Init,
	fx_bypass_Cleanup,
	fx_bypass_Proc,
	fx_bypass_Render_Parm,
};


/**************************************************************************/

/* array of effect structures */
const fx_struct *effects[FX_NUM_ALGOS] = {
	&fx_bypass_struct,
	&fx_vca_struct,
	&fx_cdr_struct,
};

/*
 * initialize the effects library
 */
void fx_init(void)
{
	/* allocate internal buffer memory */
	printf("fx_init: attempt to allocate %d bytes internal RAM for audio...", FX_MAX_MEM);
	fx_mem = malloc(FX_MAX_MEM);
	if(fx_mem)
		printf("Success!\n");
	else
	{
		printf("Failed!!!\n");
		while(1){}
	}
	
	/* start off with bypass algo */
	fx_algo = 0;
	fx = effects[fx_algo]->init(fx_mem);
}

/*
 * switch algorithms
 */
void fx_select_algo(uint8_t algo)
{
	uint8_t prev_fx_algo;
	
	/* only legal algorithms */
	if(algo >= FX_NUM_ALGOS)
		return;
	
	/* bypass during init */
	prev_fx_algo = fx_algo;
	Audio_Set_Algo(&fx_algo, 0);
	
	/* cleanup previous effect */
	effects[prev_fx_algo]->cleanup(fx);
	
	/* init next effect from effect array */
	fx = effects[algo]->init(fx_mem);
		
	/* switch to next effect */
	Audio_Set_Algo(&fx_algo, algo);
}

/*
 * process audio through current effect
 */
void __not_in_flash_func(fx_proc)(int16_t *dst, int16_t *src, uint16_t sz)
{
	/* use effect structure function pointers */
	effects[fx_algo]->proc(fx, dst, src, sz);
}

/*
 * get current algorithm
 */
uint8_t fx_get_algo(void)
{
	return fx_algo;
}

/*
 * get number of params
 */
uint8_t fx_get_num_parms(void)
{
	return effects[fx_algo]->parms;
}

/*
 * get name of effect
 */
char * fx_get_algo_name(void)
{
	return (char *)effects[fx_algo]->name;
}

/*
 * get name of param by index
 */
char * fx_get_parm_name(uint8_t idx)
{
	return (char *)effects[fx_algo]->parm_names[idx];
}

/*
 * render parameter parts
 */
void fx_render_parm(uint8_t idx)
{
	effects[fx_algo]->render_parm(fx, idx);
}



