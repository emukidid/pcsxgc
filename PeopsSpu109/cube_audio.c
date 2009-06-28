//cube_audio.c AUDIO output via libOGC

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#include "stdafx.h"
#include "externals.h"
#include "PsxCommon.h"

////////////////////////////////////////////////////////////////////////
// cube audio globals
////////////////////////////////////////////////////////////////////////
#include "../Gamecube/DEBUG.h"
#include <ogc/audio.h>
#include <ogc/cache.h>
#ifdef THREADED_AUDIO
#include <ogc/lwp.h>
#include <ogc/semaphore.h>
#endif
#include <malloc.h>
#include <math.h>


#define NUM_BUFFERS 8
#define BUFFER_SIZE 3840
static char buffer[NUM_BUFFERS][BUFFER_SIZE] __attribute__((aligned(32)));
static int which_buffer = 0;
static unsigned int buffer_offset = 0;
#define NEXT(x) (x=(x+1)%NUM_BUFFERS)
static const float freq_ratio = 44100.0f / 48000.0f;
static const float inv_freq_ratio = 48000.0f / 44100.0f;
static enum { BUFFER_SIZE_48_60 = 3200, BUFFER_SIZE_48_50 = 3840 } buffer_size;

#ifdef THREADED_AUDIO
static lwp_t audio_thread;
static sem_t buffer_full;
static sem_t buffer_empty;
static sem_t audio_free;
static int   thread_running;
#define AUDIO_STACK_SIZE 1024 // MEM: I could get away with a smaller stack
static char  audio_stack[AUDIO_STACK_SIZE];
#define AUDIO_PRIORITY 120
static int   thread_buffer = 0;
#else // !THREADED_AUDIO
#define thread_buffer which_buffer
#endif

static void inline play_buffer(void);
static void done_playing(void);
static void copy_to_buffer_mono(void* out, void* in, unsigned int samples);
static void copy_to_buffer_stereo(void* out, void* in, unsigned int samples);
static void (*copy_to_buffer)(void* out, void* in, unsigned int samples);

////////////////////////////////////////////////////////////////////////
// SETUP SOUND
////////////////////////////////////////////////////////////////////////

void SetupSound(void)
{
  AUDIO_Init(NULL);

#ifdef THREADED_AUDIO
	// Create our semaphores and start/resume the audio thread; reset the buffer index
	LWP_SemInit(&buffer_full, 0, NUM_BUFFERS);
	LWP_SemInit(&buffer_empty, NUM_BUFFERS, NUM_BUFFERS);
	LWP_SemInit(&audio_free, 1, 1);
	thread_running = 1;
	LWP_CreateThread(&audio_thread, (void*)play_buffer, NULL, audio_stack, AUDIO_STACK_SIZE, AUDIO_PRIORITY);
	AUDIO_RegisterDMACallback(done_playing);
	thread_buffer = which_buffer = 0;
#endif

	buffer_size = Config.PsxType ? BUFFER_SIZE_48_50 : BUFFER_SIZE_48_60;
	AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);
	copy_to_buffer = iDisStereo ? copy_to_buffer_mono : copy_to_buffer_stereo;
	//printf("SetupSound\n");
}

////////////////////////////////////////////////////////////////////////
// REMOVE SOUND
////////////////////////////////////////////////////////////////////////

void RemoveSound(void)
{
 AUDIO_StopDMA();

#ifdef THREADED_AUDIO
	// Destroy semaphores and suspend the thread so audio can't play
	thread_running = 0;
	LWP_SemDestroy(buffer_full);
	LWP_SemDestroy(buffer_empty);
	LWP_SemDestroy(audio_free);
	LWP_JoinThread(audio_thread, NULL);
#endif

 //DEBUG_print("RemoveSound called",12);
}

////////////////////////////////////////////////////////////////////////
// GET BYTES BUFFERED
////////////////////////////////////////////////////////////////////////

unsigned long SoundGetBytesBuffered(void)
{
 	/*sprintf(txtbuffer,"SoundGetBytesBuffered returns approx: %d bytes",
 	        buffer_size - AUDIO_GetDMABytesLeft());
 	DEBUG_print(txtbuffer,12);*/
  return buffer_size - AUDIO_GetDMABytesLeft();
}

#ifdef THREADED_AUDIO
static void done_playing(void){
	// We're done playing, so we're releasing a buffer and the audio
	LWP_SemPost(buffer_empty);
	LWP_SemPost(audio_free);
}
#endif

static void inline play_buffer(void){
#ifndef THREADED_AUDIO
	// We should wait for the other buffer to finish its DMA transfer first
	while( AUDIO_GetDMABytesLeft() );
	AUDIO_StopDMA();

#else // THREADED_AUDIO
	// This thread will keep giving buffers to the audio as they come
	while(thread_running){

	// Wait for a buffer to be processed
	LWP_SemWait(buffer_full);
#endif

	// Make sure the buffer is in RAM, not the cache
	DCFlushRange(buffer[thread_buffer], buffer_size);

	// Actually send the buffer out to be played next
	AUDIO_InitDMA((unsigned int)&buffer[thread_buffer], buffer_size);

#ifdef THREADED_AUDIO
	// Wait for the audio interface to be free before playing
	LWP_SemWait(audio_free);
#endif

	// Start playing the buffer
	AUDIO_StartDMA();

#ifdef THREADED_AUDIO
	// Move the index to the next buffer
	NEXT(thread_buffer);
	}
#endif
}

static void copy_to_buffer_mono(void* b, void* s, unsigned int length){
	// NOTE: length is in resampled samples (mono (1) short)
	short* buffer = b, * stream = s;
	int di;
	float si;
	for(di = 0, si = 0.0f; di < length; ++di, si += freq_ratio){
		// Linear interpolation between current and next sample
		float t = si - floorf(si);
		buffer[di] = (1.0f - t)*stream[(int)si] + t*stream[(int)ceilf(si)];
	}
}

static void copy_to_buffer_stereo(void* b, void* s, unsigned int length){
	// NOTE: length is in resampled samples (stereo (2) shorts)
	int* buffer = b, * stream = s;
	int di;
	float si;
	for(di = 0, si = 0.0f; di < length; ++di, si += freq_ratio){
#if 1
		// Linear interpolation between current and next sample
		float t = si - floorf(si);
		short* osample  = (short*)(buffer + di);
		short* isample1 = (short*)(stream + (int)si);
		short* isample2 = (short*)(stream + (int)ceilf(si));
		// Left and right
		osample[0] = (1.0f - t)*isample1[0] + t*isample2[0];
		osample[1] = (1.0f - t)*isample1[1] + t*isample2[1];
#else
		// Quick and dirty resampling: skip over or repeat samples
		buffer[di] = stream[(int)si];
#endif
	}
}

static void inline add_to_buffer(void* stream, unsigned int length){
	// This shouldn't lose any data and works for any size
	unsigned int stream_offset = 0;
	// Length calculations are in samples
	//   (Either pairs of shorts (stereo) or shorts (mono))
	const int shift = iDisStereo ? 1 : 2;
	unsigned int lengthi, rlengthi;
	unsigned int lengthLeft = length >> shift;
	unsigned int rlengthLeft = ceilf(lengthLeft * inv_freq_ratio);

	while(rlengthLeft > 0){
		rlengthi = (buffer_offset + (rlengthLeft << shift) <= buffer_size) ?
		            rlengthLeft : ((buffer_size - buffer_offset) >> shift);
		lengthi  = rlengthi * freq_ratio;

#ifdef THREADED_AUDIO
		// Wait for a buffer we can copy into
		LWP_SemWait(buffer_empty);
#endif
		copy_to_buffer(buffer[which_buffer] + buffer_offset,
		               stream + stream_offset, rlengthi);

		if(buffer_offset + (rlengthLeft << shift) < buffer_size){
			buffer_offset += rlengthi << shift;
#ifdef THREADED_AUDIO
			// This is a little weird, but we didn't fill this buffer.
			//   So it is still considered 'empty', but since we 'decremented'
			//   buffer_empty coming in here, we want to set it back how
			//   it was, so we don't cause a deadlock
			LWP_SemPost(buffer_empty);
#endif
			return;
		}

		lengthLeft    -= lengthi;
		stream_offset += lengthi << shift;
		rlengthLeft   -= rlengthi;

#ifdef THREADED_AUDIO
		// Let the audio thread know that we've filled a new buffer
		LWP_SemPost(buffer_full);
#else
		play_buffer();
#endif

		NEXT(which_buffer);
		buffer_offset = 0;
	}
}

////////////////////////////////////////////////////////////////////////
// FEED SOUND DATA
////////////////////////////////////////////////////////////////////////
void SoundFeedStreamData(unsigned char* pSound,long lBytes)
{
	add_to_buffer(pSound, lBytes);
}
