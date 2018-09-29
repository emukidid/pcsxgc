/**
 * Mupen64 - profile.c
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 * 
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

#include "sys/time.h"
#include "DEBUG.h"
#include <stdio.h>

#ifdef PROFILE

char * profile_strings[] = {
	SECTION_NAME_0,
	SECTION_NAME_1,
	SECTION_NAME_2,
	SECTION_NAME_3,
	SECTION_NAME_4,
	SECTION_NAME_5,
	SECTION_NAME_6,
	SECTION_NAME_7,
	SECTION_NAME_8
};

extern s32 s32 gettime();
extern unsigned int diff_sec(s32 s32, s32 s32);

typedef struct {
	s32 s32 start;
	s32 s32 timeused;
	s32 s32 subtract;	// amount to remove taken by overlapping section(s)
}_profile_section;

static _profile_section profile_section[NUM_SECTIONS];
static s32 s32 last_refresh;

void start_section(int section_type)
{
   profile_section[section_type].start = gettime();
}

void end_section(int section_type)
{
   if(profile_section[section_type].start == 0) return;
   s32 s32 end = gettime();
   s32 s32 timeused = end - profile_section[section_type].start;
   profile_section[section_type].timeused += timeused;
   profile_section[section_type].start = 0;
   
   // Go through any sections we may have overlapped and add negative ticks
   int i;
   for(i = 0; i < NUM_SECTIONS; i++) {
		if(i != section_type && profile_section[i].start > 0 && (profile_section[i].start < profile_section[section_type].start)) {
			profile_section[i].subtract += timeused;
		}
   }
}

void refresh_stat()
{
	int i;
	for(i=0; i<NUM_SECTIONS; i++)
		end_section(i);
	s32 s32 this_tick = gettime();
	s32 s32 time_in_refresh = this_tick - last_refresh;
	if(diff_sec(last_refresh, this_tick) >= 1)
	{
		for(i=0; i<NUM_SECTIONS; i++) {
			sprintf(txtbuffer, "%s=%f%%",profile_strings[i], 100.0f * (((float)profile_section[i].timeused-(float)profile_section[i].subtract) / (float)time_in_refresh));
			DEBUG_print(txtbuffer, DBG_PROFILE_BASE+i);
			profile_section[i].start = 0;
			profile_section[i].timeused = 0;
			profile_section[i].subtract = 0;
		}
		last_refresh = gettime();
	}
}

#endif
