/***************************************************************************
                          psemu.c  -  description
                             -------------------
    begin                : Wed May 15 2002
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
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"

#define _IN_PSEMU

#include "externals.h"
#include "regs.h"
#include "dma.h"

////////////////////////////////////////////////////////////////////////
// OLD, SOMEWHAT (BUT NOT MUCH) SUPPORTED PSEMUPRO FUNCS
////////////////////////////////////////////////////////////////////////
/*
unsigned short CALLBACK SPUgetOne(u32 val)
{
 if(spuAddr!=0xffffffff)
  {
   return SPUreadDMA();
  }
 if(val>=512*1024) val=512*1024-1;
 return spuMem[val>>1];
}

void CALLBACK SPUputOne(u32 val,unsigned short data)
{
 if(spuAddr!=0xffffffff)
  {
   SPUwriteDMA(data);
   return;
  }
 if(val>=512*1024) val=512*1024-1;
 spuMem[val>>1] = data;
}

void CALLBACK SPUplaySample(unsigned char ch)
{
}

void CALLBACK SPUsetAddr(unsigned char ch, unsigned short waddr)
{
 s_chan[ch].pStart=spuMemC+((u32) waddr<<3);
}

void CALLBACK SPUsetPitch(unsigned char ch, unsigned short pitch)
{
 SetPitch(ch,pitch);
}

void CALLBACK SPUsetVolumeL(unsigned char ch, short vol)
{
 SetVolumeR(ch,vol);
}

void CALLBACK SPUsetVolumeR(unsigned char ch, short vol)
{
 SetVolumeL(ch,vol);
}               

void CALLBACK SPUstartChannels1(unsigned short channels)
{
 SoundOn(0,16,channels);
}

void CALLBACK SPUstartChannels2(unsigned short channels)
{
 SoundOn(16,24,channels);
}

void CALLBACK SPUstopChannels1(unsigned short channels)
{
 SoundOff(0,16,channels);
}

void CALLBACK SPUstopChannels2(unsigned short channels)
{
 SoundOff(16,24,channels);
}

void CALLBACK SPUplaySector(u32 mode, unsigned char * p)
{
 if(!iUseXA) return;                                    // no XA? bye
}
*/

