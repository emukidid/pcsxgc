/***************************************************************************
                            dma.c  -  description
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

#define _IN_DMA

#include "externals.h"
#include <gccore.h>
#include "../PsxMem.h"
////////////////////////////////////////////////////////////////////////
// READ DMA (one value)
////////////////////////////////////////////////////////////////////////

unsigned short CALLBACK PEOPS_SPUreadDMA(void)
{
  //DEBUG_print("PEOPS_SPUreadDMA called",13);
 unsigned short s;

 //s=SWAP16(spuMem[spuAddr>>1]);
  s=spuMem[spuAddr>>1];

 spuAddr+=2;
 if(spuAddr>0x7ffff) spuAddr=0;

 iSpuAsyncWait=0;

 return s;
}

////////////////////////////////////////////////////////////////////////
// READ DMA (many values)
////////////////////////////////////////////////////////////////////////

void CALLBACK PEOPS_SPUreadDMAMem(unsigned short * pusPSXMem,int iSize)
{
  //DEBUG_print("PEOPS_SPUreadDMAmem called",14);
 int i;

 for(i=0;i<iSize;i++)
  {
   *pusPSXMem++=spuMem[spuAddr>>1];                    // spu addr got by writeregister
   //spuMem[spuAddr>>1] = SWAP16(spuMem[spuAddr>>1]);
   spuAddr+=2;                                         // inc spu addr
   if(spuAddr>0x7ffff) spuAddr=0;                      // wrap
  }

 iSpuAsyncWait=0;

}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

// to investigate: do sound data updates by writedma affect spu
// irqs? Will an irq be triggered, if new data is written to
// the memory irq address?

////////////////////////////////////////////////////////////////////////
// WRITE DMA (one value)
////////////////////////////////////////////////////////////////////////
  
void CALLBACK PEOPS_SPUwriteDMA(unsigned short val)
{
//  DEBUG_print("PEOPS_SPUwriteDMA called",15);
 //spuMem[spuAddr>>1] = SWAP16(val);                             // spu addr got by writeregister
  spuMem[spuAddr>>1] = val;
 spuAddr+=2;                                           // inc spu addr
 if(spuAddr>0x7ffff) spuAddr=0;                        // wrap

 iSpuAsyncWait=0;

}

////////////////////////////////////////////////////////////////////////
// WRITE DMA (many values)
////////////////////////////////////////////////////////////////////////

void CALLBACK PEOPS_SPUwriteDMAMem(unsigned short * pusPSXMem,int iSize)
{
//  DEBUG_print("PEOPS_SPUwriteDMAmem called",16);
 int i;

 for(i=0;i<iSize;i++)
  {
   spuMem[spuAddr>>1] = *pusPSXMem++;                  // spu addr got by writeregister
   //spuMem[spuAddr>>1] = SWAP16(spuMem[spuAddr>>1]);
   spuAddr+=2;                                         // inc spu addr
   if(spuAddr>0x7ffff) spuAddr=0;                      // wrap
  }
 
 iSpuAsyncWait=0;

}

////////////////////////////////////////////////////////////////////////

