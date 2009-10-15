#ifndef _PLUGCD_H_
#define _PLUGCD_H_

#include <stdio.h>
#include <gccore.h>
#include "fileBrowser/fileBrowser.h"
#define CHAR_LEN 256

// 2352 is a sector size, so cache is 50 sectors
#define BUFFER_SECTORS 32
#define BUFFER_SIZE BUFFER_SECTORS*2352
#define BZIP_BUFFER_SECTORS 10

//  74 minutes * 60 sex/min * 75 frames/sec * 96 bytes needed per frame
#define TOTAL_CD_LENGTH 74*60*75
#define BYTES_PER_SUBCHANNEL_FRAME 96
#define MAX_SUBCHANNEL_DATA TOTAL_CD_LENGTH*BYTES_PER_SUBCHANNEL_FRAME

typedef struct {
	char	dn[128];
	char	fn[128];
} cd_conf;

cd_conf CDConfiguration;

enum TrackType
{
   unknown, Mode1, Mode2, Audio, Pregap = 0x80
};

enum CDType
{
   unk, Bin, Cue, Rar, IndexBZ, IndexZ, SBI, M3S
};

typedef struct
{
   enum TrackType type;
   char num;
   unsigned char start[3];
   unsigned char end[3];
} Track;

struct
{
   fileBrowser_file* cd;
   fileBrowser_file* cdda;
   int numtracks;
   long bufferPos;
   long sector;
   Track* tl;
   unsigned char buffer[BUFFER_SIZE];
   enum CDType type;
} CD;

void CDDAclose(void);

// function headers for cdreader.c
void openCue(fileBrowser_file* file);
void openBin(fileBrowser_file* file);
void openIso(fileBrowser_file* filename);
char getNumTracks();
void seekSector(const unsigned char m, const unsigned char s, const unsigned char f);
unsigned char* getSector();
void newCD(fileBrowser_file* filename);
void readit(const unsigned char m, const unsigned char s, const unsigned char f);


// subtracts two times in integer format (non-BCD) ->  l - r = a
#define sub(l, r, a)\
   a[1] = 0;\
   a[0] = 0;\
   a[2] = l[2] - r[2];\
   if ((char)a[2] < 0)\
   {\
      a[2] += 75;\
      a[1] -= 1;\
   }\
   a[1] += l[1] - r[1];\
   if ((char)a[1] < 0)\
   {\
      a[1] += 60;\
      a[0] -= 1;\
   }\
   a[0] += l[0] - r[0];\

// converts a time like 17:61:00  to 18:01:00
#define normalizeTime(c)\
   while(c[2] > 75)\
   {\
      c[2] -= 75;\
      c[1] += 1;\
   }\
   while(c[1] > 60)\
   {\
      c[1] -= 60;\
      c[0] += 1;\
   }

// converts uchar in c to BCD character
#define intToBCD(c) (unsigned char)((c%10) | ((c/10)<<4))

// converts BCD number in c to uchar
#define BCDToInt(c) (unsigned char)((c & 0x0F) + 10 * ((c & 0xF0) >> 4))

#endif

