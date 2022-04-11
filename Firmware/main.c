/*
 * main.c - part of rp2040_audio. Includes WM8731 codec + LCD.
 * WM8731 requires full-duplex operation so the generic I2S
 * output driver provided for RPi RP2040 will not work for it.
 * 03-28-22 E. Brombaugh
 */

#include <stdio.h>
#include "main.h"
#include "pico/unique_id.h"
#include "i2s_fulldup.h"
#include "wm8731.h"
#include "audio.h"
#include "st7735.h"
#include "button.h"
#include "adc.h"
#include "gfx.h"
#include "menu.h"
#include "splash.h"

/* build version in simple format */
const char *fwVersionStr = "V0.2";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

/*
 * main prog
 */
int main()
{
	int32_t i;
	uint16_t prog = 255;
	char textbuf[32];
	pico_unique_board_id_t id_out;
	
	/* startup msg */
    stdio_init_all();
	printf("\n\nRP2040_Audio - Audio DSP module %s starting\n\r", fwVersionStr);
	printf("CHIP_ID: 0x%08X\n\r", *(volatile uint32_t *)(SYSINFO_BASE));
	printf("BOARD_ID: 0x");
	pico_get_unique_board_id(&id_out);
	for(i=0;i<PICO_UNIQUE_BOARD_ID_SIZE_BYTES;i++)
		printf("%02X", id_out.id[i]);
	printf("\n");
	printf("Build Date: %s\n\r", bdate);
	printf("Build Time: %s\n\r", btime);
	printf("\n");

	/* button input */
	button_init();
	printf("Button Initialized\n");
	
	/* init codec */
	WM8731_Init();
	printf("Codec Initialized\n");
	
	/* init ADC for cvs */
	ADC_init();
	printf("ADC Initialized\n");
	
	/* init Audio */
	Audio_Init();
	printf("Audio Initialized\n");
	
	/* init Audio */
	init_i2s_fulldup();
	printf("I2S Initialized\n");
	
	/* Init LCD */
	gfx_init(&ST7735_drvr);
	gfx_bitblt(0, 0, 160, 80, (uint16_t *)splash_png_565);
	gfx_set_forecolor(GFX_BLACK);
	gfx_set_backcolor(0xc7def6);
	gfx_drawstrctr(120, 20, "RP2040");
	gfx_drawstrctr(120, 30, "Audio");
	gfx_drawstrctr(120, 40, (char *)fwVersionStr);
	gfx_set_forecolor(GFX_WHITE);
	gfx_set_backcolor(GFX_BLACK);
	ST7735_setBacklight(1);
	printf("GFX Initialized\n");
	
	/* splash delay */
	printf("Splash delay\n");
	sleep_ms(3000);
	
	/* init menu */
    printf("Init menu\n");
	menu_init();
	
	/* loop here forever */
	printf("Looping...\n");
    while(true)
    {
		//printf("%1d 0x%03X 0x%03X\r", button_get(), ADC_val[0], ADC_val[1]);
		menu_update();
	}

	/* should never get here */
	return 0;
}