/******************************************************************************/
/* Title: circbuf.c                                                          */
/*                                                                            */
/* Function: Routines for circular buffers                                    */
/*                                                                            */
/* History: 2004-09-20 E. Brombaugh: Created                                  */
/******************************************************************************/

#include "circbuf.h"

/* routines for int16_t data */

/* initialize the circular buffer */
void init_circbuf_int16_t(circbuf_int16_t *c, int16_t *buf, int32_t len)
{
	c->buf = buf;
	c->len = len;
	clear_circbuf_int16_t(c);
}

/* zero out an existing circular buffer */
void clear_circbuf_int16_t(circbuf_int16_t *c)
{
	int32_t i;
	for(i=0;i<c->len;i++)
		c->buf[i] = 0;
	c->ptr = 0;
}

/* advance circular buffer one sample and insert data */
void __not_in_flash_func(put_circbuf_int16_t)(circbuf_int16_t *c, int16_t in)
{ 	
	/* increment and adjust circular buffer point16_ter */
    (c->ptr)++;
	c->ptr = (c->ptr == c->len) ? 0 : c->ptr;
	
	/* clear the new location */
    c->buf[c->ptr] = in;
}

/* get data out of circular buffer */
int16_t __not_in_flash_func(get_circbuf_int16_t)(circbuf_int16_t *c, int32_t offset)
{
	int32_t ptr = c->ptr - offset;

	ptr = (ptr < 0) ? c->len + ptr : ptr;
	
	return c->buf[ptr];
}

/* stuff a value int16_to the circular buffer at an offset from current location */
void __not_in_flash_func(set_circbuf_int16_t)(circbuf_int16_t *c, int16_t in, int32_t offset)
{
	int32_t ptr = c->ptr - offset;

	ptr = (ptr < 0) ? c->len + ptr : ptr;
	c->buf[ptr] = in;
}
