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

void CALLBACK PEOPS_SPUasync(unsigned long cycle);

void CALLBACK ZN_SPUupdate(void) 
{
 PEOPS_SPUasync(0);
}

//-------------------------------------------------------------------------// 

void CALLBACK PEOPS_SPUwriteRegister(unsigned long reg, unsigned short val);

void CALLBACK ZN_SPUwriteRegister(unsigned long reg, unsigned short val)
{ 
 PEOPS_SPUwriteRegister(reg,val);
}

//-------------------------------------------------------------------------// 

unsigned short CALLBACK PEOPS_SPUreadRegister(unsigned long reg);

unsigned short CALLBACK ZN_SPUreadRegister(unsigned long reg)
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

long CALLBACK PEOPS_SPUinit(void);

#ifdef _WINDOWS

long CALLBACK SPUopen(HWND hW);

long CALLBACK ZN_SPUopen(HWND hW)                          
{
 iZincEmu=1;
 SPUinit();
 return SPUopen(hW);
}

#else

long PEOPS_SPUopen(void);

long ZN_SPUopen(void)
{
 iZincEmu=1;
 PEOPS_SPUinit();
 return PEOPS_SPUopen();
}

#endif

//-------------------------------------------------------------------------// 

long CALLBACK PEOPS_SPUshutdown(void);
long CALLBACK PEOPS_SPUclose(void);

long CALLBACK ZN_SPUclose(void)
{
 long lret=PEOPS_SPUclose();
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

long CALLBACK PEOPS_SPUfreeze(unsigned long ulFreezeMode,void * pF);

long CALLBACK ZN_SPUfreeze(unsigned long ulFreezeMode,void * pF)
{
 return PEOPS_SPUfreeze(ulFreezeMode,pF);
}

//-------------------------------------------------------------------------// 

extern void (CALLBACK *irqQSound)(unsigned char *,long *,long);      

void CALLBACK ZN_SPUqsound(void (CALLBACK *callback)(unsigned char *,long *,long))
{
 irqQSound = callback;
}

//-------------------------------------------------------------------------// 
