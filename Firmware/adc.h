/*
 * ADC.h - background ADC management
 */

#ifndef __ADC__
#define __ADC__

#include "main.h"

#define ADC_NUMVALS 2
#define ADC_NUMPARAMS 4

extern volatile int16_t ADC_val[ADC_NUMVALS], ADC_param[ADC_NUMPARAMS];

void ADC_init(void);
void ADC_setactparam(uint8_t idx);
void ADC_setparamval(uint8_t idx, int16_t val);
void ADC_forceactparam(void);

#endif