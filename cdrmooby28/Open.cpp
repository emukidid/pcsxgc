/************************************************************************

Copyright mooby 2002

CDRMooby2 Open.cpp
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/

#ifdef WINDOWS
#pragma warning(disable:4786)
#endif

#include <iostream>
#include <string>

#include "CDInterface.hpp"
#include "defines.h"
#include "Preferences.hpp"


#include "externs.h"
#include <stdlib.h>

#ifdef WINDOWS
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_File_Icon.H>
#endif

extern "C" {
#include "../DEBUG.h"
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "../fileBrowser/fileBrowser-DVD.h"
#include "../fileBrowser/fileBrowser-CARD.h"
extern fileBrowser_file *isoFile; 
}

using namespace std;

extern CDInterface* theCD;
extern int rc;
extern TDTNFormat tdtnformat;
extern EMUMode mode;
extern Preferences prefs;

#ifdef _WINDOWS
#include <windows.h>

extern "C" 
{
  int WINAPI DllMain (HANDLE h, DWORD reason, void *ptr);
}

// DLL init.  thanks PEOPS =)
int WINAPI DllMain(HANDLE hModule,                  // DLL INIT
                   DWORD  dwReason, 
                   LPVOID lpReserved)
{
   return TRUE;                                          // very quick :)
}

#endif

int CD_Wait(void)
{
   return FPSE_OK;
}

void closeIt(void)
{
   if (theCD)
   {
      delete theCD;
      theCD = NULL;
   }
}

long CALLBACK Mooby2CDRclose(void)
{
   closeIt();
   return 0;
}

void CD_Close(void)
{
   closeIt();
}

void openIt(void)
{
   if (theCD)
      Mooby2CDRclose();
/*
   std::string theFile;
   if (prefs.prefsMap[autorunString] == std::string())
   {
      char * returned;
#ifdef WINDOWS
      while ( (returned = moobyFileChooser("Choose an image to run", theUsualSuspects.c_str(), prefs.prefsMap[lastrunString])) == NULL)
      {
         if (moobyAsk("You hit cancel or didn't pick a file.\nPick a different file? ('No' will end the program)") == 0)
         {
            exit(0);
         }
      }
#endif
      theFile = isoFile->name;
   }
   else
   {
      theFile = prefs.prefsMap[autorunString];
   }
   prefs.prefsMap[lastrunString] = theFile;
   prefs.write();*/
   theCD = new CDInterface();
   theCD->open(isoFile->name);
}

// psemu open call - call open
long CALLBACK Mooby2CDRopen(void)
{
   mode = psemu;
   openIt();
   return 0;
}

int CD_Open(unsigned int* par)
{
   mode = fpse;
   openIt();
   return FPSE_OK;
}

long CALLBACK Mooby2CDRshutdown(void)
{
   return Mooby2CDRclose();
}

long CALLBACK Mooby2CDRplay(unsigned char * sector)
{  
   return theCD->playTrack(CDTime(sector, msfint));
}

int CD_Play(unsigned char * sector)
{
   return theCD->playTrack(CDTime(sector, msfint));
}

long CALLBACK Mooby2CDRstop(void)
{
	return theCD->stopTrack();
}

int CD_Stop(void)
{
   return theCD->stopTrack();
}

long CALLBACK Mooby2CDRgetStatus(struct CdrStat *stat) 
{
   if (theCD->isPlaying())
   {
      stat->Type = 0x02;
      stat->Status = 0x80;
   }
   else
   {
      stat->Type = 0x01;
      stat->Status = 0x20;
   }
   MSFTime now = theCD->readTime().getMSF();
   stat->Time[0] = intToBCD(now.m());
   stat->Time[1] = intToBCD(now.s());
   stat->Time[2] = intToBCD(now.f());
   return 0;
}

char CALLBACK Mooby2CDRgetDriveLetter(void)
{
   return 0;
}


long CALLBACK Mooby2CDRinit(void)
{
   theCD=NULL;
   return 0;
}

long CALLBACK Mooby2CDRgetTN(unsigned char *buffer)
{
   buffer[0] = 1;
   if (tdtnformat == fsmint)
      buffer[1] = (char)theCD->getNumTracks();
   else
      buffer[1] = intToBCD((char)theCD->getNumTracks());
   return 0;
}

int CD_GetTN(char* buffer)
{
   buffer[1] = 1;
   buffer[2] = (char)theCD->getNumTracks();
   return FPSE_OK;
}

unsigned char * CALLBACK Mooby2CDRgetBufferSub(void)
{
   return theCD->readSubchannelPointer();
}

unsigned char* CD_GetSeek(void)
{
   return theCD->readSubchannelPointer() + 12;
}

long CALLBACK Mooby2CDRgetTD(unsigned char track, unsigned char *buffer)
{
   if (tdtnformat == fsmint)
      memcpy(buffer, theCD->getTrackInfo(track).trackStart.getMSFbuf(tdtnformat), 3);
   else
      memcpy(buffer, theCD->getTrackInfo(BCDToInt(track)).trackStart.getMSFbuf(tdtnformat), 3);
   return 0;
}

int CD_GetTD(char* result, int track)
{
	MSFTime now = theCD->getTrackInfo(BCDToInt(track)).trackStart.getMSF();
   result[1] = now.m();
   result[2] = now.s();

   return FPSE_OK;
}

long CALLBACK Mooby2CDRreadTrack(unsigned char *time)
{
   CDTime now(time, msfbcd);
   theCD->moveDataPointer(now);
   return 0;
}

unsigned char* CD_Read(unsigned char* time)
{
   CDTime now(time, msfint);
   theCD->moveDataPointer(now);
   return theCD->readDataPointer() + 12;
}

unsigned char * CALLBACK Mooby2CDRgetBuffer(void)
{
   return theCD->readDataPointer() + 12;
}
