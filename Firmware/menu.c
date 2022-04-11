/*
 * menu.c - menu / UI handler for RP2040 Audio
 * 03-28-22 E. Brombaugh
 */
#include <stdio.h>
#include <string.h>
#include "hardware/sync.h"
#include "menu.h"
#include "button.h"
#include "gfx.h"
#include "widgets.h"
#include "audio.h"
#include "fx.h"
#include "wm8731.h"
#include "adc.h"
#include "nvs.h"
#include "i2s_fulldup.h"

#define MENU_INTERVAL 50000
#define MENU_MAX_PARAMS (FX_MAX_PARAMS+1)
#define MENU_VU_WIDTH 50

enum save_flags
{
	SAVE_ACT = 1,
	SAVE_VALUE = 2,
	SAVE_ALGO = 4,
};

enum tags
{
	TAG_ALGO = 0,
	TAG_ACT = 1,
};

static int16_t menu_item_values[FX_NUM_ALGOS][MENU_MAX_PARAMS];
static uint8_t menu_reset, menu_act_item;
static uint16_t menu_algo, menu_save_counter;
static uint64_t menu_time;
static char txtbuf[32];

/*
 * commit state to flash - needs to shut down lots of stuff
 */
void menu_save_state(void)
{
	uint32_t irqs;

	printf("menu_save_state: committing tags to flash\n");
	
	/* mute */
	Audio_Set_Mute(1);
	printf("muted\n");
	
	/* shut down core 0 IRQs */
	irqs = save_and_disable_interrupts();
	printf("core 0 irq disabled\n");
	
	/* shut down core 1 IRQs */
	Audio_Disable_Core(1);
	printf("core 1 blocked\n");

	/* commit tags to flash */
	nvs_commit();
	printf("committed\n");

	/* re-enable core 1 IRQs */
	Audio_Disable_Core(0);
	printf("core 1 unblocked\n");

	/* re-enable core 0 IRQs */
	restore_interrupts(irqs);
	printf("core 0 irq enabled\n");
	
	/* unmute */
	Audio_Set_Mute(0);			// hangs if flash was written
	printf("unmuted\n");
}

/* 
 * Load operation state from flash - for RP2040 only
 */
uint8_t menu_load_state(void)
{
	uint8_t i,j, commit=0;
	uint8_t tag;
	int16_t raw_param;
	
	/* init NVS */
	nvs_init();
	
	/* set state */
	tag = TAG_ALGO;
	raw_param = 0;
	if(!nvs_get_tag(tag, &raw_param))
	{
		printf("menu_load_state: fetched tag %d [menu_algo] = %d\n", tag, raw_param);
	}
	else
	{
		nvs_put_tag(tag, raw_param);
		commit = 1;
		printf("menu_load_state: created tag %d [menu_algo] = %d\n", tag, raw_param);
	}
	menu_algo = raw_param;
	
	tag = TAG_ACT;
	raw_param = 0;
	if(!nvs_get_tag(tag, &raw_param))
	{
		printf("menu_load_state: fetched tag %d [menu_act_item] = %d\n", tag, raw_param);
	}
	else
	{
		nvs_put_tag(tag, raw_param);
		commit = 1;
		printf("menu_load_state: created tag %d [menu_act_item] = %d\n", tag, raw_param);
	}
	menu_act_item = raw_param;
	
	/* get params */
	for(i=0;i<FX_NUM_ALGOS;i++)
	{
		for(j=0;j<MENU_MAX_PARAMS;j++)
		{
			tag = (i+1)<<2|j;
			raw_param = 0;
			
			if(!nvs_get_tag(tag, &raw_param))
			{
				/* tag was found so use it */
				printf("menu_load_state: fetched tag %d [%s:%d] = %d\n", tag, effects[i]->name, j, raw_param);
			}
			else
			{
				/* tag was not found so create it */
				nvs_put_tag(tag, raw_param);
				commit = 1;
				printf("menu_load_state: created tag %d [%s:%d] = %d\n", tag, effects[i]->name, j, raw_param);
			}
			
			/* assign value to param */
			menu_item_values[i][j] = raw_param;
		}
	}
	
	/* tags created so commit */
	if(commit)
	{
		menu_save_state();
		printf("menu_load_state: commit complete.\n");
	}
	
	return 0;
}

/*
 * periodic menu updates
 */
void menu_timer_callback(void)
{
	/* update load */
	gfx_set_forecolor(GFX_WHITE);
	uint64_t load;
	
	/* update load indicator */
	if(audio_period != 0)
	{
		load = 100*audio_duty/audio_period;
		sprintf(txtbuf, "%2d%% ", (uint32_t)load);
		gfx_drawstr(40, 0, txtbuf);
	}
	
	/* update state save */
	if(menu_save_counter != 0)
	{
		menu_save_counter--;
		
		if(menu_save_counter == 0)
		{
			printf("menu_timer_callback: Saving State\n");
			menu_save_state();
			gfx_set_forecolor(GFX_GREEN);
			gfx_fillcircle(156, 3, 3);
			gfx_set_forecolor(GFX_WHITE);
		}
	}

	menu_item_values[menu_algo][menu_act_item] = ADC_param[menu_act_item];
	fx_render_parm(menu_act_item);
	
	/* update mix */
	widg_sliderH(30, 50, 100, 8, ADC_val[1]/41);
	
	/* update VU meters */
	widg_bargraphHG(20, 60, MENU_VU_WIDTH, 8, audio_sl[1]/328); audio_sl[1] = 0;
	widg_bargraphHG(20, 70, MENU_VU_WIDTH, 8, audio_sl[0]/328); audio_sl[0] = 0;
	widg_bargraphHG(100, 60, MENU_VU_WIDTH, 8, audio_sl[3]/328); audio_sl[3] = 0;
	widg_bargraphHG(100, 70, MENU_VU_WIDTH, 8, audio_sl[2]/328); audio_sl[2] = 0;
}

/*
 * put a param to NVS and schedule commit in the future
 */
void menu_sched_save(uint8_t mask)
{
	if(mask & SAVE_ACT)
	{
		nvs_put_tag(TAG_ACT, menu_act_item);
		printf("menu_sched_save: Scheduling menu_act_item = %d save\n",
			menu_act_item);
	}
	
	if(mask & SAVE_ALGO)
	{
		nvs_put_tag(TAG_ALGO, menu_algo);
		printf("menu_sched_save: Scheduling menu_algo = %d save\n",
			menu_algo);
	}
	
	if(mask & SAVE_VALUE)
	{
		uint8_t tag = (menu_algo+1)<<2|menu_act_item;
		nvs_put_tag(tag, menu_item_values[menu_algo][menu_act_item]);
		printf("menu_sched_save: Scheduling [%s:%d] = %d save\n",
			effects[menu_algo]->name, menu_act_item,
			menu_item_values[menu_algo][menu_act_item]);
	}
	
	/* set timeout */
	menu_save_counter = 5000000/MENU_INTERVAL;	 // 5 sec
	gfx_set_forecolor(GFX_RED);
	gfx_fillcircle(156, 3, 3);
	gfx_set_forecolor(GFX_WHITE);
}

/*
 * render any changes in the menu
 */
void menu_render(void)
{
	uint8_t i;
	char *name;
	GFX_RECT rect;
	
	/* set constants */
	gfx_set_forecolor(GFX_WHITE);
	GFX_COLOR fgcolor = gfx_get_forecolor(), bgcolor = gfx_get_backcolor();
	
	/* update active item */
	for(i=0;i<MENU_MAX_PARAMS;i++)
	{
		/* highlight active item */
		if(menu_act_item == i)
			gfx_set_forecolor(GFX_MAGENTA);
		else
			gfx_set_forecolor(bgcolor);
		rect.x0 = 0;
		rect.y0 = i*10+9;
		rect.x1 = 159;
		rect.y1 = rect.y0+9;
		gfx_drawrect(&rect);
		
		/* item names */
		gfx_set_forecolor(fgcolor);
		if(i == 0)
		{
			name = "Algo:";
			gfx_drawstr(1, i*10+10, name);
		}
		else
		{
			if(i<fx_get_num_parms()+1)
			{
				/* render parm name and clear rest of line */
				//fx_render_parm(i, PARM_NAME);
				gfx_drawstr(1, i*10+10, fx_get_parm_name(i-1));
			}
			else
			{
				/* past last parm so clear whole line */
				rect.x0 = 1;
				rect.y0 = i*10+10;
				rect.x1 = 158;
				rect.y1 = rect.y0+7;
				gfx_clrrect(&rect);
			}
		}
		
		/* init all item values */
		if(menu_reset)
		{
			if(i == 0)
			{
				sprintf(txtbuf, "%s             ", fx_get_algo_name());
				txtbuf[13] = 0;	// max 13 chars 
				gfx_drawstr(48, i*10+10, txtbuf);
			}
			else
			{
				if(i<fx_get_num_parms()+1)
				{
					fx_render_parm(i);
				}
			}
		}
	}
	
	/* refresh static items */
	if(menu_reset)
	{
		menu_reset = 0;
		
		/* save state */
		if(menu_save_counter)
			gfx_set_forecolor(GFX_RED);
		else
			gfx_set_forecolor(GFX_GREEN);
		gfx_fillcircle(156, 3, 3);
		
		/* load % */
		gfx_set_forecolor(GFX_WHITE);
		gfx_drawstr(0, 0, "Load");
		
		/* W/D mix */
		gfx_drawstr(0,   50, "Dry");
		gfx_drawstr(136, 50, "Wet");
		
		/* vu meters labels and boxes */
		gfx_drawstr(0, 61, "il");
		widg_bargraphH(20, 60, MENU_VU_WIDTH, 8, 0);
		gfx_drawstr(0, 71, "ir");
		widg_bargraphH(20, 70, MENU_VU_WIDTH, 8, 0);
		gfx_drawstr(80, 61, "ol");
		widg_bargraphH(100, 60, MENU_VU_WIDTH, 8, 0);
		gfx_drawstr(80, 71, "or");
		widg_bargraphH(100, 70, MENU_VU_WIDTH, 8, 0);
		
	}
}

/*
 * initialize menu handler
 */
void menu_init(void)
{
	uint8_t i;
	
 	/* init menu state */
	printf("menu_init: zeroing\n");
	menu_reset = 1;
	menu_save_counter = 0;
	
	/* load stored state */
	printf("menu_init: load state\n");
	menu_load_state();
	printf("menu_init: ADC_setactparam.\n");
	ADC_setactparam(menu_act_item);
	for(i=0;i<MENU_MAX_PARAMS;i++)
		ADC_setparamval(i, menu_item_values[menu_algo][i]);
	printf("menu_init: ADC_forceactparam.\n");
	ADC_forceactparam();
	printf("menu_init: fx_select_algo.\n");	// hangs
	fx_select_algo(menu_algo);
	printf("menu_init: Audio_Set_Mute.\n");
	Audio_Set_Mute(0);	// initial unmute after algo selected
	
	/* init VU gradient */
	printf("menu_init: init gradient.\n");
	widg_gradient_init(MENU_VU_WIDTH);
	
	/* initial draw of menu */
	gfx_clrscreen();
	menu_render();
	
	/* set time for next update */
	menu_time = time_us_64() + MENU_INTERVAL;
}

/*
 * periodic menu update call
 */
void menu_update(void)
{
	/* look for button press */
	if(button_re())
	{
		/* save value of currently selected param */
		menu_sched_save(SAVE_VALUE);
		
		/* advance active param */
		menu_act_item++;
		menu_act_item = menu_act_item < fx_get_num_parms()+1 ? menu_act_item : 0;
		
		/* save new active param */
		menu_sched_save(SAVE_ACT | SAVE_VALUE);
		
		/* adjust active param for ADC muxing */
		ADC_setactparam(menu_act_item);
		
		printf("menu_update: act_item = %d\n", menu_act_item);
		menu_render();
	}
	
	/* test for algo change */
	if(menu_act_item == 0)
	{
		if(dsp_ratio_hyst_arb(&menu_algo, ADC_param[0]&0xfff, FX_NUM_ALGOS-1))
		{
			printf("menu_update: algo = %d\n", menu_algo);
			Audio_Set_Mute(1);
			menu_sched_save(SAVE_ALGO);
			fx_select_algo(menu_algo);
			for(int i=1;i<MENU_MAX_PARAMS;i++)
				ADC_setparamval(i, menu_item_values[menu_algo][i]);
			menu_reset = 1;
			menu_render();
			Audio_Set_Mute(0);
		}
	}
	
	/* periodic updates in foreground to avoid conflicts */
	if(time_us_64() >= menu_time)
	{
		menu_time = time_us_64() + MENU_INTERVAL;
		menu_timer_callback();
	}
}
