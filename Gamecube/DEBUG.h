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
#define DBG_CDR1  10
#define DBG_CDR2  11
#define DBG_CDR3  12
#define DBG_CDR4  13
#define DBG_PROFILE_BASE 14

#define DBG_STATSBASE 12 // ALL stats print from this line onwards
#define DBG_SDGECKOAPPEND 0xFB
#define DBG_SDGECKOOPEN 0xFC
#define DBG_SDGECKOCLOSE 0xFD
#define DBG_SDGECKOPRINT 0xFE
#define DBG_USBGECKO 0xFF

// profiling

#define GFX_SECTION 1
#define SECTION_NAME_1 "Graphics"
#define AUDIO_SECTION 2
#define SECTION_NAME_2 "Audio"
#define XA_SECTION 3
#define SECTION_NAME_3 "XA"
#define IDLE_SECTION 4
#define SECTION_NAME_4 "Idle"
#define CDR_SECTION 5
#define SECTION_NAME_5 "CDR"
#define CDDA_SECTION 6
#define SECTION_NAME_6 "CDDA"
#define COMPILER_SECTION 7
#define SECTION_NAME_7 "Compiler"
#define CORE_SECTION 8
#define SECTION_NAME_8 "Code Exec"
#define INTERP_SECTION 9
#define SECTION_NAME_9 "Interpreter"
#define NUM_SECTIONS 10

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
#define DEBUG_TEXT_WIDTH  256
#define DEBUG_TEXT_HEIGHT 16

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


