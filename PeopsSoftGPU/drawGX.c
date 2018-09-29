/***************************************************************************
    drawGX.m
    PeopsSoftGPU for cubeSX/wiiSX
  
    Created by sepp256 on Thur Jun 26 2008.
    Copyright (c) 2008 Gil Pedersen.
    Adapted from draw.c by Pete Bernet and drawgl.m by Gil Pedersen.
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

#ifdef __SWITCH__
#include <switch.h>
#else
#include <gccore.h>
#endif
#include <malloc.h>
#include <time.h>
#include "stdafx.h"
#define _IN_DRAW
#include "externals.h"
#include "gpu.h"
#include "draw.h"
#include "prim.h"
#include "menu.h"
#include "../Gamecube/DEBUG.h"
#include "../Gamecube/wiiSXconfig.h"

////////////////////////////////////////////////////////////////////////////////////
// misc globals
////////////////////////////////////////////////////////////////////////////////////
int            iResX;
int            iResY;
s32           lLowerpart;
BOOL           bIsFirstFrame = TRUE;
BOOL           bCheckMask=FALSE;
unsigned short sSetMask=0;
u32  lSetMask=0;
int            iDesktopCol=16;
int            iShowFPS=1;
int            iWinSize;
int            iUseScanLines=0;
int            iUseNoStretchBlt=0;
int            iFastFwd=0;
int            iDebugMode=0;
int            iFVDisplay=0;
PSXPoint_t     ptCursorPoint[8];
unsigned short usCursorActive=0;

//Some GX specific variables
#define RESX_MAX 1024	//Vmem width
#define RESY_MAX 512	//Vmem height
int		iResX_Max=RESX_MAX;
int		iResY_Max=RESY_MAX;
static unsigned char	Xpixels[RESX_MAX*RESY_MAX*4] __attribute__((aligned(32)));
char *	pCaptionText;

#ifdef __SWITCH__
#include <EGL/egl.h>    // EGL library
#include <EGL/eglext.h> // EGL extensions
#include <glad/glad.h>  // glad library (OpenGL loader)

extern GLuint s_tex;
extern void sceneRender();
#endif

extern time_t tStart;
extern char text[DEBUG_TEXT_HEIGHT][DEBUG_TEXT_WIDTH]; /*** DEBUG textbuffer ***/
extern char menuActive;
extern char screenMode;

void BlitScreen32(unsigned char * surf,s32 x,s32 y)  // BLIT IN 32bit COLOR MODE
{
	unsigned char * pD;
	unsigned int startxy;
	u32 lu;unsigned short s;
	unsigned short row,column;
	unsigned short dx=PreviousPSXDisplay.Range.x1;
	unsigned short dy=PreviousPSXDisplay.DisplayMode.y;
	s32 lPitch=(dx+PreviousPSXDisplay.Range.x0)<<2;

	if(PreviousPSXDisplay.Range.y0)                       // centering needed?
	{
		surf+=PreviousPSXDisplay.Range.y0*lPitch;
		dy-=PreviousPSXDisplay.Range.y0;
	}

	surf+=PreviousPSXDisplay.Range.x0<<2;

	if(PSXDisplay.RGB24)
	{
		for(column=0;column<dy;column++)
		{
			startxy=((1024)*(column+y))+x;
			pD=(unsigned char *)&psxVuw[startxy];

			for(row=0;row<dx;row++)
			{
				lu=*((u32 *)pD);
				*((u32 *)((surf)+(column*lPitch)+(row<<2)))=
				0xff000000|(RED(lu)<<16)|(GREEN(lu)<<8)|(BLUE(lu));
				pD+=3;
			}
		}
	}
	else
	{
		for(column=0;column<dy;column++)
		{
			startxy=((1024)*(column+y))+x;
			for(row=0;row<dx;row++)
			{
				s=psxVuw[startxy++];
				*((u32 *)((surf)+(column*lPitch)+(row<<2)))=
				((((s<<19)&0xf80000)|((s<<6)&0xf800)|((s>>7)&0xf8))&0xffffff)|0xff000000;
			}
		}
	}
}


void DoBufferSwap(void)                                // SWAP BUFFERS
{                                                      // (we don't swap... we blit only)
	static int iOldDX=0;
	static int iOldDY=0;
	s32 x = PSXDisplay.DisplayPosition.x;
	s32 y = PSXDisplay.DisplayPosition.y;
	short iDX = PreviousPSXDisplay.Range.x1;
	short iDY = PreviousPSXDisplay.DisplayMode.y;
	
	//SysPrintf("DoBufferSwap iDX %i iDY %i\n", iDX, iDY);
	if (menuActive) return;

/*     
 // TODO: visual rumble
  if(iRumbleTime) 
   {
    ScreenRect.left+=((rand()*iRumbleVal)/RAND_MAX)-(iRumbleVal/2); 
    ScreenRect.right+=((rand()*iRumbleVal)/RAND_MAX)-(iRumbleVal/2); 
    ScreenRect.top+=((rand()*iRumbleVal)/RAND_MAX)-(iRumbleVal/2); 
    ScreenRect.bottom+=((rand()*iRumbleVal)/RAND_MAX)-(iRumbleVal/2); 
    iRumbleTime--;
   }
*/
	//	For now stretch only using GX
	if(iOldDX!=iDX || iOldDY!=iDY)
	{
		memset(Xpixels,0,iResY_Max*iResX_Max*4);
		iOldDX=iDX;iOldDY=iDY;
	}
	BlitScreen32((u8 *)Xpixels, x, y);
	if(ulKeybits&KEY_SHOWFPS) //DisplayText();               // paint menu text
	{
		if(szDebugText[0] && ((time(NULL) - tStart) < 2))
		{
			strcpy(szDispBuf,szDebugText);
		}
		else
		{
			szDebugText[0]=0;
			strcat(szDispBuf,szMenuBuf);
		}
	}

	glBindTexture(GL_TEXTURE_2D, s_tex);  
	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iDX, iDY, 0, GL_BGRA, GL_UNSIGNED_BYTE, (unsigned char *) Xpixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	sceneRender();
}

////////////////////////////////////////////////////////////////////////

void DoClearScreenBuffer(void)                         // CLEAR DX BUFFER
{
	memset(Xpixels,0,iResY_Max*iResX_Max*4);
}

////////////////////////////////////////////////////////////////////////

void DoClearFrontBuffer(void)                          // CLEAR DX BUFFER
{
	if (menuActive) return;

	// clear the screen, and flush it
	memset(Xpixels,0,iResY_Max*iResX_Max*4);
	glBindTexture(GL_TEXTURE_2D, s_tex);  
	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 640, 480, 0, GL_BGRA, GL_UNSIGNED_BYTE, (unsigned char *) Xpixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	sceneRender();
}


////////////////////////////////////////////////////////////////////////

u32 ulInitDisplay(void)
{
	bUsingTWin=FALSE;
	InitMenu();
	bIsFirstFrame = FALSE;                                // done

	if(iShowFPS)
	{
		iShowFPS=0;
		ulKeybits|=KEY_SHOWFPS;
		szDispBuf[0]=0;
		BuildDispMenu(0);
	}
	memset(Xpixels,0,iResX_Max*iResY_Max*2);

	return (u32)Xpixels;		//This isn't right, but didn't want to return 0..
}

////////////////////////////////////////////////////////////////////////

void CloseDisplay(void)
{

}

////////////////////////////////////////////////////////////////////////

void CreatePic(unsigned char * pMem)
{
}

///////////////////////////////////////////////////////////////////////////////////////

void DestroyPic(void)
{
}

///////////////////////////////////////////////////////////////////////////////////////

void DisplayPic(void)
{
}

///////////////////////////////////////////////////////////////////////////////////////

void ShowGpuPic(void)
{
}

///////////////////////////////////////////////////////////////////////////////////////

void ShowTextGpuPic(void)
{
}

///////////////////////////////////////////////////////////////////////
