/*
 * audio.h - G081 audio processing
 * 10-10-2021 E. Brombaugh
 */

#ifndef __audio__
#define __audio__

#include "main.h"

#define SMPS 32
#define CHLS 2
#define BUFSZ (SMPS*CHLS)

extern int16_t audio_sl[4], audio_len;
extern uint64_t audio_duty, audio_period;

void Audio_Init(void);
void Audio_Set_Algo(uint8_t *curr_algo, uint8_t next_algo);
void Audio_Set_Mute(uint8_t enable);
void Audio_Disable_Core(uint8_t disable);
void Audio_Fore(void);
void Audio_Proc(volatile int16_t *dst, volatile int16_t *src, int32_t sz);

#endif

