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
#include "../PsxCommon.h"

////////////////////////////////////////////////////////////////////////
// cube audio globals
////////////////////////////////////////////////////////////////////////
#include "../Gamecube/DEBUG.h"
#include <asndlib.h>
#ifdef THREADED_AUDIO
#include <ogc/lwp.h>
#include <ogc/semaphore.h>
#endif
#include <malloc.h>


#define NUM_BUFFERS 16
#define BUFFER_SIZE (4*1024)
static char buffer[NUM_BUFFERS][BUFFER_SIZE] __attribute__((aligned(32)));
static int which_buffer = 0;
static unsigned int buffer_offset = 0;
#define NEXT(x) (x=(x+1)%NUM_BUFFERS)

#ifdef THREADED_AUDIO
static lwp_t audio_thread;
static sem_t buffer_full;
static sem_t buffer_empty;
static sem_t audio_free;
static sem_t first_audio;
static int   thread_running = 0;
#define AUDIO_STACK_SIZE 1024 // MEM: I could get away with a smaller stack
static char  audio_stack[AUDIO_STACK_SIZE];
#define AUDIO_PRIORITY 120
static int   thread_buffer = 0;
static int   audio_paused = 0;
#else // !THREADED_AUDIO
#define thread_buffer which_buffer
#endif

char audioEnabled;

static const s32 freq = 44100;
static s32 voice = SND_INVALID, first_sample = TRUE;
s32 audio_volume[2] = { MID_VOLUME, MID_VOLUME };

static void done_playing(void);
static void play_buffer(void);
static void add_to_buffer(void* stream, unsigned int length);
static void dummy_callback(s32 voice){ }

////////////////////////////////////////////////////////////////////////
// SETUP SOUND
////////////////////////////////////////////////////////////////////////

void SetupSound(void)
{
	voice = ASND_GetFirstUnusedVoice();
	first_sample = TRUE;
	
#ifdef THREADED_AUDIO
	// Create our semaphores and start/resume the audio thread; reset the buffer index
	LWP_SemInit(&buffer_full, 0, NUM_BUFFERS);
	LWP_SemInit(&buffer_empty, NUM_BUFFERS, NUM_BUFFERS);
	LWP_SemInit(&audio_free, 1, 1);
	LWP_SemInit(&first_audio, 0, 1);
	thread_running = 0;
	LWP_CreateThread(&audio_thread, (void*)play_buffer, NULL, audio_stack, AUDIO_STACK_SIZE, AUDIO_PRIORITY);
	thread_buffer = which_buffer = 0;
#endif
}

////////////////////////////////////////////////////////////////////////
// REMOVE SOUND
////////////////////////////////////////////////////////////////////////

void RemoveSound(void)
{
	ASND_StopVoice(voice);
	
#ifdef THREADED_AUDIO
	// Destroy semaphores and suspend the thread so audio can't play
	if(!thread_running) LWP_SemPost(first_audio);
	thread_running = 0;
	LWP_SemDestroy(buffer_full);
	LWP_SemDestroy(buffer_empty);
	LWP_SemDestroy(audio_free);
	LWP_SemDestroy(first_audio);
	LWP_JoinThread(audio_thread, NULL);
#endif
}

////////////////////////////////////////////////////////////////////////
// GET BYTES BUFFERED
////////////////////////////////////////////////////////////////////////

unsigned long SoundGetBytesBuffered(void)
{
	// FIXME: I don't think this is accurate at all
	unsigned int buffered = ((which_buffer - thread_buffer + NUM_BUFFERS) % NUM_BUFFERS) * BUFFER_SIZE;
	return buffered + (ASND_TestVoiceBufferReady(voice) ? 0 : ASND_GetSamplesPerTick());
}

#ifdef THREADED_AUDIO
static void done_playing(void){
	// We're done playing, so we're releasing a buffer and the audio
	LWP_SemPost(buffer_empty);
	LWP_SemPost(audio_free);
}
#endif

static void play_buffer(void){
#ifdef THREADED_AUDIO
	// Wait for a sample to actually be played to work around a deadlock
	LWP_SemWait(first_audio);
	
	// This thread will keep giving buffers to the audio as they come
	while(thread_running){

	// Wait for a buffer to be processed
	LWP_SemWait(buffer_full);
	
	// Wait for the audio interface to be free before playing
	LWP_SemWait(audio_free);
#endif

	// Start playing the buffer
	if(first_sample){
		// Set up the audio stream for the first time
		ASND_SetVoice(voice,
					  iDisStereo ? VOICE_MONO_16BIT : VOICE_STEREO_16BIT,
					  freq, 0, buffer[thread_buffer], BUFFER_SIZE,
					  audio_volume[0], audio_volume[1],
#ifdef THREADED_AUDIO
					  done_playing
#else
					  dummy_callback
#endif
					  );
		ASND_Pause(0);
		first_sample = FALSE;
	} else {
#ifndef THREADED_AUDIO
		while(
#endif
		// Add more audio to the stream
		ASND_AddVoice(voice, buffer[thread_buffer], BUFFER_SIZE)
#ifndef THREADED_AUDIO
		== SND_BUSY)
#endif
		;
	}

#ifdef THREADED_AUDIO
	// Move the index to the next buffer
	NEXT(thread_buffer);
	}
#endif
}

////////////////////////////////////////////////////////////////////////
// FEED SOUND DATA
////////////////////////////////////////////////////////////////////////
void SoundFeedStreamData(unsigned char* pSound,long lBytes)
{
	if(!audioEnabled) return;
	
	add_to_buffer(pSound, lBytes);
}

void pauseAudio(void){
	ASND_Pause(1);
}

void resumeAudio(void){
	ASND_Pause(0);
}

static void add_to_buffer(void* stream, unsigned int length){
	// This shouldn't lose any data and works for any size
	unsigned int stream_offset = 0;
	unsigned int lengthi;
	unsigned int lengthLeft = length;

	while(lengthLeft > 0){
		lengthi = (buffer_offset + lengthLeft <= BUFFER_SIZE) ?
		            lengthLeft : (BUFFER_SIZE - buffer_offset);

#ifdef THREADED_AUDIO
		// Wait for a buffer we can copy into
		LWP_SemWait(buffer_empty);
#endif
		memcpy(buffer[which_buffer] + buffer_offset,
		       stream + stream_offset, lengthi);

		if(buffer_offset + lengthLeft < BUFFER_SIZE){
			buffer_offset += lengthi;
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
		stream_offset += lengthi;

#ifdef THREADED_AUDIO
		// Start the thread if this is the first sample
		if(!thread_running){
			thread_running = 1;
			LWP_SemPost(first_audio);
		}
		// Let the audio thread know that we've filled a new buffer
		LWP_SemPost(buffer_full);
#else
		play_buffer();
#endif

		NEXT(which_buffer);
		buffer_offset = 0;
	}
}

