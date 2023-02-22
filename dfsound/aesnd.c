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
#include "out.h"
#include "spu_config.h"
#include "../GameCube/wiisxConfig.h"

char audioEnabled;
static AESNDPB* voice = NULL;

#define NUM_BUFFERS 4
static struct { void* buffer; u32 len;} buffers[NUM_BUFFERS];
static u32 fill_buffer, play_buffer;

static void aesnd_callback(AESNDPB* voice, u32 state){
	if(state == VOICE_STATE_STREAM) {
		if(play_buffer != fill_buffer) {
			AESND_SetVoiceBuffer(voice,
					buffers[play_buffer].buffer, buffers[play_buffer].len);
			play_buffer = (play_buffer + 1) % NUM_BUFFERS;
		}
	}
}

void SetVolume(void)
{
	u16 _volume = (4 - volume + 1) * 64 - 1;
	if (voice) AESND_SetVoiceVolume(voice, 255, 255);
}

static int aesnd_init(void) {
	voice = AESND_AllocateVoice(aesnd_callback);
	AESND_SetVoiceFormat(voice, VOICE_STEREO16);
	AESND_SetVoiceFrequency(voice, 44100);
	SetVolume();
	AESND_SetVoiceStream(voice, true);
	fill_buffer = play_buffer = 0;
	return 0;
}

static void aesnd_finish(void) {
	AESND_SetVoiceStop(voice, true);
}

static int aesnd_busy(void) {
	unsigned long bytes_buffered = 0, i = fill_buffer;
	while(1) {
		bytes_buffered += buffers[i].len;
		
		if(i == play_buffer) break;
		i = (i + NUM_BUFFERS - 1) % NUM_BUFFERS;
	}
	
	return bytes_buffered;
}

static void aesnd_feed(void *pSound, int lBytes) {
	if(!audioEnabled) return;
	
	buffers[fill_buffer].buffer = pSound;
	buffers[fill_buffer].len = lBytes;
	fill_buffer = (fill_buffer + 1) % NUM_BUFFERS;
	
	AESND_SetVoiceStop(voice, false);
}

void out_register_aesnd(struct out_driver *drv)
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
