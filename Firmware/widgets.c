/*
 * widgets.c - UI graphics widgets for ESP32S2 Audio
 * 03-20-22 E. Brombaugh
 */
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "gfx.h"
#include "menu.h"

#define GRAD_MAX_WIDTH 100

static uint16_t gradient[2][GRAD_MAX_WIDTH];

/*
 * Horizontal bargraph
 */
void widg_bargraphH(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t v)
{
	GFX_RECT rect;
	uint16_t max, b;
	
	/* bounding box */
	rect.x0 = x;
	rect.y0 = y;
	rect.x1 = rect.x0+w;
	rect.y1 = rect.y0+h;
	gfx_drawrect(&rect);
	
	/* active area on left */
	max = w-2;
	b = (max*v)/100;
	rect.x0 = x+1;
	rect.y0 = y+1;
	rect.x1 = rect.x0+b;
	rect.y1 = rect.y0+(h-2);
	gfx_fillrect(&rect);
	
	/* inactive area on right */
	if(max-b)
	{
		rect.x0 = x+1+b;
		rect.x1 = rect.x0+(max-b);
		gfx_clrrect(&rect);
	}
}

/*
 * initialize a gradient for the HG bargraphs
 */
uint8_t widg_gradient_init(int16_t width)
{
	uint8_t i;
	uint8_t hsv[3];
	
	if(width > GRAD_MAX_WIDTH)
		return 1;
	
	hsv[1] = 255;
	hsv[2] = 255;
	for(i=0;i<(width-2);i++)
	{
		hsv[0] = 85-(17*i/10);
		gradient[0][i] = gfx_getcolor(gfx_hsv2rgb(hsv));
	}
	
	return 0;
}

/*
 * Horizontal bargraph with gradient and no bounding box
 */
void widg_bargraphHG(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t v)
{
	uint16_t max, b, t;
	
	if(w > GRAD_MAX_WIDTH)
		return;
	
	/* scale */
	max = w-2;
	b = (max*v)/100;

	/* copy gradient to temp */
	memcpy(gradient[1], gradient[0], max*sizeof(uint16_t));
	
	/* inactive area on right */
	if(max-b)
		memset(&gradient[1][b+1], 0, sizeof(uint16_t)*(max-b));
	
	/* blit gradient to display line at a time */
	for(t=0;t<(h-1);t++)
		gfx_bitblt(x+1, y+1+t, max, 1, gradient[1]);
}

/*
 * Horizontal slider
 */
void widg_sliderH(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t v)
{
	uint16_t fore, b, t;
	
	if(w > GRAD_MAX_WIDTH)
		return;
	
	/* clear out part of gradient array */
	memset(&gradient[1][0], 0, sizeof(uint16_t)*w);
	
	/* draw thumb to scale */
	fore = gfx_getcolor(gfx_get_forecolor());
	b = ((w-2)*v)/100;
	gradient[1][b] = fore;
	gradient[1][b+1] = fore;
	gradient[1][b+2] = fore;
	
	/* blit to display line at a time */
	for(t=0;t<h;t++)
		if(t!=(h>>1))
			gfx_bitblt(x, y+t, w, 1, gradient[1]);
		else
			gfx_drawhline(y+t, x, x+w);
}