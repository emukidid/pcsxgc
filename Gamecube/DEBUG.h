/* DEBUG.h - DEBUG interface
   by Mike Slegeir for Mupen64-GC
 */

#ifndef DEBUG_H
#define DEBUG_H

//#define SDPRINT
//#define PEOPS_SDLOG

#define DBG_MEMFREEINFO 0
#define DBG_CORE1	1
#define DBG_CORE2	2
#define DBG_CORE3	3
#define DBG_GPU1	4
#define DBG_GPU2	5
#define DBG_GPU3	6
#define DBG_SPU1	7
#define DBG_SPU2	8
#define DBG_SPU3	9
#define DBG_PROFILE_GFX 15
#define DBG_PROFILE_AUDIO 16
#define DBG_PROFILE_TLB 17
#define DBG_PROFILE_FP 18
#define DBG_PROFILE_COMP 19
#define DBG_PROFILE_INTERP 20
#define DBG_PROFILE_TRAMP 21
#define DBG_STATSBASE 12 // ALL stats print from this line onwards
#define DBG_SDGECKOAPPEND 0xFB
#define DBG_SDGECKOOPEN 0xFC
#define DBG_SDGECKOCLOSE 0xFD
#define DBG_SDGECKOPRINT 0xFE
#define DBG_USBGECKO 0xFF

// profiling

#define GFX_SECTION 1
#define AUDIO_SECTION 2
#define COMPILER_SECTION 3
#define IDLE_SECTION 4
#define TLB_SECTION 5
#define FP_SECTION 6
#define INTERP_SECTION 7
#define TRAMP_SECTION 8
#define FUNCS_SECTION 9
#define NUM_SECTIONS 9

#ifdef PROFILE

void start_section(int section_type);
void end_section(int section_type);
void refresh_stat();

#else

#define start_section(a)
#define end_section(a)
#define refresh_stat()

#endif

//DEBUG_stats defines
#define STAT_TYPE_ACCUM 0
#define STAT_TYPE_AVGE  1
#define STAT_TYPE_CLEAR 2

#define STATS_RECOMPCACHE 	0
#define STATS_CACHEMISSES	1
#define STATS_FCOUNTER		2	//FRAME counter
#define STATS_THREE			3

extern char txtbuffer[1024];
// Amount of time each string will be held onto
#define DEBUG_STRING_LIFE 5
// Dimensions of array returned by get_text
#define DEBUG_TEXT_WIDTH  100
#define DEBUG_TEXT_HEIGHT 40

#ifdef __cplusplus
extern "C" {
#endif

// Pre-formatted string (use sprintf before sending to print)
void DEBUG_print(char* string,int pos);
void DEBUG_stats(int stats_id, char *info, unsigned int stats_type, unsigned int adjustment_value);

// Should be called before get_text. Ages the strings, and remove old ones
void DEBUG_update(void);

// Returns pointer to an array of char*
char** DEBUG_get_text(void);

#ifdef __cplusplus
}
#endif

#endif


