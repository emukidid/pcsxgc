/* SDL Driver for P.E.Op.S Sound Plugin
 * Copyright (c) 2010, Wei Mingzhi <whistler_wmz@users.sf.net>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA
 */

#include <stdlib.h>
#include <aesndlib.h>
#include <string.h>

#include "../pcsx_rearmed/plugins/dfsound/out.h"
#include "../pcsx_rearmed/plugins/dfsound/spu_config.h"
#include "wiiSXconfig.h"

char audioEnabled;
static AESNDPB* voice = NULL;

#define NUM_BUFFERS 4
#define BUFFER_SIZE 8*DSP_STREAMBUFFER_SIZE
char buffers[NUM_BUFFERS][BUFFER_SIZE];
int playBuffer = 0;
int fillBuffer = 0;
int fillBufferOffset[NUM_BUFFERS];
int bytesBuffered = 0;

static void aesnd_callback(AESNDPB* voice, u32 state){
	if(state == VOICE_STATE_STREAM) {
		if(playBuffer != fillBuffer) {
			if(fillBufferOffset[playBuffer] == BUFFER_SIZE) {
				AESND_SetVoiceBuffer(voice,
						buffers[playBuffer], BUFFER_SIZE);
				playBuffer = (playBuffer + 1) % NUM_BUFFERS;
				bytesBuffered -= BUFFER_SIZE;
			}
		}
	}
}

void SetVolume(void)
{
	u16 aesnd_vol = (u16)(((float)(spu_config.iVolume / 1024.0f)) * 255);
	if (voice) AESND_SetVoiceVolume(voice, aesnd_vol, aesnd_vol);
}

static int aesnd_init(void) {
	voice = AESND_AllocateVoice(aesnd_callback);
	AESND_SetVoiceFormat(voice, VOICE_STEREO16);
	AESND_SetVoiceFrequency(voice, 44100);
	SetVolume();
	AESND_SetVoiceStream(voice, true);
	fillBuffer = playBuffer = 0;
	return 0;
}

static void aesnd_finish(void) {
	AESND_SetVoiceStop(voice, true);
}

static int aesnd_busy(void) {
	return bytesBuffered;
}

static void aesnd_feed(void *pSound, int lBytes) {
	if(!audioEnabled) return;
	
    char *soundData = (char *)pSound;
    int bytesRemaining = lBytes;

    while (bytesRemaining > 0) {
        // Compute how many bytes to copy into the fillBuffer
        int bytesToCopy = BUFFER_SIZE - fillBufferOffset[fillBuffer];
        if (bytesToCopy > bytesRemaining) {
            bytesToCopy = bytesRemaining;
        }

        // Copy the sound data into the fillBuffer
        memcpy(&buffers[fillBuffer][fillBufferOffset[fillBuffer]], soundData, bytesToCopy);
        soundData += bytesToCopy;
        bytesRemaining -= bytesToCopy;
        fillBufferOffset[fillBuffer] += bytesToCopy;
        bytesBuffered += bytesToCopy;

        // If the fillBuffer is full, advance to the next fillBuffer
        if (fillBufferOffset[fillBuffer] == BUFFER_SIZE) {
            fillBuffer = (fillBuffer + 1) % NUM_BUFFERS;
            fillBufferOffset[fillBuffer] = 0;
        }
    }
	
	AESND_SetVoiceStop(voice, false);
}

void out_register_oss(struct out_driver *drv)
{
	drv->name = "aesnd";
	drv->init = aesnd_init;
	drv->finish = aesnd_finish;
	drv->busy = aesnd_busy;
	drv->feed = aesnd_feed;
}

void pauseAudio(void){
	AESND_Pause(true);
}

void resumeAudio(void){
	AESND_Pause(false);
}
