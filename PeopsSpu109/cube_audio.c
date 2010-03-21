//cube_audio.c ASND output via libOGC

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


char audioEnabled;

static const s32 freq = 44100;
static s32 voice = SND_INVALID, first_sample = TRUE;
s32 audio_volume[2] = { MID_VOLUME, MID_VOLUME };

static void dummy_cb(s32 voice) { }

////////////////////////////////////////////////////////////////////////
// SETUP SOUND
////////////////////////////////////////////////////////////////////////

void SetupSound(void)
{
	voice = ASND_GetFirstUnusedVoice();
	first_sample = TRUE;
}

////////////////////////////////////////////////////////////////////////
// REMOVE SOUND
////////////////////////////////////////////////////////////////////////

void RemoveSound(void)
{
	ASND_StopVoice(voice);
}

////////////////////////////////////////////////////////////////////////
// GET BYTES BUFFERED
////////////////////////////////////////////////////////////////////////

unsigned long SoundGetBytesBuffered(void)
{
	// FIXME: I don't think this is accurate at all
	return ASND_TestVoiceBufferReady(voice) ? 0 : ASND_GetSamplesPerTick();
}

////////////////////////////////////////////////////////////////////////
// FEED SOUND DATA
////////////////////////////////////////////////////////////////////////
void SoundFeedStreamData(unsigned char* pSound,long lBytes)
{
	if(!audioEnabled) return;
	
	// FIXME: Ensure pSound is aligned (32B alignment and length)
	
	if(first_sample) {
		// Set up the audio stream for the first time
		ASND_SetVoice(voice,
		              iDisStereo ? VOICE_MONO_16BIT : VOICE_STEREO_16BIT,
		              freq, 0, pSound, lBytes,
		              audio_volume[0], audio_volume[1], dummy_cb);
		ASND_Pause(0);
		first_sample = FALSE;
	} else {
		// Spin on adding the audio to the voice
		while(SND_BUSY == ASND_AddVoice(voice, pSound, lBytes));
	}
}

void pauseAudio(void){
	ASND_Pause(1);
}

void resumeAudio(void){
	ASND_Pause(0);
}
