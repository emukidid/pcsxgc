/***************************************************************************
                            toc.c  -  description
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
// 2002/09/19 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

/////////////////////////////////////////////////////////

#include "stdafx.h"
#define _IN_TOC
#include "externals.h"

/////////////////////////////////////////////////////////

TOC sTOC;

/////////////////////////////////////////////////////////
// read toc

void ReadTOC(void)
{
 unsigned char xbuffer[3];DWORD dwStatus;

 LockGenCDAccess();

 memset(&(sTOC),0,sizeof(sTOC));                       // init toc infos

 dwStatus=GetSCSITOC((LPTOC)&sTOC);                    // get toc by scsi... may change that for ioctrl in xp/2k?

 UnlockGenCDAccess();

 if(dwStatus!=SS_COMP) return;
                                                       // re-order it to psemu pro standards
 addr2time(reOrder(sTOC.tracks[sTOC.cLastTrack].lAddr),xbuffer);
 xbuffer[0]=itob(xbuffer[0]);
 xbuffer[1]=itob(xbuffer[1]);
 xbuffer[2]=itob(xbuffer[2]);
 lMaxAddr=time2addrB(xbuffer);                         // get max data adr
}

/////////////////////////////////////////////////////////
// get the highest address of first (=data) track

unsigned long GetLastTrack1Addr(void)
{
 unsigned char xbuffer[3];DWORD dwStatus;
 unsigned long lmax;
 TOC xTOC;

 LockGenCDAccess();

 memset(&(xTOC),0,sizeof(xTOC));

 dwStatus=GetSCSITOC((LPTOC)&xTOC);

 UnlockGenCDAccess();

 if(dwStatus!=SS_COMP) return 0;

 addr2time(reOrder(xTOC.tracks[1].lAddr),xbuffer);

 xbuffer[0]=itob(xbuffer[0]);
 xbuffer[1]=itob(xbuffer[1]);
 xbuffer[2]=itob(xbuffer[2]);

 lmax=time2addrB(xbuffer);
 if(lmax<150) return 0;

 return lmax-150;
}

/////////////////////////////////////////////////////////
