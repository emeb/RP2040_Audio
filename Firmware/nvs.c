/*
 * nvs.c - non-volatile storage API for RP2040
 * 04-01-2022 E. Brombaugh
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hardware/flash.h"
#include "nvs.h"

#define FLASH_MB 2
#define FLASH_START 0x10000000
#define FLASH_END (FLASH_START+FLASH_MB*(1<<20))
#define FLASH_ERASE_PG 4096
#define FLASH_WRITE_PG 256
#define NVS_MAX_MEM 1024
#define NVS_MAX_PGS 8
#define NVS_START (FLASH_END-NVS_MAX_PGS*FLASH_ERASE_PG)

static uint32_t *nvs_buff;			// buffered tags for committing
static uint32_t *nvs_end;			// next writeable tag in flash
static uint16_t nvs_save_tags;		// # tags in buffer, index to next
static uint32_t *nvs_pg_buff;		// flash write pg buffer

/*
 * initialize non-volatile API
 */
uint8_t nvs_init(void)
{
	uint32_t i, *entry;
	
	/* allocate internal buffer memory */
	printf("nvs_init: Allocating %d bytes internal RAM for nvs_buff...", NVS_MAX_MEM);
	nvs_buff = malloc(NVS_MAX_MEM);
	if(nvs_buff)
		printf("Success!\n");
	else
	{
		printf("Failed!!!\n");
		while(1){}
	}
	
	/* allocate page buffer */
	printf("nvs_init: Allocating %d bytes internal RAM for nvs_pg_buff...", 256);
	nvs_pg_buff = malloc(256);
	if(nvs_pg_buff)
		printf("Success!\n");
	else
	{
		printf("Failed!!!\n");
		while(1){}
	}
	
	/* find end of NVS */
	entry = (uint32_t *)NVS_START;
	printf("nvs_init: start of NVS flash space @0x%08X\n", entry);
	for(i=0;i<(NVS_MAX_PGS*FLASH_ERASE_PG/sizeof(uint32_t));i++)
	{
		if(*entry & 0x10000)
			break;
		entry++;
	}
	nvs_end = entry;
	printf("nvs_init: end of NVS flash space @0x%08X\n", entry);
	printf("nvs_init: found %d tags in flash\n",
		((uint32_t)(entry)-NVS_START)/sizeof(uint32_t));

#if 0
	/* dump first page of tags */
	entry = (uint32_t *)NVS_START;
	for(i=0;i<128;i+= 4)
	{
		printf("%08X: ", (uint32_t)entry);
		for(uint8_t j=0;j<4;j++)
		{
			printf("%08X ", *entry++);
		}
		printf("\n");
	}
#endif	
	
	/* init state */
	nvs_save_tags = 0;
}

/*
 * get a tag from NVS
 */
uint8_t nvs_get_tag(uint8_t tag, int16_t *value)
{
	uint8_t result = 1;
	uint32_t *entry = nvs_end-1;
	
	/* scan backwards from end searching for tag */
	while(entry >= (uint32_t *)NVS_START)
	{
		if((*entry >> 24)==tag)
		{
			result = 0;
			*value = *entry & 0xffff;
			break;
		}
		entry--;
	}
	
	return result;
}

/*
 * put a tag to NVS buffer
 */
void nvs_put_tag(uint8_t tag, int16_t value)
{
	uint16_t i;
	
	/* search thru buffer to see if we're updating an existing tag */
	for(i=0;i<nvs_save_tags;i++)
	{
		if((nvs_buff[i]>>24) == tag)
		{
			/* found one in the list, so update the value and quit */
			nvs_buff[i] = (nvs_buff[i] & 0xff000000) | value;
			return;
		}
	}
	
	/* none found so add to new one to buffer and advance index */
	nvs_buff[nvs_save_tags++] = (tag << 24) | (value&0xffff);
}

/*
 * commit put tags to flash memory
 */
uint8_t nvs_commit(void)
{
#if 0
	/* doesn't do anything yet - just delays for 10ms */
	uint64_t dlytime = time_us_64() + 10000;
	while(time_us_64() < dlytime) {}

	return 0;
#else
	/* keep going until no more tags in buffer */
	printf("nvs_commit: %d tags in buffer\n", nvs_save_tags);
	while(nvs_save_tags)
	{
		/* room for more tags in flash? */
		if(nvs_end != (uint32_t *)FLASH_END)
		{
			/* Yes - update currently active page */
			/* get page and tag addresses for writing */
			uint32_t pg_addr = ((nvs_end - (uint32_t *)FLASH_START)*sizeof(uint32_t));
			uint32_t tag_idx = (pg_addr & FLASH_WRITE_PG-1)/sizeof(uint32_t);
			pg_addr &= ~(FLASH_WRITE_PG-1);
			printf("nvs_commit: adding to page 0x%08X:0x%02X\n", pg_addr, tag_idx);
			
			/* load current page to write buffer */
			memcpy(nvs_pg_buff, (uint32_t *)(NVS_START+pg_addr), FLASH_WRITE_PG);
			
			/* add saved tags to write buffer until done or full page */
			uint8_t tcnt = 0;
			while(nvs_save_tags && (tag_idx != (FLASH_WRITE_PG/sizeof(uint32_t))))
			{
				/* add tag to write buffer and adjust indices */
				nvs_pg_buff[tag_idx++] = nvs_buff[--nvs_save_tags];
				
				/* adjust end pointer */
				nvs_end++;
				tcnt++;
			}
			
			/* write page to flash */
			printf("nvs_commit: flashing %d new tags...", tcnt);
			flash_range_program(pg_addr, (const unsigned char *)nvs_pg_buff, FLASH_WRITE_PG);
			printf("done. %d tags remaining.\n", nvs_save_tags);
		}
		else
		{
			/* No - Flash is full, time to reset */
			printf("nvs_commit: flash full - resetting\n");
			
			/* load tags from flash that aren't already in buffer */
			for(uint16_t tag = 0;tag < 256;tag++)
			{
				/* try to get a tag/value pair */
				int16_t value;
				if(!nvs_get_tag(tag, &value))
				{
					/* tag exists in flash, see if newer one in buffer */
					uint8_t newer = 0;
					for(uint16_t i=0;i<nvs_save_tags;i++)
					{
						if((nvs_buff[i]>>24) == tag)
						{
							/* found in the buffer so set flag */
							newer = 1;
							break;
						}
					}
					
					if(!newer)
					{
						/* none newer found so add to existing and advance index */
						nvs_buff[nvs_save_tags++] = (tag << 24) | (value&0xffff);
					}
				}
			}
			
			/* Erase Flash */
			uint32_t flash_offs = (uint32_t)nvs_buff - FLASH_START;
			size_t count = (FLASH_END - flash_offs);
			flash_range_erase(flash_offs, count);
			
			/* reset end pointer to beginning */
			nvs_end = nvs_buff;
		}
	}
	
	printf("nvs_commit: complete\n");
	
	return 0;
#endif
}
