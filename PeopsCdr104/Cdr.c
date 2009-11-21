/***************************************************************************
                            cdr.c  -  description
                             -------------------
    begin                : Wed Sep 18 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************//
// History of changes:
//
// 2002/09/28 - linuzappz
// - improved CDRgetStatus
//
// 2002/09/19 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

/////////////////////////////////////////////////////////

#include "stdafx.h"
#include <time.h>
#include "resource.h"
#define _IN_CDR
#include "externals.h"

/////////////////////////////////////////////////////////

const unsigned char version  =  PLUGIN_VERSION;
const unsigned char revision =  1;
const unsigned char build    =  4;
char *libraryName            =  "P.E.Op.S. CDR Driver";

/////////////////////////////////////////////////////////

BOOL bIsOpen=FALSE;                                    // flag: open called once
BOOL bCDDAPlay=FALSE;                                  // flag: audio is playing
int  iCDROK=0;                                         // !=0: cd is ok

/////////////////////////////////////////////////////////
// usual info funcs

char * CALLBACK PSEgetLibName(void)
{
 return libraryName;
}
                             
unsigned long CALLBACK PSEgetLibType(void)
{
 return  PSE_LT_CDR;
}

unsigned long CALLBACK PSEgetLibVersion(void)
{
 return version<<16|revision<<8|build;
}

/////////////////////////////////////////////////////////
// init: called once at library load

long CALLBACK CDRinit(void)
{
 szSUBF[0]=0;                                          // just init the filename buffers
 szPPF[0] =0;
 return 0;                                            
}

/////////////////////////////////////////////////////////
// shutdown: called once at final exit

long CALLBACK CDRshutdown(void)
{
 return 0;
}

/////////////////////////////////////////////////////////
// open: called, when games starts/cd has been changed

long CALLBACK CDRopen(void)
{
 if(bIsOpen)                                           // double-open check (if the main emu coder doesn't know what he is doing ;)
  {
   if(iCDROK<=0) return PSE_CDR_ERR_NOREAD;
   else          return PSE_CDR_ERR_SUCCESS;
  }

 bIsOpen=TRUE;                                         // ok, open func called once
 
 ReadConfig();                                         // read user config

 BuildPPFCache();                                      // build ppf cache

 BuildSUBCache();                                      // build sub cache

 CreateREADBufs();                                     // setup generic read buffers

 CreateGenEvent();                                     // create read event

 iCDROK=OpenGenCD(iCD_AD,iCD_TA,iCD_LU,iRType);        // generic open, setup read func

 if(iCDROK<=0) {iCDROK=0;return PSE_CDR_ERR_NOREAD;}

 ReadTOC();                                            // read the toc

 SetGenCDSpeed(0);                                     // try to change the reading speed (if wanted)

 return PSE_CDR_ERR_SUCCESS;                           // ok, done
}

/////////////////////////////////////////////////////////
// close: called when emulation stops

long CALLBACK CDRclose(void)
{
 if(!bIsOpen) return 0;                                // no open? no close...

 bIsOpen=FALSE;                                        // no more open

 LockGenCDAccess();                                    // make sure that no more reading is happening

 if(iCDROK)                                            // cd was ok?
  {
   if(bCDDAPlay) {DoCDDAPlay(0);bCDDAPlay=FALSE;}      // -> cdda playing? stop it
   SetGenCDSpeed(1);                                   // -> repair speed
   CloseGenCD();                                       // -> cd not used anymore
  }

 UnlockGenCDAccess();

 FreeREADBufs();                                       // free read bufs
 FreeGenEvent();                                       // free event
 FreePPFCache();                                       // free ppf cache
 FreeSUBCache();                                       // free sub cache

 return 0;
}

/////////////////////////////////////////////////////////
// test: ah, well, always fine

long CALLBACK CDRtest(void)
{
 return PSE_CDR_ERR_SUCCESS;
}            

/////////////////////////////////////////////////////////
// gettn: first/last track num

long CALLBACK CDRgetTN(unsigned char *buffer)       
{
 if(!bIsOpen)  CDRopen();                              // not open? funny emu...

 if(!iCDROK) {buffer[0]=1;buffer[1]=1;return -1;}      // cd not ok?

 ReadTOC();                                            // read the TOC

 buffer[0]=sTOC.cFirstTrack;                           // get the infos
 buffer[1]=sTOC.cLastTrack;

 return 0;
}

/////////////////////////////////////////////////////////
// gettd: track addr

long CALLBACK CDRgetTD(unsigned char track, unsigned char *buffer)
{
 unsigned char b;unsigned long lu;

 if(!bIsOpen) CDRopen();                               // not open? funny emu...
 
 if(!iCDROK)  return PSE_ERR_FATAL;                    // cd not ok? bye

 ReadTOC();                                            // read toc

 if(track==0)                                          // 0 = last track
  {
   lu=reOrder(sTOC.tracks[sTOC.cLastTrack].lAddr);
   addr2time(lu,buffer);
  }
 else                                                  // others: track n
  {
   lu=reOrder(sTOC.tracks[track-1].lAddr);
   addr2time(lu,buffer);
  }

 b=buffer[0];                                          // swap infos (psemu pro/epsxe)
 buffer[0]=buffer[2];
 buffer[2]=b;

 return 0;
}

/////////////////////////////////////////////////////////
// readtrack: start reading at given address

long CALLBACK CDRreadTrack(unsigned char *time)
{
 if(!bIsOpen)   CDRopen();                             // usual checks
 if(!iCDROK)    return PSE_ERR_FATAL;
 if(bCDDAPlay)  bCDDAPlay=FALSE;

 lLastAccessedAddr=time2addrB(time);                   // calc adr

 if(!pReadTrackFunc(lLastAccessedAddr))
  return PSE_CDR_ERR_NOREAD;

 return PSE_CDR_ERR_SUCCESS;
}

/////////////////////////////////////////////////////////
// getbuffer: will be called after readtrack, to get ptr 
//            to data

unsigned char * CALLBACK CDRgetBuffer(void)
{
 if(!bIsOpen) CDRopen();

 if(pGetPtrFunc) pGetPtrFunc();                        // get ptr on thread modes

 return pCurrReadBuf;                                  // easy :)
}

/////////////////////////////////////////////////////////
// getsubbuffer: will also be called after readtrack, to 
//               get ptr to subdata, or NULL
           
unsigned char * CALLBACK CDRgetBufferSub(void)
{
 if(!bIsOpen)  CDRopen();

 if(bCDDAPlay) return GetCDDAPlayPosition();           // cdda playing? give (not exact) sub data

 if(subHead)                                           // some sub file?
  CheckSUBCache(lLastAccessedAddr);                    // -> get cached subs
 else 
 if(iUseSubReading!=1 && pCurrSubBuf)                  // no direct cd sub read?
  FakeSubData(lLastAccessedAddr);                      // -> fake the data

 return pCurrSubBuf;                                   // can be NULL: then the main emu has to calc the right data
}

/////////////////////////////////////////////////////////
// audioplay: PLAYSECTOR is NOT BCD coded !!!

long CALLBACK CDRplay(unsigned char * sector)
{
 if(!bIsOpen)   CDRopen();
 if(!iCDROK)    return PSE_ERR_FATAL;

 if(!DoCDDAPlay(time2addr(sector)));                   // start playing
  return PSE_CDR_ERR_NOREAD;

 bCDDAPlay=TRUE;                                       // raise flag: we are playing

 return PSE_CDR_ERR_SUCCESS;
}

/////////////////////////////////////////////////////////
// audiostop: stops cdda playing

long CALLBACK CDRstop(void)
{
 if(!bCDDAPlay) return PSE_ERR_FATAL;

 DoCDDAPlay(0);                                        // stop cdda

 bCDDAPlay=FALSE;                                      // reset flag: no more playing

 return PSE_CDR_ERR_SUCCESS;
}

/////////////////////////////////////////////////////////
// getdriveletter

char CALLBACK CDRgetDriveLetter(void)
{
 if(!iCDROK) return 0;                                 // not open? no way to get the letter

 if(iInterfaceMode==2 || iInterfaceMode==3)            // w2k/xp: easy
  {
   return MapIOCTLDriveLetter(iCD_AD,iCD_TA,iCD_LU);
  }
 else                                                  // but with aspi???
  {                                                    // -> no idea yet (maybe registry read...pfff)
  }

 return 0;
}

/////////////////////////////////////////////////////////
// configure: shows config window

long CALLBACK CDRconfigure(void)
{
 if(iCDROK)                                            // mmm... someone has already called Open? bad
  {MessageBeep((UINT)-1);return PSE_ERR_FATAL;}

 CreateGenEvent();                                     // we need an event handle

 DialogBox(hInst,MAKEINTRESOURCE(IDD_CONFIG),          // call dialog
           GetActiveWindow(),(DLGPROC)CDRDlgProc);

 FreeGenEvent();                                       // free event handle

 return PSE_CDR_ERR_SUCCESS;
}

/////////////////////////////////////////////////////////
// about: shows about window

long CALLBACK CDRabout(void)
{
 DialogBox(hInst,MAKEINTRESOURCE(IDD_ABOUT),
           GetActiveWindow(),(DLGPROC)AboutDlgProc);

 return PSE_CDR_ERR_SUCCESS;
}

/////////////////////////////////////////////////////////
// getstatus: pcsx func... poorly supported here
//            problem is: func will be called often, which
//            would block all of my cdr reading if I would use
//            lotsa scsi commands

struct CdrStat 
{
 unsigned long Type;
 unsigned long Status;
 unsigned char Time[3]; // current playing time
};

struct CdrStat ostat;

// reads cdr status
// type:
// 0x00 - unknown
// 0x01 - data
// 0x02 - audio
// 0xff - no cdrom
// status:
// 0x00 - unknown
// 0x02 - error
// 0x08 - seek error
// 0x10 - shell open
// 0x20 - reading
// 0x40 - seeking
// 0x80 - playing
// time:
// byte 0 - minute
// byte 1 - second
// byte 2 - frame


long CALLBACK CDRgetStatus(struct CdrStat *stat) 
{
 int iStatus;
 static time_t to;

 if(!bCDDAPlay)  // if not playing update stat only once in a second
  { 
   if(to<time(NULL)) 
    {
     to = time(NULL);
    } 
   else 
    {
     memcpy(stat, &ostat, sizeof(struct CdrStat));
     return 0;
    }
  }

 memset(stat, 0, sizeof(struct CdrStat));

 if(!iCDROK) return -1;                                // not opened? bye

 if(bCDDAPlay)                                         // cdda is playing?
  {
   unsigned char * pB=GetCDDAPlayPosition();           // -> get pos
   stat->Type = 0x02;                                  // -> audio
   if(pB)
    {
     stat->Status|=0x80;                               // --> playing flag
     stat->Time[0]=pB[18];                             // --> and curr play time
     stat->Time[1]=pB[19];
     stat->Time[2]=pB[20];
    }
  }
 else                                                  // cdda not playing?
  { 
   stat->Type = 0x01;                                  // -> data
  }

 LockGenCDAccess();                                    // make sure that no more reading is happening
 iStatus=GetSCSIStatus(iCD_AD,iCD_TA,iCD_LU);          // get device status
 UnlockGenCDAccess();

 if(iStatus==SS_ERR) 
  {                                                    // no cdrom?
   stat->Type = 0xff;
   stat->Status|= 0x10;
  }

 memcpy(&ostat, stat, sizeof(struct CdrStat));

 return 0;
}

/////////////////////////////////////////////////////////
