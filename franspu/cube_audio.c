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

#include "franspu.h"
#include "../PsxCommon.h"

////////////////////////////////////////////////////////////////////////
// cube audio globals
////////////////////////////////////////////////////////////////////////
#include "../Gamecube/DEBUG.h"
#if defined(__SWITCH__)
#include <malloc.h>
#else
#include <aesndlib.h>
#endif


char audioEnabled;
extern unsigned int iVolume; 

#if defined(__SWITCH__)

#define AUDIO_SAMPLERATE 48000
#define AUDIO_BUFFER_COUNT 6
#define AUDIO_BUFFER_SAMPLES (AUDIO_SAMPLERATE / 20)
#define AUDIO_TRANSFERBUF_SIZE (AUDIO_BUFFER_SAMPLES * 20)

static int audioTransferBufferUsed = 0;
static u32 *audioTransferBuffer;
static Mutex audioLock;
Thread audio_thread;
void audio_thread_main(void *x);
#else

#define NUM_BUFFERS 4
static const u32 freq = 44100;
static struct { void* buffer; u32 len; } buffers[NUM_BUFFERS];
static u32 fill_buffer, play_buffer;
static AESNDPB* voice = NULL;
static void aesnd_callback(AESNDPB* voice, u32 state);
#endif


void SetVolume(void)
{
	// iVolume goes 1 (loudest) - 4 (lowest); volume goes 255-64
#if defined(__SWITCH__)
#else	
	u16 volume = (4 - iVolume + 1) * 64 - 1;
	if (voice) AESND_SetVoiceVolume(voice, volume, volume);
#endif
}

////////////////////////////////////////////////////////////////////////
// SETUP SOUND
////////////////////////////////////////////////////////////////////////

void SetupSound(void)
{
#if defined(__SWITCH__)
	audioTransferBuffer = (u32 *)malloc(AUDIO_TRANSFERBUF_SIZE * sizeof(u32));
	mutexInit(&audioLock);

	audoutInitialize();
	audoutStartAudioOut();

	threadCreate(&audio_thread, audio_thread_main, NULL, 0x10000, 0x2B, 2);
	threadStart(&audio_thread);
#else
	voice = AESND_AllocateVoice(aesnd_callback);
	AESND_SetVoiceFormat(voice, iDisStereo ? VOICE_MONO16 : VOICE_STEREO16);
	AESND_SetVoiceFrequency(voice, freq);
	SetVolume();
	AESND_SetVoiceStream(voice, true);
	fill_buffer = play_buffer = 0;
#endif
}

////////////////////////////////////////////////////////////////////////
// REMOVE SOUND
////////////////////////////////////////////////////////////////////////

void RemoveSound(void)
{
#if defined(__SWITCH__)
	threadWaitForExit(&audio_thread);
	threadClose(&audio_thread);

	audoutStopAudioOut();
	audoutExit();

	free(audioTransferBuffer);
#else
	AESND_SetVoiceStop(voice, true);
#endif
}

////////////////////////////////////////////////////////////////////////
// GET BYTES BUFFERED
////////////////////////////////////////////////////////////////////////

u32 SoundGetBytesBuffered(void)
{
#if defined(__SWITCH__)
	return audioTransferBufferUsed;
#else
	u32 bytes_buffered = 0, i = fill_buffer;
	while(1) {
		bytes_buffered += buffers[i].len;
		
		if(i == play_buffer) break;
		i = (i + NUM_BUFFERS - 1) % NUM_BUFFERS;
	}
	
	return bytes_buffered;
#endif
}

#if defined(__SWITCH__)
void audio_thread_main(void *x) {
	AudioOutBuffer sources[2];

	u32 raw_data_size = (AUDIO_BUFFER_SAMPLES * sizeof(u32) + 0xfff) & ~0xfff;
	u32 *raw_data[2];
	for (int i = 0; i < 2; i++) {
		raw_data[i] = memalign(0x1000, raw_data_size);
		memset(raw_data[i], 0, raw_data_size);

		sources[i].next = 0;
		sources[i].buffer = raw_data[i];
		sources[i].buffer_size = raw_data_size;
		sources[i].data_size = AUDIO_BUFFER_SAMPLES * sizeof(u32);
		sources[i].data_offset = 0;

		audoutAppendAudioOutBuffer(&sources[i]);
	}

	while (1) {
		u32 count;
		AudioOutBuffer *released;
		audoutWaitPlayFinish(&released, &count, U64_MAX);

		mutexLock(&audioLock);

		u32 size = (audioTransferBufferUsed < AUDIO_BUFFER_SAMPLES ? audioTransferBufferUsed : AUDIO_BUFFER_SAMPLES) * sizeof(u32);
		memcpy(released->buffer, audioTransferBuffer, size);
		released->data_size = size == 0 ? AUDIO_BUFFER_SAMPLES * sizeof(u32) : size;

		audioTransferBufferUsed -= size / sizeof(u32);
		memmove(audioTransferBuffer, audioTransferBuffer + (size / sizeof(u32)), audioTransferBufferUsed * sizeof(u32));

		mutexUnlock(&audioLock);

		audoutAppendAudioOutBuffer(released);
	}

	free(raw_data[0]);
	free(raw_data[1]);
}
#else
static void aesnd_callback(AESNDPB* voice, u32 state){
	if(state == VOICE_STATE_STREAM) {
		if(play_buffer != fill_buffer) {
			AESND_SetVoiceBuffer(voice,
					buffers[play_buffer].buffer, buffers[play_buffer].len);
			play_buffer = (play_buffer + 1) % NUM_BUFFERS;
		}
	}
}
#endif

////////////////////////////////////////////////////////////////////////
// FEED SOUND DATA
////////////////////////////////////////////////////////////////////////
void SoundFeedStreamData(unsigned char* pSound,s32 lBytes)
{
	if(!audioEnabled) return;
	
#if defined(__SWITCH__)
	mutexLock(&audioLock);
	if (audioTransferBufferUsed + lBytes >= AUDIO_TRANSFERBUF_SIZE) {
		mutexUnlock(&audioLock);
		return;
	}

	memcpy(audioTransferBuffer + audioTransferBufferUsed, pSound, lBytes);

	audioTransferBufferUsed += lBytes;
	mutexUnlock(&audioLock);
#else	
	buffers[fill_buffer].buffer = pSound;
	buffers[fill_buffer].len = lBytes;
	fill_buffer = (fill_buffer + 1) % NUM_BUFFERS;
	AESND_SetVoiceStop(voice, false);
#endif
}

void pauseAudio(void){
#if defined(__SWITCH__)
	mutexLock(&audioLock);
	memset(audioTransferBuffer, 0, AUDIO_TRANSFERBUF_SIZE * sizeof(u32));
	audioTransferBufferUsed = AUDIO_TRANSFERBUF_SIZE;
	mutexUnlock(&audioLock);
#else
	AESND_Pause(true);
#endif
}

void resumeAudio(void){
#if defined(__SWITCH__)

#else
	AESND_Pause(false);
#endif
}
