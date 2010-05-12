#if 0
#include "spu_callback.h"
#include <minimal.h>

extern "C" void app_MuteSound(void);
extern "C" void app_DemuteSound(void);

/* Sound Thread*/
static void *gp2x_sound_thread_play(void *none)
{
#ifndef NOSOUND
	app_DemuteSound();
#endif
	return NULL;
}

/* Mute Sound Thread*/
void gp2x_sound_thread_mute(void)
{
#ifndef NOSOUND
	app_MuteSound();
#endif
}

#define SOUNDQUEUE_SIZE 64000

unsigned char sound_queue[SOUNDQUEUE_SIZE];
int head = 0, tail = 0;

unsigned char* SPU_async_X(int nsamples);

extern "C" void sound_callback(void *userdata, u8 *stream, int len)
{
	//if( global_enable_audio )
	{
		/*if(tail > head)
		{
		    memcpy(stream, sound_queue+head, len);
		}
		else
		{
		    if(head+len<SOUNDQUEUE_SIZE)
		    memcpy(stream, sound_queue+head, len);
		else
		{
		    memcpy(stream, sound_queue+head, SOUNDQUEUE_SIZE-head);
		    memcpy(stream+(SOUNDQUEUE_SIZE-head), sound_queue, len-(SOUNDQUEUE_SIZE-head));
		}
		}

		head = (head + len) % SOUNDQUEUE_SIZE;*/

		memcpy(stream, SPU_async_X(len/2), len);
	}
}

/* Start Sound Core */
void SetupSound(void)
{
#ifndef NOSOUND
	if( iSoundMuted != 0 ) return;
 	gp2x_sound_thread_mute();
 	gp2x_sound_thread_play(NULL);
#endif
}

/* Stop Sound Core */
void RemoveSound(void)
{
#ifndef NOSOUND
	if( iSoundMuted != 0 ) return;
 	gp2x_sound_thread_mute();
#endif
}

/* Feed Sound Data */
void SoundFeedStreamData(unsigned char* pSound,long lBytes)
{
#ifndef NOSOUND
    int new_tail = tail + lBytes;

    if(tail < head && new_tail > head) new_tail = head;

    if(new_tail < SOUNDQUEUE_SIZE)
    {
        memcpy(sound_queue+tail, pSound, lBytes);
    }
    else
    {
        new_tail %= SOUNDQUEUE_SIZE;

        memcpy(sound_queue+tail, pSound, SOUNDQUEUE_SIZE-tail);
        memcpy(sound_queue, pSound+(SOUNDQUEUE_SIZE-tail), new_tail);
    }

    tail = new_tail;

 	/*
 	if(oss_audio_fd == -1) return;
	int nbuff=gp2x_sound_buffer; nbuff++; if (nbuff==DEFAULT_SAMPLE_NUM_BUFF) nbuff=0;	// Number of the sound buffer to write
	memcpy(&gp2x_sound_buffers[32768*nbuff],pSound,lBytes);					// Write the sound buffer
	sndlen=lBytes;										// Update the sound buffer length
	gp2x_sound_buffer=nbuff;								// Update the current sound buffer
	*/
#endif
}
#endif