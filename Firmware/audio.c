/*
 * audio.c - G081 audio processing
 * 10-10-2021 E. Brombaugh
 */

#include <string.h>
#include <stdio.h>
#include "hardware/sync.h"
#include "pico/multicore.h"
#include "audio.h"
#include "adc.h"
#include "fx.h"

uint64_t audio_duty, audio_period, audio_start_time, audio_prev_time;
int16_t audio_sl[4], audio_len;
volatile int16_t audio_mute_state, audio_mute_cnt;
int16_t prc[2*BUFSZ];
volatile uint8_t algo_chg_req, *algo_curr, algo_next;
volatile uint8_t mute_chg_req, mute_next;

/*
 * init audio handler
 */
void Audio_Init(void)
{
	/* init fx */
	fx_init();
	
	/* init state */
	audio_sl[0] = audio_sl[1] = audio_sl[2] = audio_sl[3] = 0;
	audio_duty = audio_period = audio_start_time = audio_prev_time = 0;
	audio_mute_state = 2;	// start up  muted
	audio_mute_cnt = 0;
	algo_chg_req = 0;
	mute_chg_req = 0;
}

/*
 * Request Algo change and block until complete - only used by core 0
 */
void Audio_Set_Algo(uint8_t *curr_algo, uint8_t next_algo)
{
	/* wait for algo change to be available */
	//printf("Audio_Set_Algo: next_algo = %d wait for algo_chg_req = %d\n", next_algo, 0);
	while(algo_chg_req){}
	
	/* set up change request */
	algo_curr = curr_algo;
	algo_next = next_algo;
	algo_chg_req = 1;
	
	/* wait for change to propagate */
	//printf("Audio_Set_Algo: next_algo = %d wait for algo_chg_req = %d\n", next_algo, 2);
	while(algo_chg_req != 2){}
	
	/* clear change request */
	algo_chg_req = 0;
}


/*
 * Request mute on/off and block until complete
 */
void Audio_Set_Mute(uint8_t enable)
{
	/* do nothing if already in desired state */
	if(enable == audio_mute_state)
		return;
	
	/* wait for mute change to be available */
	//printf("Audio_Set_Mute: enable = %d wait for mute_chg_req = %d\n", enable, 0);
	while(mute_chg_req){}
	
	/* set up change request */
	mute_next = enable;
	mute_chg_req = 1;
	
	/* wait for change to propagate */
	//printf("Audio_Set_Mute: enable = %d wait for mute_chg_req = %d\n", enable, 2);
	while(mute_chg_req != 2){}
	
	/* clear change request */
	mute_chg_req = 0;
	
	/* block until mute change has finished */
	//printf("Audio_Set_Mute: enable = %d wait for finish (current audio mute_state = %d)\n", enable, audio_mute_state);
	while((audio_mute_state == 1) || (audio_mute_state == 3)){}
}

/*
 * disable core 1
 */
void Audio_Disable_Core(uint8_t disable)
{
	if(disable)
	{
		multicore_lockout_start_timeout_us(1000);
	}
	else
	{
		multicore_lockout_end_timeout_us(1000);
	}
}

/*
 * Audio foreground task - run by core 1
 */
void Audio_Fore(void)
{
	uint32_t irqs;

	/* handle algo change */
	if(algo_chg_req == 1)
	{
		/* update value as requested */
		*algo_curr = algo_next;
		
		/* advance state */
		algo_chg_req = 2;
	}
	
	/* handle mute change */
	if(mute_chg_req == 1)
	{
		if((audio_mute_state == 0) && (mute_next == 1))
		{
			/* mute requested */			
			audio_mute_cnt = 512;
			audio_mute_state = 1;
			
			/* wait for mute to complete (is this needed?) */
			//while(audio_mute_state != 2){}
		}
		else if((audio_mute_state == 2) && (mute_next == 0))
		{
			/* unmute requested */
			audio_mute_cnt = 0;
			audio_mute_state = 3;
			
			/* wait for unmute to complete (is this needed?) */
			//while(audio_mute_state != 0){}
		}
		
		/* ack change req */
		mute_chg_req = 2;
	}
}

/*
 * buffer math:
 * SMPS = 32 (same as stereo "FRAMES")
 * CHLS = 2
 * BUFSZ = SMPS*CHLS = 64
 * FRAMES_PER_BUFFER = BUFSZ/2 = SMPS = 32 (in 32-bit words)
 * length(I2S inbuf) = FRAMES_PER_BUFFER * 2 = 64 total (both halves) but 32-bit words
 * length(src) = length(I2S inbuf/2) = SMPS = 32 in 32-bit words
 * i iterations = SMPS = 32
 * src increments = i iterations * 2 = 64
 */

/*
 * signal level calc
 */
void __not_in_flash_func(level_calc)(int16_t sig, uint16_t *level)
{
	uint16_t rect;

	/* rectify */
	rect = (sig < 0) ? -sig : sig;

	/* peak hold - externally reset */
	if(*level < rect)
		*level = rect;
}

/*
 * handle new buffer of ADC data
 */
void __not_in_flash_func(Audio_Proc)(volatile int16_t *dst, volatile int16_t *src, int32_t len)
{
	uint8_t i;
	int32_t wet, dry, mix;
	
	/* update start time for load calcs */
	audio_prev_time = audio_start_time;
	audio_start_time = time_us_64();
	
	len >>= 1;	// len input is total left + right ints - we need frames
	audio_len = len;
	
	/* check input levels */
	for(i=0;i<len;i++)
	{
		level_calc(src[2*i], &audio_sl[0]);
		level_calc(src[2*i+1], &audio_sl[1]);
	}
	
	/* process the selected algorithm */
	fx_proc(prc, (int16_t *)src, len);
	
	/* set W/D mix gain */	
	wet = ADC_val[1];
	dry = 0xfff - wet;
	
	/* handle output */
	for(i=0;i<len;i++)
	{
		/* W/D with saturation */
		mix = prc[2*i] * wet + src[2*i] * dry;
		dst[2*i] = dsp_ssat16(mix>>12);
		mix = prc[2*i+1] * wet + src[2*i+1] * dry;
		dst[2*i+1] = dsp_ssat16(mix>>12);
		
		/* handle muting */
		switch(audio_mute_state)
		{
			case 0:
				/* pass thru and wait for foreground to force a transition */
				break;
			
			case 1:
				/* transition to mute state */
				mix = (dst[2*i] * audio_mute_cnt);
				dst[2*i] = dsp_ssat16(mix>>9);
				mix = (dst[2*i+1] * audio_mute_cnt);
				dst[2*i+1] = dsp_ssat16(mix>>9);
				audio_mute_cnt--;
				if(audio_mute_cnt == 0)
					audio_mute_state = 2;
				break;
				
			case 2:
				/* mute and wait for foreground to force a transition */
				dst[2*i] = 0;
				dst[2*i+1] = 0;
				break;
			
			case 3:
				/* transition to unmute state */
				mix = (dst[2*i] * audio_mute_cnt);
				dst[2*i] = dsp_ssat16(mix>>9);
				mix = (dst[2*i+1] * audio_mute_cnt);
				dst[2*i+1] = dsp_ssat16(mix>>9);
				audio_mute_cnt++;
				if(audio_mute_cnt == 512)
				{
					audio_mute_state = 0;
					audio_mute_cnt = 0;
				}
				break;
				
			default:
				/* go to legal state */
				audio_mute_state = 0;
				break;
		}
	
		/* check output levels */
		level_calc(dst[2*i], &audio_sl[2]);
		level_calc(dst[2*i+1], &audio_sl[3]);
	}
	
	/* update load calcs */
	audio_period = audio_start_time - audio_prev_time;
	audio_duty = time_us_64() - audio_start_time;
}
