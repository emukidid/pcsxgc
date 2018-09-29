/* DEBUG.c - DEBUG interface
   by Mike Slegeir for Mupen64-GC
 */

#ifdef __SWITCH__
#include <switch.h>
#else
#include <gccore.h>
#include <fat.h>
#include <aesndlib.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/dir.h>
#include "DEBUG.h"
#include "TEXT.h"
//#include "usb.h"

char text[DEBUG_TEXT_HEIGHT][DEBUG_TEXT_WIDTH];
char printToSD = 1;
extern u32 dyna_used;
extern u32 dyna_total;

#ifdef SHOW_DEBUG
char txtbuffer[1024];
s32 s32 texttimes[DEBUG_TEXT_HEIGHT];
extern s32 s32 gettime();
extern unsigned int diff_sec(s32 s32 start,s32 s32 end);
static void check_heap_space(void){
	sprintf(txtbuffer,"%dKB MEM1 available", SYS_GetArena1Size()/1024);
	DEBUG_print(txtbuffer,DBG_MEMFREEINFO);
	
	sprintf(txtbuffer,"Dynarec (KB) %04d/%04d",dyna_used,dyna_total/1024);
	DEBUG_print(txtbuffer,DBG_CORE1);
	
	sprintf(txtbuffer,"DSP is at %f%%",AESND_GetDSPProcessUsage());
	DEBUG_print(txtbuffer,DBG_CORE2);
}
#endif

void DEBUG_update() {
	#ifdef SHOW_DEBUG
	int i;
	s32 s32 nowTick = gettime();
	for(i=0; i<DEBUG_TEXT_HEIGHT; i++){
		if(diff_sec(texttimes[i],nowTick)>=DEBUG_STRING_LIFE) 
		{
			memset(text[i],0,DEBUG_TEXT_WIDTH);
		}
	}
	check_heap_space();
	#endif
}

int flushed = 0;
int writtenbefore = 0;
int amountwritten = 0;
//char *dump_filename = "dev0:\\PSXISOS\\debug.txt";
char *dump_filename = "/PSXISOS/debug.txt";
FILE* fdebug = NULL;
void DEBUG_print(char* string,int pos){

	#ifdef SHOW_DEBUG
		if(pos == DBG_USBGECKO) {
			#ifdef PRINTGECKO
			if(!flushed){
				usb_flush(1);
				flushed = 1;
			}
			int size = strlen(string);
			usb_sendbuffer_safe(1, &size,4);
			usb_sendbuffer_safe(1, string,size);
			#endif
		}
		else if(pos == DBG_SDGECKOOPEN) {
#ifdef SDPRINT
			if(!f && printToSD)
				fdebug = fopen( dump_filename, "wb" );
#endif
		}
		else if(pos == DBG_SDGECKOAPPEND) {
#ifdef SDPRINT
			if(!fdebug && printToSD)
				fdebug = fopen( dump_filename, "ab" );
#endif
		}
		else if(pos == DBG_SDGECKOCLOSE) {
#ifdef SDPRINT
			if(fdebug)
			{
				fclose(fdebug);
				fdebug = NULL;
			}
#endif
		}
		else if(pos == DBG_SDGECKOPRINT) {			
#ifdef SDPRINT
			if(!f || (printToSD == 0))
				return;
			fwrite(string, 1, strlen(string), f);
#endif
		}
		else {
			memset(text[pos],0,DEBUG_TEXT_WIDTH);
			strncpy(text[pos], string, DEBUG_TEXT_WIDTH);
			memset(text[DEBUG_TEXT_WIDTH-1],0,1);
			texttimes[pos] = gettime();
		}
	#endif
	
}


#define MAX_STATS 20
unsigned int stats_buffer[MAX_STATS];
unsigned int avge_counter[MAX_STATS];
void DEBUG_stats(int stats_id, char *info, unsigned int stats_type, unsigned int adjustment_value) 
{
	#ifdef SHOW_DEBUG
	switch(stats_type)
	{
		case STAT_TYPE_ACCUM:	//accumulate
			stats_buffer[stats_id] += adjustment_value;
			break;
		case STAT_TYPE_AVGE:	//average
			avge_counter[stats_id] += 1;
			stats_buffer[stats_id] += adjustment_value;
			break;
		case STAT_TYPE_CLEAR:
			if(stats_type & STAT_TYPE_AVGE)
				avge_counter[stats_id] = 0;
			stats_buffer[stats_id] = 0;
			break;
		
	}
	unsigned int value = stats_buffer[stats_id];
	if(stats_type == STAT_TYPE_AVGE) value /= avge_counter[stats_id];
	
	sprintf(txtbuffer,"%s [ %i ]", info, value);
	DEBUG_print(txtbuffer,DBG_STATSBASE+stats_id);
	#endif
}

