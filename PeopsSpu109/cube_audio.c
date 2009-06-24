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

////////////////////////////////////////////////////////////////////////
// cube audio globals
////////////////////////////////////////////////////////////////////////
#include "../Gamecube/DEBUG.h"
#include <ogc/audio.h>
#include <gccore.h>
#include <malloc.h>

////////////////////////////////////////////////////////////////////////
// SETUP SOUND
////////////////////////////////////////////////////////////////////////

void SetupSound(void)
{
  AUDIO_Init(NULL);
  //AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);
  //DEBUG_print("SetupSound called",12);
}

////////////////////////////////////////////////////////////////////////
// REMOVE SOUND
////////////////////////////////////////////////////////////////////////

void RemoveSound(void)
{
 AUDIO_StopDMA();
 //DEBUG_print("RemoveSound called",12);
}

////////////////////////////////////////////////////////////////////////
// GET BYTES BUFFERED
////////////////////////////////////////////////////////////////////////

unsigned long SoundGetBytesBuffered(void)
{
 	//sprintf(txtbuffer,"SoundGetBytesBuffered returns approx: %d bytes",AUDIO_GetDMABytesLeft());
 	//DEBUG_print(txtbuffer,12);
  return AUDIO_GetDMABytesLeft();
}

////////////////////////////////////////////////////////////////////////
// FEED SOUND DATA
////////////////////////////////////////////////////////////////////////
unsigned char *audio_buffer = NULL;
void SoundFeedStreamData(unsigned char* pSound,long lBytes)
{
  // We should wait for the other buffer to finish its DMA transfer first
	//while( AUDIO_GetDMABytesLeft() );       //slow
	AUDIO_StopDMA();
  
	if(audio_buffer)
	{
	  free(audio_buffer);
	  audio_buffer = NULL;
  }
	lBytes-=lBytes%32;
  audio_buffer = (unsigned char*)memalign(32,lBytes);
  
  memcpy(audio_buffer,pSound,lBytes);
   
	// Make sure the buffer is in RAM, not the cache
	DCFlushRange(audio_buffer,lBytes);
	
	// Actually send the buffer out to be played
	AUDIO_InitDMA((unsigned int)audio_buffer, lBytes);
	AUDIO_StartDMA();
	//sprintf(txtbuffer,"SoundFeedStreamData length: %ld bytes",lBytes);
	//DEBUG_print(txtbuffer,13);
	
}
