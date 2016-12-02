/***************************************************************************
                          key.c  -  description
                             -------------------
    begin                : Sun Mar 08 2009
    copyright            : (C) 1999-2009 by Pete Bernert
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
// 2009/03/08 - Pete  
// - generic cleanup for the Peops release
//
//*************************************************************************// 

#include "stdafx.h"

#define _IN_KEY

#include "externals.h"
#include "menu.h"
#include "texture.h"
#include "draw.h"
#include "fps.h"

////////////////////////////////////////////////////////////////////////
// KeyBoard handler stuff
////////////////////////////////////////////////////////////////////////

unsigned long   ulKeybits=0;                     

#ifdef _WINDOWS

static WNDPROC  pOldWinProc=0;
char            szGPUKeys[7];

////////////////////////////////////////////////////////////////////////
// keyboard handler (WINDOWS)
////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK key_winproc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
 switch(message)
  {
   case WM_SYSCOMMAND: 
     if(iNoScreenSaver && wParam==SC_SCREENSAVE) return 0L;
    break;

   case WM_KEYDOWN:
    if(wParam==(WPARAM)szGPUKeys[1]) {ulKeybits|=KEY_RESETTEXSTORE;break;}
    break;

   case WM_SYSKEYUP:
    if(wParam==VK_RETURN) bChangeWinMode=TRUE;
   break;

   case WM_KEYUP:
     if(wParam==(WPARAM)szGPUKeys[0]) 
      {
       if(ulKeybits&KEY_SHOWFPS) 
        {
         ulKeybits&=~KEY_SHOWFPS;
         HideText();
         DestroyPic();
        }
       else 
        {
         ulKeybits|=KEY_SHOWFPS;
         szDispBuf[0]=0;
         BuildDispMenu(0);
        }
       break;
      }
     if(wParam==(WPARAM)szGPUKeys[1]) {ShowGpuPic();break;}
     if(wParam==(WPARAM)szGPUKeys[2]) {SwitchDispMenu(-1); break;}
     if(wParam==(WPARAM)szGPUKeys[3]) {SwitchDispMenu( 1); break;}
     if(wParam==(WPARAM)szGPUKeys[4]) 
      {
       if(GetAsyncKeyState(VK_SHIFT)&32768)
        {
         if(iBlurBuffer) iBlurBuffer=0;
         else            iBlurBuffer=1;
        }
       BuildDispMenu(-1);
       break;
      }
     if(wParam==(WPARAM)szGPUKeys[5]) {BuildDispMenu( 1);  break;}

  }
 return pOldWinProc( hwnd, message, wParam, lParam );
}

////////////////////////////////////////////////////////////////////////

void InitKeyHandler(void)
{
 if(!pOldWinProc)
  {      
   pOldWinProc = (WNDPROC)GetWindowLong(hWWindow, GWL_WNDPROC );
   SetWindowLong(hWWindow, GWL_WNDPROC, (long)key_winproc);
  }
} 

////////////////////////////////////////////////////////////////////////

void ExitKeyHandler(void)
{
 if(pOldWinProc)
  SetWindowLong(hWWindow,GWL_WNDPROC,              // set old proc
                (long)pOldWinProc);
 pOldWinProc = 0;                                      
}

////////////////////////////////////////////////////////////////////////

#else

////////////////////////////////////////////////////////////////////////
// keyboard handler (LINUX)
////////////////////////////////////////////////////////////////////////

#define VK_INSERT      65379
#define VK_HOME        65360
#define VK_PRIOR       65365
#define VK_NEXT        65366
#define VK_END         65367
#define VK_DEL         65535
#define VK_F5          65474

void GPUkeypressed(int keycode)
{
    switch(keycode)
     {
      case VK_F5:
       bSnapShot=1;
      break;

      case VK_INSERT:
       ulKeybits|=KEY_RESETTEXSTORE;
       if(iBlurBuffer) iBlurBuffer=0;
       else            iBlurBuffer=1;
       break;

      case VK_DEL: 
       if(ulKeybits&KEY_SHOWFPS)
        {
         ulKeybits&=~KEY_SHOWFPS;
         HideText();
         DestroyPic();
        }
       else 
        {
         ulKeybits|=KEY_SHOWFPS;
         szDispBuf[0]=0;
         BuildDispMenu(0);

        }
       break;

      case VK_PRIOR: BuildDispMenu(-1);            break;
      case VK_NEXT:  BuildDispMenu( 1);            break;
      case VK_END:   SwitchDispMenu( 1);           break;
      case VK_HOME:  SwitchDispMenu(-1);           break;
     }

}

////////////////////////////////////////////////////////////////////////

void InitKeyHandler(void)
{
// nothing in Linux
}

////////////////////////////////////////////////////////////////////////

void ExitKeyHandler(void)
{
// nothing in Linux
}

////////////////////////////////////////////////////////////////////////

#endif

////////////////////////////////////////////////////////////////////////
// reset stuff on special keyboard commands
////////////////////////////////////////////////////////////////////////

void ResetStuff(void)
{
#ifndef __GX__
 ResetTextureArea(TRUE);
 ulKeybits&=~KEY_RESETTEXSTORE;

 if(ulKeybits&KEY_BLACKWHITE)
  {
   if(bUseFixes) {bUseFixes=FALSE;dwActFixes=0;}
   else          {bUseFixes=TRUE; dwActFixes=dwCfgFixes;}
   SetExtGLFuncs();
   if(iFrameLimit==2) SetAutoFrameCap();
   ulKeybits&=~KEY_BLACKWHITE;
  }

 if(ulKeybits&KEY_RESETFILTER)
  {
   if(ulKeybits&KEY_STEPDOWN)
        iFilterType--;
   else iFilterType++;
   if(iFilterType>6) iFilterType=0;
   if(iFilterType<0) iFilterType=6;
   SetExtGLFuncs();
   ulKeybits&=~(KEY_RESETFILTER|KEY_STEPDOWN);
   BuildDispMenu(0);
  }

 if(ulKeybits&KEY_RESETOPAQUE)
  {
   bOpaquePass=!bOpaquePass;
   SetExtGLFuncs();
   ulKeybits&=~KEY_RESETOPAQUE;
   BuildDispMenu(0);
  }

 if(ulKeybits&KEY_RESETADVBLEND)
  {
   bAdvancedBlend=!bAdvancedBlend;
   SetExtGLFuncs();
   ulKeybits&=~KEY_RESETADVBLEND;
   BuildDispMenu(0);
  }

 if(ulKeybits&KEY_RESETDITHER)
  {
   bDrawDither=!bDrawDither;
   if(bDrawDither)  glEnable(GL_DITHER); 
   else             glDisable(GL_DITHER); 
   ulKeybits&=~KEY_RESETDITHER;
   BuildDispMenu(0);
  }

 if(ulKeybits & KEY_TOGGLEFBTEXTURE)
  {
   if(ulKeybits&KEY_STEPDOWN)
        iFrameTexType--;
   else iFrameTexType++;
   if(iFrameTexType>3) iFrameTexType=0;
   if(iFrameTexType<0) iFrameTexType=3;
   if(gTexFrameName!=0)                              
    glDeleteTextures(1, &gTexFrameName);             
   gTexFrameName=0;                                  
   ulKeybits&=~(KEY_TOGGLEFBTEXTURE|KEY_STEPDOWN);
  }

 if(ulKeybits & KEY_TOGGLEFBREAD)
  {
   if(ulKeybits&KEY_STEPDOWN)
        iFrameReadType--;
   else iFrameReadType++;
   if(iFrameReadType>4) iFrameReadType=0;
   if(iFrameReadType<0) iFrameReadType=4;
   if(iFrameReadType==4) bFullVRam=TRUE;
   else                  bFullVRam=FALSE;
   iRenderFVR=0;
   ulKeybits&=~(KEY_TOGGLEFBREAD|KEY_STEPDOWN);
  }
#endif
}

////////////////////////////////////////////////////////////////////////
