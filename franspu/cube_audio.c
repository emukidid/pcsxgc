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
/*
#include "stdafx.h"
#include "externals.h"*/
#include "franspu.h"
#include "../PsxCommon.h"

////////////////////////////////////////////////////////////////////////
// cube audio globals
////////////////////////////////////////////////////////////////////////
#include "../Gamecube/DEBUG.h"
#include <aesndlib.h>


char audioEnabled;

static const u32 freq = 44100;
static AESNDPB* voice = NULL;
s32 audio_volume[2] = { 128, 128 };

#define NUM_BUFFERS 4
static struct { void* buffer; u32 len; } buffers[NUM_BUFFERS];
static u32 fill_buffer, play_buffer;
#define SILENCE_SIZE 896
static unsigned char silence[SILENCE_SIZE];

static void aesnd_callback(AESNDPB* voice, u32 state);

////////////////////////////////////////////////////////////////////////
// SETUP SOUND
////////////////////////////////////////////////////////////////////////

void SetupSound(void)
{
	voice = AESND_AllocateVoice(aesnd_callback);
	AESND_SetVoiceFormat(voice, VOICE_STEREO16);
	AESND_SetVoiceFrequency(voice, freq);
	AESND_SetVoiceVolume(voice, audio_volume[0], audio_volume[1]);
	AESND_SetVoiceStream(voice, true);
}

////////////////////////////////////////////////////////////////////////
// REMOVE SOUND
////////////////////////////////////////////////////////////////////////

void RemoveSound(void)
{
	AESND_SetVoiceStop(voice, true);
}

////////////////////////////////////////////////////////////////////////
// GET BYTES BUFFERED
////////////////////////////////////////////////////////////////////////

unsigned long SoundGetBytesBuffered(void)
{
	unsigned long bytes_buffered = 0, i = fill_buffer;
	while(1) {
		bytes_buffered += buffers[i].len;
		
		if(i == play_buffer) break;
		i = (i + NUM_BUFFERS - 1) % NUM_BUFFERS;
	}
	
	return bytes_buffered;
}

static void aesnd_callback(AESNDPB* voice, u32 state){
	if(state == VOICE_STATE_STREAM) {
		if(play_buffer != fill_buffer) {
			AESND_SetVoiceBuffer(voice,
					buffers[play_buffer].buffer, buffers[play_buffer].len);
			play_buffer = (play_buffer + 1) % NUM_BUFFERS;
		} else {
			AESND_SetVoiceBuffer(voice, silence, SILENCE_SIZE);
		}
	}
}

////////////////////////////////////////////////////////////////////////
// FEED SOUND DATA
////////////////////////////////////////////////////////////////////////
void SoundFeedStreamData(unsigned char* pSound,long lBytes)
{
	if(!audioEnabled) return;
	
	buffers[fill_buffer].buffer = pSound;
	buffers[fill_buffer].len = lBytes;
	fill_buffer = (fill_buffer + 1) % NUM_BUFFERS;
	
	AESND_SetVoiceStop(voice, false);
}

void pauseAudio(void){
	AESND_Pause(true);
}

void resumeAudio(void){
	AESND_Pause(false);
}
