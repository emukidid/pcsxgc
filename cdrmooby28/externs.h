/************************************************************************

Copyright mooby 2002

CDRMooby2 externs.h
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/

#ifdef WINDOWS
#pragma warning(disable:4786)
#endif

#ifndef EXTERNS_H
#define EXTERNS_H

#include "defines.h"

// sets all the callbacks as extern "c" for linux compatability
extern "C"
{

char * CALLBACK PSEgetLibName(void);
unsigned long CALLBACK PSEgetLibType(void);
unsigned long CALLBACK PSEgetLibVersion(void);

void CALLBACK Mooby2CDRabout(void);
long CALLBACK Mooby2CDRtest(void);
long CALLBACK Mooby2CDRconfigure(void);
long CALLBACK Mooby2CDRclose(void);
long CALLBACK Mooby2CDRopen(void);
long CALLBACK Mooby2CDRshutdown(void);
long CALLBACK Mooby2CDRplay(unsigned char * sector);
long CALLBACK Mooby2CDRstop(void);
long CALLBACK Mooby2CDRgetStatus(struct CdrStat *stat) ;
char CALLBACK Mooby2CDRgetDriveLetter(void);
long CALLBACK Mooby2CDRinit(void);
long CALLBACK Mooby2CDRgetTN(unsigned char *buffer);
unsigned char * CALLBACK Mooby2CDRgetBufferSub(void);
long CALLBACK Mooby2CDRgetTD(unsigned char track, unsigned char *buffer);
long CALLBACK Mooby2CDRreadTrack(unsigned char *time);
unsigned char * CALLBACK Mooby2CDRgetBuffer(void);

/* FPSE stuff, we don't use it in WiiSX */
void   CD_About(UINT32 *par);
int CD_Wait(void);
void CD_Close(void);
int CD_Open(unsigned int* par);
int CD_Play(unsigned char * sector);
int CD_Stop(void);
int CD_GetTN(char* result);
unsigned char* CD_GetSeek(void);
unsigned char* CD_Read(unsigned char* time);
int CD_GetTD(char* result, int track);
int    CD_Configure(UINT32 *par);

/* PS2 callbacks */

u32   CALLBACK PS2EgetLibType(void);
u32   CALLBACK PS2EgetLibVersion(void);
char* CALLBACK PS2EgetLibName(void);

s32  CALLBACK CDVDinit();
s32  CALLBACK CDVDopen();
void CALLBACK CDVDclose();
void CALLBACK CDVDshutdown();
s32  CALLBACK CDVDreadTrack(cdvdLoc *Time);

// return can be NULL (for async modes)
u8*  CALLBACK CDVDgetBuffer();
s32  CALLBACK CDVDgetTN(cdvdTN *Buffer);
s32  CALLBACK CDVDgetTD(u8 Track, cdvdLoc *Buffer);

// extended funcs
void CALLBACK CDVDconfigure();
void CALLBACK CDVDabout();
s32  CALLBACK CDVDtest();


}

#endif
