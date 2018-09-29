/***************************************************************************
                          zn.c  -  description
                             -------------------
    begin                : Sat Jan 31 2004
    copyright            : (C) 2004 by Pete Bernert
    web                  : www.pbernert.com   
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
// 2004/01/31 - Pete  
// - added zn interface
//
//*************************************************************************// 

#include "stdafx.h"

#define _IN_ZN

#include "externals.h"

// --------------------------------------------------- // 
// - psx gpu plugin interface prototypes-------------- //
// --------------------------------------------------- //

#ifdef _WINDOWS
s32 CALLBACK GPUopen(HWND hwndGPU);
#else
s32 GPUopen(u32 * disp,const char * CapText,const char * CfgFile);
#endif
void CALLBACK GPUdisplayFlags(u32 dwFlags);
void CALLBACK GPUmakeSnapshot(void);
s32 CALLBACK GPUinit();
s32 CALLBACK GPUclose();
s32 CALLBACK GPUshutdown();
void CALLBACK GPUcursor(int iPlayer,int x,int y);
void CALLBACK GPUupdateLace(void);
u32 CALLBACK GPUreadStatus(void);
void CALLBACK GPUwriteStatus(u32 gdata);
void CALLBACK GPUreadDataMem(u32 * pMem, int iSize);
u32 CALLBACK GPUreadData(void);
void CALLBACK GPUwriteDataMem(u32 * pMem, int iSize);
void CALLBACK GPUwriteData(u32 gdata);
void CALLBACK GPUsetMode(u32 gdata);
s32 CALLBACK GPUgetMode(void);
s32 CALLBACK GPUdmaChain(u32 * baseAddrL, u32 addr);
s32 CALLBACK GPUconfigure(void);
void CALLBACK GPUabout(void);
s32 CALLBACK GPUtest(void);
s32 CALLBACK GPUfreeze(u32 ulGetFreezeData,void * pF);
void CALLBACK GPUgetScreenPic(unsigned char * pMem);
void CALLBACK GPUshowScreenPic(unsigned char * pMem);
#ifndef _WINDOWS
void CALLBACK GPUkeypressed(int keycode);
#endif

// --------------------------------------------------- // 
// - zn gpu interface -------------------------------- // 
// --------------------------------------------------- // 

u32 dwGPUVersion=0;
int           iGPUHeight=512;
int           iGPUHeightMask=511;
int           GlobalTextIL=0;
int           iTileCheat=0;

// --------------------------------------------------- //
// --------------------------------------------------- // 
// --------------------------------------------------- // 

typedef struct GPUOTAG
 {
  u32  Version;        // Version of structure - currently 1
  s32           hWnd;           // Window handle
  u32  ScreenRotation; // 0 = 0CW, 1 = 90CW, 2 = 180CW, 3 = 270CW = 90CCW
  u32  GPUVersion;     // 0 = a, 1 = b, 2 = c
  const char*    GameName;       // NULL terminated string
  const char*    CfgFile;        // NULL terminated string
 } GPUConfiguration_t;

// --------------------------------------------------- // 
// --------------------------------------------------- // 
// --------------------------------------------------- // 

void CALLBACK ZN_GPUdisplayFlags(u32 dwFlags)
{
 GPUdisplayFlags(dwFlags);
}

// --------------------------------------------------- //

void CALLBACK ZN_GPUmakeSnapshot(void)
{
 GPUmakeSnapshot();
}

// --------------------------------------------------- //

s32 CALLBACK ZN_GPUinit()
{                                                      // we always set the vram size to 2MB, if the ZN interface is used
 iGPUHeight=1024;
 iGPUHeightMask=1023;

 return GPUinit();
}

// --------------------------------------------------- //

s32 CALLBACK ZN_GPUopen(void * vcfg)
{
 GPUConfiguration_t * cfg=(GPUConfiguration_t *)vcfg;
 s32 lret;

 if(!cfg)            return -1;
 if(cfg->Version!=1) return -1;

#ifdef _WINDOWS
 //pConfigFile=(char *)cfg->CfgFile;                     // only used in this open, so we can store this temp pointer here without danger... don't access it later, though!
 lret=GPUopen((HWND)cfg->hWnd);
#else
 lret=GPUopen(&cfg->hWnd,cfg->GameName,cfg->CfgFile);
#endif

/*
 if(!lstrcmp(cfg->GameName,"kikaioh")     ||
    !lstrcmp(cfg->GameName,"sr2j")        ||
    !lstrcmp(cfg->GameName,"rvschool_a"))
  iTileCheat=1;
*/

 // some ZN games seem to erase the cluts with a 'white' TileS... strange..
 // I've added a cheat to avoid this issue. We can set it globally (for
 // all ZiNc games) without much risk

 iTileCheat=1;

 dwGPUVersion=cfg->GPUVersion;

 return lret;
}

// --------------------------------------------------- //

s32 CALLBACK ZN_GPUclose()
{
 return GPUclose();
}

// --------------------------------------------------- // 

s32 CALLBACK ZN_GPUshutdown()
{
 return GPUshutdown();
}

// --------------------------------------------------- // 

void CALLBACK ZN_GPUupdateLace(void)
{
 GPUupdateLace();
}

// --------------------------------------------------- // 

u32 CALLBACK ZN_GPUreadStatus(void)
{
 return GPUreadStatus();
}

// --------------------------------------------------- // 

void CALLBACK ZN_GPUwriteStatus(u32 gdata)
{
 GPUwriteStatus(gdata);
}

// --------------------------------------------------- // 

s32 CALLBACK ZN_GPUdmaSliceOut(u32 *baseAddrL, u32 addr, u32 iSize)
{
 GPUreadDataMem(baseAddrL+addr,iSize);
 return 0;
}

// --------------------------------------------------- // 

u32 CALLBACK ZN_GPUreadData(void)
{
 return GPUreadData();
}

// --------------------------------------------------- // 

void CALLBACK ZN_GPUsetMode(u32 gdata)
{
 GPUsetMode(gdata);
}

// --------------------------------------------------- // 

s32 CALLBACK ZN_GPUgetMode(void)
{
 return GPUgetMode();
}

// --------------------------------------------------- // 

s32 CALLBACK ZN_GPUdmaSliceIn(u32 *baseAddrL, u32 addr, u32 iSize)
{
 GPUwriteDataMem(baseAddrL+addr,iSize);
 return 0;
}
// --------------------------------------------------- // 

void CALLBACK ZN_GPUwriteData(u32 gdata)
{
 GPUwriteDataMem(&gdata,1);
}

// --------------------------------------------------- // 

s32 CALLBACK ZN_GPUdmaChain(u32 * baseAddrL, u32 addr)
{
 return GPUdmaChain(baseAddrL,addr);
}

// --------------------------------------------------- // 

s32 CALLBACK ZN_GPUtest(void)
{
 return GPUtest();
}

// --------------------------------------------------- // 

s32 CALLBACK ZN_GPUfreeze(u32 ulGetFreezeData,void * pF)
{
 return GPUfreeze(ulGetFreezeData,pF);
}

// --------------------------------------------------- // 

void CALLBACK ZN_GPUgetScreenPic(unsigned char * pMem)
{
 GPUgetScreenPic(pMem);
}

// --------------------------------------------------- // 

void CALLBACK ZN_GPUshowScreenPic(unsigned char * pMem)
{
 GPUshowScreenPic(pMem);
}

// --------------------------------------------------- // 
                              
#ifndef _WINDOWS

void CALLBACK ZN_GPUkeypressed(int keycode)
{
 GPUkeypressed(keycode);
}

#endif
