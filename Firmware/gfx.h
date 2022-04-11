/*
 * gfx.h - high-level graphics library
 * 03-12-20 E. Brombaugh
 */

#ifndef __gfx__
#define __gfx__

#include "main.h"

// Color definitions
#define GFX_BLACK   0x00000000
#define GFX_BLUE    0x000000FF
#define GFX_GREEN   0x0000FF00
#define GFX_CYAN    0x0000FFFF
#define GFX_RED     0x00FF0000
#define GFX_MAGENTA 0x00FF00FF
#define GFX_YELLOW  0x00FFFF00
#define GFX_DGRAY   0x00505050
#define GFX_LGRAY   0x00A0A0A0
#define GFX_PINK    0x00F8A0F8
#define GFX_LORANGE 0x00F8A000
#define GFX_LYELLOW 0x00F8FCA0
#define GFX_LGREEN  0x0000FCA0
#define GFX_LBLUE   0x0000A0F8
#define GFX_LPURPLE 0x00F850F8
#define GFX_WHITE   0x00FFFFFF
#define GFX_CREAM	0x00DAFFFF
#define GFX_LSLATE	0x006D6D48

enum gfx_txtmodes
{
	GFX_TXTNORM,
	GFX_TXTREV = 0x80,
};

typedef uint32_t GFX_COLOR;

/*
 * this structure holds function pointers to the
 * specific LCD driver routines
 */
typedef struct
{
	uint16_t xmax, ymax;
	void (*init)(void);
	void (*setRotation)(uint8_t m);
	uint16_t (*Color565)(GFX_COLOR rgb24);
	GFX_COLOR (*ColorRGB)(uint16_t color565);
	void (*fillRect)(int16_t x, int16_t y, int16_t w, int16_t h,
		uint16_t color);
	void (*drawPixel)(int16_t x, int16_t y, uint16_t color);
	void (*bitblt)(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *buf);
} GFX_DRIVER;

typedef struct
{
	int16_t x;
	int16_t y;
} GFX_POINT;

typedef struct
{
	int16_t x0;
	int16_t y0;
	int16_t x1;
	int16_t y1;
} GFX_RECT;

extern const GFX_COLOR gfx_colortab[];

void gfx_init(GFX_DRIVER *drvr);
int16_t gfx_getcolor(GFX_COLOR color);
void gfx_set_forecolor(GFX_COLOR color);
GFX_COLOR gfx_get_forecolor(void);
void gfx_set_backcolor(GFX_COLOR color);
GFX_COLOR gfx_get_backcolor(void);
void gfx_clrscreen(void);
void gfx_setpixel(GFX_POINT pixel);
void gfx_clrpixel(GFX_POINT pixel);
void gfx_fillrect(GFX_RECT *rect);
void gfx_clrrect(GFX_RECT *rect);
void gfx_drawhline(int16_t y, int16_t x0, int16_t x1);
void gfx_drawvline(int16_t x, int16_t y0, int16_t y1);
void gfx_drawrect(GFX_RECT *rect);
void gfx_drawline(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void gfx_drawcircle(int16_t x, int16_t y, int16_t radius);
void gfx_fillcircle(int16_t x, int16_t y, int16_t radius);
void gfx_set_txtscale(uint8_t scale);
void gfx_set_txtmode(uint8_t mode);
void gfx_drawchar(int16_t x, int16_t y, uint8_t chr);
void gfx_drawstr(int16_t x, int16_t y, char *str);
void gfx_drawstrctr(int16_t x, int16_t y, char *str);
void gfx_drawstrrect(GFX_RECT *rect, char *str);
void gfx_bitblt(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *buf);
GFX_COLOR gfx_hsv2rgb(uint8_t hsv[]);

#endif
