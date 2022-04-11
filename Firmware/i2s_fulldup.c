/*
 * i2s_fulldup.c - PIO-based full-duplex I2S for RP2040
 * 12-19-21 E. Brombaugh
 *
 * Based on original ideas from
 * https://gist.github.com/jonbro/3da573315f066be8ea390db39256f9a7
 * and the pico-extras library.
 */

#include <stdio.h>
#include <string.h>
#include "i2s_fulldup.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/structs/clocks.h"
#include "hardware/clocks.h"
#include "pico/multicore.h"
#include "i2s_fulldup.pio.h"
#include "audio.h"

/* uncomment this to run audio processing on core 1 */
#define MULTICORE

/* I2S PIO has L+R (Frames) in 32-bits */
#define FRAMES_PER_BUFFER (BUFSZ/2)

/* I2S comes out on these pins */
#if 0
/* HW V0.1 */
#define I2S_DO_PIN 18		// data out
#define I2S_DI_PIN 17		// data in
#define I2S_CLK_PIN_BASE 24	// BCLK, LRCK (Temporarily on TP3, TP4)
#define I2S_MCLK_PIN 23		// MCLK

#define IN_DIAG_PIN 10		// TP5
#define OUT_DIAG_PIN 11		// TP6
#else
/* HW V0.2 */
#define I2S_DO_PIN 18		// data out
#define I2S_DI_PIN 19		// data in
#define I2S_CLK_PIN_BASE 16	// BCLK, LRCK
#define I2S_MCLK_PIN 23		// MCLK

#define IN_DIAG_PIN 10		// TP5
#define OUT_DIAG_PIN 11		// TP6
#endif

/* resources we use */
PIO pio;
uint sm;
uint dma_chan_input, dma_chan_output;
uint ib_idx, ob_idx;
uint32_t input_buf[2*FRAMES_PER_BUFFER], output_buf[2*FRAMES_PER_BUFFER],
	xfer_buf[FRAMES_PER_BUFFER];

/*
 * IRQ0 handler - used only for I2S input
 * ATM this is not double-buffered, but it would be prudent to
 * do so if adding code to compute the next buffer.
 */
void dma_input_handler(void)
{
	gpio_put(IN_DIAG_PIN, 1);
	
	/* Clear IRQ for I2S input */
    dma_irqn_acknowledge_channel(DMA_IRQ_0, dma_chan_input);
	
	/* reset write address to start of next buffer */
	ib_idx ^= 1;
	dma_channel_set_write_addr(dma_chan_input,
		&input_buf[ib_idx*FRAMES_PER_BUFFER],
		true
	);
	
	/* start next transfer sequence */
	dma_channel_start(dma_chan_input);
	
#if 0
	/* save previous buffer */
	memcpy(xfer_buf, &input_buf[ib_idx*FRAMES_PER_BUFFER],
		FRAMES_PER_BUFFER*sizeof(uint32_t));
#else
	/* do audio processing on previous buffer */
	Audio_Proc((int16_t *)xfer_buf,
	(int16_t *)&input_buf[(ib_idx^1)*FRAMES_PER_BUFFER],
		2*FRAMES_PER_BUFFER);
#endif

	gpio_put(IN_DIAG_PIN, 0);
}

/*
 * IRQ1 handler - used only for I2S output
 * ATM this is not double-buffered, but it would be prudent to
 * do so if adding code to compute the next buffer.
 */
void dma_output_handler()
{
	gpio_put(OUT_DIAG_PIN, 1);
	
	/* Clear IRQ for I2S output */
    dma_irqn_acknowledge_channel(DMA_IRQ_1, dma_chan_output);
	
	/* reset read address to start of next buffer */
	ob_idx ^= 1;
	dma_channel_set_read_addr(dma_chan_output,
		&output_buf[ob_idx*FRAMES_PER_BUFFER],
		true
	);
	
	/* start next transfer sequence */
	dma_channel_start(dma_chan_output);
	
	/* fill previous buffer with new data */
#if 1
	/* loopback */
	memcpy(&output_buf[ob_idx*FRAMES_PER_BUFFER], xfer_buf,
		FRAMES_PER_BUFFER*sizeof(uint32_t));
#else
	/* do audio processing */
	Audio_Proc((int16_t *)&output_buf[ob_idx*FRAMES_PER_BUFFER],
		(int16_t *)xfer_buf,
		2*FRAMES_PER_BUFFER);
#endif
	
	gpio_put(OUT_DIAG_PIN, 0);
}

#ifdef MULTICORE
/*
 * entry point for 2nd core to start running
 */
void core1_entry()
{
	/* enable IRQ handler for dma input */
    irq_set_exclusive_handler(DMA_IRQ_0, dma_input_handler);
    irq_set_enabled(DMA_IRQ_0, true);
	

	/* enable IRQ handler for dma output */
    irq_set_exclusive_handler(DMA_IRQ_1, dma_output_handler);
    irq_set_enabled(DMA_IRQ_1, true);
	
	/* enable multicore lockout */
	multicore_lockout_victim_init();

	/* loop here waiting for IRQs */
	while(1)
	{
		Audio_Fore();
	}
}
#endif

/*
 * initialize the I2S processing
 */
void init_i2s_fulldup(void)
{
	int32_t i;
	int16_t *obuf = (int16_t *)output_buf;
	int16_t *ibuf = (int16_t *)input_buf;
	
	/* clear buffers */
	for(i=0;i<2*FRAMES_PER_BUFFER;i++)
	{
		*ibuf++ = 0;
		*ibuf++ = 0;
		*obuf++ = 0;
		*obuf++ = 0;
	}

	/* diag GPIO */
	gpio_init(IN_DIAG_PIN);
	gpio_set_dir(IN_DIAG_PIN, GPIO_OUT);
	gpio_init(OUT_DIAG_PIN);
	gpio_set_dir(OUT_DIAG_PIN, GPIO_OUT);

    /* set up PIO */
    pio = pio0;
    uint offset = pio_add_program(pio, &i2s_fulldup_program);
    printf("loaded program at offset: %i\n", offset);
    sm = pio_claim_unused_sm(pio, true);
    printf("claimed sm: %i\n", sm);
	
	/* compute PIO divider for desired sample rate */
    uint32_t system_clock_frequency = clock_get_hz(clk_sys);
    assert(system_clock_frequency < 0x40000000);
    printf("System clock %u Hz\n", (uint) system_clock_frequency);
    uint32_t sample_freq = 48000;
    printf("Target sample freq %d\n", sample_freq);
    uint32_t divider = system_clock_frequency * 2 / sample_freq; // avoid arithmetic overflow
    divider = divider & ~(0x1ff); // mask off bottom 9 for exact MCLK/BCLK ratio of 4.0
    assert(divider < 0x1000000);
    printf("PIO clock divider 0x%x/256\n", divider);
	printf("Actual sample freq = %d\n", 2 * system_clock_frequency / divider);
	
	/* set up the PIO and divider */
    i2s_fulldup_program_init(
		pio,
		sm,
		offset,
		I2S_DO_PIN,
		I2S_DI_PIN,
		I2S_CLK_PIN_BASE
	);
    pio_sm_set_clkdiv_int_frac(pio, sm, divider >> 8u, divider & 0xffu);
	
	/* generate an MCLK on GPIO at 8x BCLK (256x LRCK) */
	gpio_set_function(I2S_MCLK_PIN, GPIO_FUNC_GPCK);
	clock_gpio_init(I2S_MCLK_PIN, CLOCKS_CLK_GPOUT1_CTRL_AUXSRC_VALUE_CLK_SYS, divider>>9);
	printf("MCLK at %d Hz\n", system_clock_frequency/(divider>>9));
	
    /* configure dma channel for input */
	ib_idx = 0;
    dma_chan_input = dma_claim_unused_channel(true);
	printf("DMA input using chl %d\n", dma_chan_input);
    dma_channel_config c = dma_channel_get_default_config(dma_chan_input);
    channel_config_set_read_increment(&c,false);
    channel_config_set_write_increment(&c,true);
    channel_config_set_dreq(&c,pio_get_dreq(pio,sm,false));
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);

    dma_channel_configure(
        dma_chan_input,
        &c,
        input_buf, 			// Destination pointer
        &pio->rxf[sm], 		// Source pointer
        FRAMES_PER_BUFFER, // Number of transfers
        true				// Start immediately
    );
    dma_channel_set_irq0_enabled(dma_chan_input, true);

    /* configure dma channel for output */
	ob_idx = 0;
	dma_chan_output = dma_claim_unused_channel(true);
	printf("DMA output using chl %d\n", dma_chan_output);
    dma_channel_config cc = dma_channel_get_default_config(dma_chan_output);
    channel_config_set_read_increment(&cc,true);
    channel_config_set_write_increment(&cc,false);
    channel_config_set_dreq(&cc,pio_get_dreq(pio,sm,true));
    channel_config_set_transfer_data_size(&cc, DMA_SIZE_32);
    dma_channel_configure(
        dma_chan_output,
        &cc,
        &pio->txf[sm],		// Destination pointer
        output_buf,			// Source pointer
        FRAMES_PER_BUFFER,	// Number of transfers
        true				// Start immediately
    );
    dma_channel_set_irq1_enabled(dma_chan_output, true);
    
#ifndef MULTICORE
	/* No multi-core - just use core 0 */
	
	/* enable IRQ handler for dma input */
    irq_set_exclusive_handler(DMA_IRQ_0, dma_input_handler);
    irq_set_enabled(DMA_IRQ_0, true);

	/* enable IRQ handler for dma output */
    irq_set_exclusive_handler(DMA_IRQ_1, dma_output_handler);
    irq_set_enabled(DMA_IRQ_1, true);
	
	printf("Single core background started\n");
#else
	/* multi-core operation */
	multicore_launch_core1(core1_entry);
	printf("Multicore background started\n");
#endif

	/* Start PIO */
    pio_sm_set_enabled(pio, sm, true);
	printf("PIO started\n");
}
