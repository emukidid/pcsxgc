#pragma warning(disable:4786)

#include "externs.h"
#include "CDTime.hpp"
#include "CDInterface.hpp"

extern CDInterface* theCD;

s32  CALLBACK CDVDinit()
{
  return CDRinit();
}

s32  CALLBACK CDVDopen()
{
  return CDRopen();
}

void CALLBACK CDVDclose()
{
  CDRclose();
}

void CALLBACK CDVDshutdown()
{
  CDRshutdown();
}

s32  CALLBACK CDVDreadTrack(cdvdLoc *Time)
{
   CDTime now((unsigned char*)Time, msfint);
   theCD->moveDataPointer(now);
   return 0;
}

u8*  CALLBACK CDVDgetBuffer()
{
  return (u8*)theCD->readDataPointer();
}

s32  CALLBACK CDVDgetTN(cdvdTN *Buffer)
{
   return CDRgetTN((unsigned char*)Buffer);
}

s32  CALLBACK CDVDgetTD(u8 Track, cdvdLoc *Buffer)
{
   return CDRgetTD(Track, (unsigned char*)Buffer);
}

