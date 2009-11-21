/***************************************************************************
                          defines.h  -  description
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

// general buffer for reading several frames

#pragma pack(1)

typedef struct _FRAMEBUF
{
 DWORD         dwFrame; 
 DWORD         dwFrameCnt;
 DWORD         dwBufLen; 
 unsigned char BufData[1024*1024];
} FRAMEBUF;

#pragma pack()

// raw ioctl structs:

typedef enum _TRACK_MODE_TYPE 
{
 YellowMode2,
 XAForm2,
 CDDA
} TRACK_MODE_TYPE, *PTRACK_MODE_TYPE;

typedef struct _RAW_READ_INFO 
{
 LARGE_INTEGER   DiskOffset;
 ULONG           SectorCount;
 TRACK_MODE_TYPE TrackMode;
} RAW_READ_INFO, *PRAW_READ_INFO;

// sub cache:

typedef struct
{
 long   addr;
 void * pNext;
 unsigned char subq[10];
} SUB_DATA;

typedef struct
{
 long   addr;
 void * pNext;
} SUB_CACHE;

// ppf cache:

typedef struct
{
 long   addr;
 void * pNext;
 long   pos;
 long   anz;
 // memdata
} PPF_DATA;

typedef struct
{
 long   addr;
 void * pNext;
} PPF_CACHE;

/////////////////////////////////////////////////////////

#define MODE_BE_1   1
#define MODE_BE_2   2
#define MODE_28_1   3
#define MODE_28_2   4
#define MODE_D8_1   5
#define MODE_D4_1   6
#define MODE_D4_2   7

#define itob(i)      ((i)/10*16 + (i)%10)
#define btoi(b)      ((b)/16*10 + (b)%16)
#define itod(i)      ((((i)/10)<<4) + ((i)%10))
#define dtoi(b)      ((((b)>>4)&0xf)*10 + (((b)&0xf)%10))

/////////////////////////////////////////////////////////

__inline void addr2time(unsigned long addr, unsigned char *time)
{
 addr+=150;
 time[2] = (unsigned char)(addr%75);
 addr/=75;
 time[1]=(unsigned char)(addr%60);
 time[0]=(unsigned char)(addr/60);
}

__inline void addr2timeB(unsigned long addr, unsigned char *time)
{
 time[2] = itob((unsigned char)(addr%75));
 addr/=75;
 time[1]=itob((unsigned char)(addr%60));
 time[0]=itob((unsigned char)(addr/60));
}

__inline unsigned long time2addr(unsigned char *time)
{
 unsigned long addr;

 addr = time[0]*60;
 addr = (addr + time[1])*75;
 addr += time[2];
 addr -= 150;
 return addr;
}

__inline unsigned long time2addrB(unsigned char *time)
{
 unsigned long addr;

 addr = btoi(time[0])*60;
 addr = (addr + btoi(time[1]))*75;
 addr += btoi(time[2]);
 addr -= 150;
 return addr;
}

__inline unsigned long reOrder(unsigned long value)
{
#pragma warning (disable: 4035)
   __asm 
    {
     mov eax,value
     bswap eax
    }
}


/////////////////////////////////////////////////////////

typedef DWORD (*READFUNC)(BOOL bWait,FRAMEBUF * f);
typedef DWORD (*DEINITFUNC)(void);
typedef BOOL  (*READTRACKFUNC)(unsigned long addr);
typedef void  (*GETPTRFUNC)(void);

/////////////////////////////////////////////////////////
        
#define WAITFOREVER    0xFFFFFFFF
#define WAITSUB        10000
#define FRAMEBUFEXTRA  12
#define CDSECTOR       2352
#define MAXCACHEBLOCK  26
#define MAXCDBUFFER    (((MAXCACHEBLOCK+1)*(CDSECTOR+16))+240)

/////////////////////////////////////////////////////////
