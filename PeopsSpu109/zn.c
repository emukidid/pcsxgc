/***************************************************************************
                            zn.c  -  description
                            --------------------
    begin                : Wed April 23 2004
    copyright            : (C) 2004 by Pete Bernert
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
// 2004/04/23 - Pete
// - added ZINC zn interface
//
//*************************************************************************//

#include "stdafx.h"
#include "xa.h"

//*************************************************************************//
// global marker, if ZINC is running

int iZincEmu=0;

//-------------------------------------------------------------------------// 
// not used by ZINC

void CALLBACK PEOPS_SPUasync(u32 cycle);

void CALLBACK ZN_SPUupdate(void) 
{
 PEOPS_SPUasync(0);
}

//-------------------------------------------------------------------------// 

void CALLBACK PEOPS_SPUwriteRegister(u32 reg, unsigned short val);

void CALLBACK ZN_SPUwriteRegister(u32 reg, unsigned short val)
{ 
 PEOPS_SPUwriteRegister(reg,val);
}

//-------------------------------------------------------------------------// 

unsigned short CALLBACK PEOPS_SPUreadRegister(u32 reg);

unsigned short CALLBACK ZN_SPUreadRegister(u32 reg)
{
 return PEOPS_SPUreadRegister(reg);
}

//-------------------------------------------------------------------------// 
// not used by ZINC

unsigned short CALLBACK PEOPS_SPUreadDMA(void);

unsigned short CALLBACK ZN_SPUreadDMA(void)
{
 return PEOPS_SPUreadDMA();
}

//-------------------------------------------------------------------------// 
// not used by ZINC

void CALLBACK PEOPS_SPUwriteDMA(unsigned short val);

void CALLBACK ZN_SPUwriteDMA(unsigned short val)
{
 PEOPS_SPUwriteDMA(val);
}

//-------------------------------------------------------------------------// 
// not used by ZINC

void CALLBACK PEOPS_SPUwriteDMAMem(unsigned short * pusPSXMem,int iSize);

void CALLBACK ZN_SPUwriteDMAMem(unsigned short * pusPSXMem,int iSize)
{
 PEOPS_SPUwriteDMAMem(pusPSXMem,iSize);
}

//-------------------------------------------------------------------------// 
// not used by ZINC

void CALLBACK PEOPS_SPUreadDMAMem(unsigned short * pusPSXMem,int iSize);

void CALLBACK ZN_SPUreadDMAMem(unsigned short * pusPSXMem,int iSize)
{
 PEOPS_SPUreadDMAMem(pusPSXMem,iSize);
}

//-------------------------------------------------------------------------// 
// not used by ZINC

void CALLBACK PEOPS_SPUplayADPCMchannel(xa_decode_t *xap);

void CALLBACK ZN_SPUplayADPCMchannel(xa_decode_t *xap)
{
 PEOPS_SPUplayADPCMchannel(xap);
}

//-------------------------------------------------------------------------// 
// attention: no separate SPUInit/Shutdown funcs in ZN interface

s32 CALLBACK PEOPS_SPUinit(void);

#ifdef _WINDOWS

s32 CALLBACK SPUopen(HWND hW);

s32 CALLBACK ZN_SPUopen(HWND hW)                          
{
 iZincEmu=1;
 SPUinit();
 return SPUopen(hW);
}

#else

s32 PEOPS_SPUopen(void);

s32 ZN_SPUopen(void)
{
 iZincEmu=1;
 PEOPS_SPUinit();
 return PEOPS_SPUopen();
}

#endif

//-------------------------------------------------------------------------// 

s32 CALLBACK PEOPS_SPUshutdown(void);
s32 CALLBACK PEOPS_SPUclose(void);

s32 CALLBACK ZN_SPUclose(void)
{
 s32 lret=PEOPS_SPUclose();
 PEOPS_SPUshutdown();
 return lret;
}

//-------------------------------------------------------------------------// 
// not used by ZINC

void CALLBACK PEOPS_SPUregisterCallback(void (CALLBACK *callback)(void));

void CALLBACK ZN_SPUregisterCallback(void (CALLBACK *callback)(void))
{
 PEOPS_SPUregisterCallback(callback);
}

//-------------------------------------------------------------------------// 
// not used by ZINC

s32 CALLBACK PEOPS_SPUfreeze(u32 ulFreezeMode,void * pF);

s32 CALLBACK ZN_SPUfreeze(u32 ulFreezeMode,void * pF)
{
 return PEOPS_SPUfreeze(ulFreezeMode,pF);
}

//-------------------------------------------------------------------------// 

extern void (CALLBACK *irqQSound)(unsigned char *,s32 *,s32);      

void CALLBACK ZN_SPUqsound(void (CALLBACK *callback)(unsigned char *,s32 *,s32))
{
 irqQSound = callback;
}

//-------------------------------------------------------------------------// 
