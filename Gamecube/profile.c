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

char * profile_strings[NUM_SECTIONS] = {
	"Total Time",
	SECTION_NAME_1,
	SECTION_NAME_2,
	SECTION_NAME_3,
	SECTION_NAME_4,
	SECTION_NAME_5,
	SECTION_NAME_6,
	SECTION_NAME_7,
	SECTION_NAME_8,
	SECTION_NAME_9
};

extern long long gettime();
extern unsigned int diff_sec(long long, long long);

static long long time_in_section[NUM_SECTIONS];
static long long last_start[NUM_SECTIONS];
static long long last_end[NUM_SECTIONS];
static long long last_refresh;


void start_section(int section_type)
{
	last_start[section_type] = gettime();
}

void end_section(int section_type)
{
	if(!last_start[section_type]) return;
	long long end = gettime();
	long long amount_not_ours = 0;
	
	last_end[section_type] = end;
	time_in_section[section_type] += (end - last_start[section_type]);
	
	// have any other sections opened since ours did? if so, subtract their time from ours.
	int i, oldest_open_type = 0;
	long long oldest_open = 0; 
	for(i=1; i<NUM_SECTIONS; i++) {
		if(!last_end[i] && last_start[i] && i != section_type && last_start[i] > last_start[section_type] && (last_start[i] < oldest_open || oldest_open == 0)) {
			oldest_open = last_start[i];
			oldest_open_type = i;
		}
	}
	if(oldest_open > 0) {
		amount_not_ours = (end-last_start[oldest_open_type]);
	}

	// have any other sections opened and closed since ours did? if so, subtract their time from ours.
	for(i=1; i<NUM_SECTIONS; i++) {
		if(last_start[i] && last_end[i] && i != section_type && last_start[i] > last_start[section_type] && last_end[i] > last_start[section_type]) {
			amount_not_ours += (time_in_section[i]);
		}
	}
	if(amount_not_ours < time_in_section[section_type]) {
		time_in_section[section_type]-=amount_not_ours;
	}
}

void refresh_stat()
{
	long long this_tick = gettime();
	if(diff_sec(last_refresh, this_tick) >= 1) {
		time_in_section[0] = this_tick - last_start[0];

		int i;
		long long all_sections = 0;
		for(i=1; i<NUM_SECTIONS; i++) {
			sprintf(txtbuffer, "%s=%f%%",profile_strings[i], 100.0f * (float)time_in_section[i] / (float)time_in_section[0]);
			DEBUG_print(txtbuffer, DBG_PROFILE_BASE+i);
			all_sections += time_in_section[i];
			last_start[i] = 0;
			last_end[i] = 0;
			time_in_section[i] = 0;
		}
		sprintf(txtbuffer, "unknown=%f%%  %llu %llu", 100.0f * ((float)(time_in_section[0]-all_sections) / (float)time_in_section[0])
			, time_in_section[0]
			, all_sections);
		DEBUG_print(txtbuffer, NUM_SECTIONS);

		last_start[0] = this_tick;
		last_refresh = this_tick;
	}
}

#endif
