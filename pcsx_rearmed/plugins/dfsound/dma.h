/***************************************************************************
                            dma.h  -  description
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

#ifndef __P_DMA_H__
#define __P_DMA_H__

unsigned short CALLBACK SPUreadDMA(void);
void CALLBACK SPUreadDMAMem(unsigned short * pusPSXMem,int iSize);
void CALLBACK SPUwriteDMA(unsigned short val);
void CALLBACK SPUwriteDMAMem(unsigned short * pusPSXMem,int iSize);

#endif /* __P_DMA_H__ */
