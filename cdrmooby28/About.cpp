/************************************************************************

Copyright mooby 2002

CDRMooby2 About.cpp
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/


#include "defines.h"
#include "externs.h"
#include "Utils.hpp"
#include <FL/fl_ask.h>
#include <FL/Fl.H>
#include <string.h>

//  About.cpp
//  has the config, about, and test functions and related structs

//  lots of this code was taken from PEOPS.  THANK YOU!!!!

const  unsigned char version	= 1;	  // do not touch - library for PSEmu 1.x

#define REVISION  2
#define BUILD 8

#ifdef _DEBUG
#ifdef SUPER_DEBUG
static char *libraryName		= "Mooby2 cd disk image super debug driver";
#else
static char *libraryName		= "Mooby2 cd disk image debug driver";
#endif
#else
static char *libraryName		= "Mooby2 cd disk image driver";
#endif

//static char *PluginAuthor		= "Mooby";

static FPSEAbout aboutMooby = {FPSE_CDROM, 
   BUILD, 
   REVISION, 
   FPSE_OK, 
   "mooby", 
#ifdef _DEBUG
#ifdef SUPER_DEBUG
   "Mooby2 cd disk image super debug driver",
#else
   "Mooby2 cd disk image debug driver",
#endif
#else
   "Mooby2 cd disk image driver",
#endif
   "moo"};


/** PSX emu interface **/

// lots of this code was swiped from PEOPS.  thank you =)
char * CALLBACK PSEgetLibName(void)
{
 return libraryName;
}

unsigned long CALLBACK PSEgetLibType(void)
{
 return	PSE_LT_CDR;
}

unsigned long CALLBACK PSEgetLibVersion(void)
{
 return version<<16|REVISION<<8|BUILD;
}

void CALLBACK CDRabout(void)
{
   std::ostringstream out;
   out << libraryName;
   moobyMessage(out.str());
}

long CALLBACK CDRtest(void)
{
   moobyMessage("Of course it'll work.");
   return 0;
}

/** FPSE interface **/

void   CD_About(UINT32 *par)
{
   memcpy(par, &aboutMooby, sizeof(FPSEAbout));
}

/** PCSX2 interface **/

void CALLBACK CDVDabout()
{
   CDRabout();
}

s32  CALLBACK CDVDtest()
{
  return CDRtest();
}

u32   CALLBACK PS2EgetLibType(void)
{
   return PS2E_LT_CDVD;
}

u32   CALLBACK PS2EgetLibVersion(void)
{
   return PSEgetLibVersion();
}

char* CALLBACK PS2EgetLibName(void)
{
   return PSEgetLibName();  
}
