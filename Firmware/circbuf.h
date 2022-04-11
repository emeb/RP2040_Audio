/******************************************************************************/
/* Title: circbuf.c                                                           */
/*                                                                            */
/* Function: Routines for circular buffers                                    */
/*                                                                            */
/* History: 2004-09-20 E. Brombaugh: Created                                  */
/******************************************************************************/

#ifndef _circbuf_
#define _circbuf_

#include "main.h"

typedef struct
{
	int16_t *buf;
	int32_t ptr;
	int32_t len;
} circbuf_int16_t;

void init_circbuf_int16_t(circbuf_int16_t *c, int16_t *buf, int32_t len);
void clear_circbuf_int16_t(circbuf_int16_t *c);
void put_circbuf_int16_t(circbuf_int16_t *c, int16_t in);
int16_t get_circbuf_int16_t(circbuf_int16_t *c, int32_t offset); 
void set_circbuf_int16_t(circbuf_int16_t *c, int16_t in, int32_t offset);

#endif
