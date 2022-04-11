/*
 * ADC.c - background ADC management
 */

#include <stdlib.h>
#include "adc.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

/* uncomment this for autoscan - only works if ADC IRQ not blocked by others */
//#define ADC_AUTOSCAN

#define ADC_IIR_COEF 6

/*
 * storage for ADC results
 */
static uint8_t ADC_idx, ADC_hyst_state, ADC_param_idx;
static int16_t ADC_hyst_val;
int32_t ADC_iir[ADC_NUMVALS];
volatile int16_t ADC_val[ADC_NUMVALS], ADC_param[ADC_NUMPARAMS];

/*
 * ADC FIFO ISR
 */
void ADC_irq_handler(void)
{
#ifdef ADC_AUTOSCAN
	while(!ADC_fifo_is_empty())
	{
		ADC_iir[ADC_idx] += (((int32_t)(0xfff ^ adc_fifo_get())<<ADC_IIR_COEF)-ADC_iir[ADC_idx])>>ADC_IIR_COEF;
		ADC_val[ADC_idx] = ADC_iir[ADC_idx]>>ADC_IIR_COEF;
		
		if(ADC_idx==0)
		{
			/* update parameters when ADC_idx 0 is ready */
			switch(ADC_hyst_state)
			{
				case 0:	/* reset */
					ADC_hyst_val = ADC_val[0];
					ADC_hyst_state = 1;
					break;
				
				case 1: /* locked & waiting for unlock */
					if(abs(ADC_val[0]-ADC_hyst_val) > 200)
						ADC_hyst_state = 2;
					break;
				
				case 2:	/* tracking */
					ADC_param[ADC_param_idx] = ADC_val[0];
					break;
			}
		}
		
		/* advance channel */
		ADC_idx = (ADC_idx+1)&1;
	}
#else
	/* get previous result and filter */
	ADC_iir[ADC_idx] += (((int32_t)(0xfff ^ adc_fifo_get())<<ADC_IIR_COEF)-ADC_iir[ADC_idx])>>ADC_IIR_COEF;
	ADC_val[ADC_idx] = ADC_iir[ADC_idx]>>ADC_IIR_COEF;
	
	if(ADC_idx==0)
	{
		/* update parameters when ADC_idx 0 is ready */
		switch(ADC_hyst_state)
		{
			case 0:	/* reset */
				ADC_hyst_val = ADC_val[0];
				ADC_hyst_state = 1;
				break;
			
			case 1: /* locked & waiting for unlock */
				if(abs(ADC_val[0]-ADC_hyst_val) > 200)
					ADC_hyst_state = 2;
				break;
			
			case 2:	/* tracking */
				ADC_param[ADC_param_idx] = ADC_val[0];
				break;
		}
	}
		
	/* start next conversion [Why is this needed before advancing chl?] */
	adc_select_input(ADC_idx);
	
	/* advance to next ADC_idx */
	ADC_idx = (ADC_idx+1)&1;
	
	adc_hw->cs |= ADC_CS_START_ONCE_BITS;
#endif
}

/*
 * initialize the ADC
 */
void ADC_init(void)
{
	/* init state */
	ADC_iir[0] = 0;
	ADC_iir[1] = 0;
	ADC_val[0] = 0;
	ADC_val[1] = 0;
	ADC_hyst_state = 0;
	ADC_param_idx = 0;
	ADC_idx = 0;
	
	/* set up ADC for specific pins */
	adc_init();
	adc_gpio_init(26);
	adc_gpio_init(27);

#ifdef ADC_AUTOSCAN
	/* enable FIFO and set IRQ threshold */
	adc_fifo_setup(
		1,	// FIFO enabled
		0,	// no DMA req
		1,	// threshold to IRQ - keep low to avoid losing sync
		0,	// err in FIFO bits
		0	// no byte shift
	);
	
	/* enable IRQ handler for ADC output */
    irq_set_exclusive_handler(ADC_IRQ_FIFO, ADC_irq_handler);
    irq_set_enabled(ADC_IRQ_FIFO, true);
	irq_set_priority(ADC_IRQ_FIFO, 0);
	adc_irq_set_enabled(1);
	
	/* round robin on inputs 0,1 */
	adc_fifo_drain();
	adc_select_input(ADC_idx);
	adc_set_round_robin(3);
	
	/* start free running */
	ADC_run(1);
#else
	/* enable FIFO and set IRQ threshold */
	adc_fifo_setup(
		1,	// FIFO enabled
		0,	// no DMA req
		1,	// threshold to IRQ
		1,	// err in FIFO bits
		0	// no byte shift
	);
	
	/* enable IRQ handler for ADC output */
    irq_set_exclusive_handler(ADC_IRQ_FIFO, ADC_irq_handler);
    irq_set_enabled(ADC_IRQ_FIFO, true);
	adc_irq_set_enabled(1);
	
	/* start one conversion on 1st ADC_idx */
	adc_select_input(ADC_idx);
	
	adc_hw->cs |= ADC_CS_START_ONCE_BITS;
#endif
}


/*
 * set ADC[1] parameter destination
 */
void ADC_setactparam(uint8_t idx)
{
	if(idx>=ADC_NUMPARAMS)
		return;
	
	if(idx != ADC_param_idx)
	{
		/* this has to be atomic */
		uint32_t irqs = save_and_disable_interrupts();
		ADC_param_idx = idx;
		ADC_hyst_state = 0;
		restore_interrupts(irqs);
	}
}

/*
 * set ADC[1] parameter value
 */
void ADC_setparamval(uint8_t idx, int16_t val)
{
	if(idx>=ADC_NUMPARAMS)
		return;
	
	ADC_param[idx] = val;
}

/*
 * force ADC[1] active parameter hysteresis to track live value
 */
void ADC_forceactparam(void)
{
	ADC_hyst_state = 2;
}
