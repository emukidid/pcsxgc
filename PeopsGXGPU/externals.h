/***************************************************************************
                          external.h  -  description
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
#include "gl.h"
/////////////////////////////////////////////////////////////////////////////

#define MIRROR_TEST 1

/////////////////////////////////////////////////////////////////////////////

#define SCISSOR_TEST 1
         
/////////////////////////////////////////////////////////////////////////////

// for own sow/tow scaling
#define OWNSCALE 1

/////////////////////////////////////////////////////////////////////////////

#define CLUTUSED     0x80000000

#ifndef __GX__
#define SETCOL(x)  if(x.c.lcol!=ulOLDCOL) {ulOLDCOL=x.c.lcol;glColor4ubv(x.c.col);} 
#define SETPCOL(x)  if(x->c.lcol!=ulOLDCOL) {ulOLDCOL=x->c.lcol;glColor4ubv(x->c.col);}
#else
#define SETCOL(x)  if(x.c.lcol!=ulOLDCOL) {ulOLDCOL=x.c.lcol;} 
#define SETPCOL(x)  if(x->c.lcol!=ulOLDCOL) {ulOLDCOL=x->c.lcol;}
#endif

#define GL_TO_EDGE_CLAMP              0x812F

#define INFO_TW        0
#define INFO_DRAWSTART 1
#define INFO_DRAWEND   2
#define INFO_DRAWOFF   3

#define SIGNSHIFT 21
#define CHKMAX_X 1024
#define CHKMAX_Y 512

/////////////////////////////////////////////////////////////////////////////

// GPU STATUS REGISTER bit values (c) Lewpy

#define DR_NORMAL 0
#define DR_VRAMTRANSFER 1

#define GPUSTATUS_ODDLINES            0x80000000
#define GPUSTATUS_DMABITS             0x60000000 // Two bits
#define GPUSTATUS_READYFORCOMMANDS    0x10000000
#define GPUSTATUS_READYFORVRAM        0x08000000
#define GPUSTATUS_IDLE                0x04000000
#define GPUSTATUS_DISPLAYDISABLED     0x00800000
#define GPUSTATUS_INTERLACED          0x00400000
#define GPUSTATUS_RGB24               0x00200000
#define GPUSTATUS_PAL                 0x00100000
#define GPUSTATUS_DOUBLEHEIGHT        0x00080000
#define GPUSTATUS_WIDTHBITS           0x00070000 // Three bits
#define GPUSTATUS_MASKENABLED         0x00001000
#define GPUSTATUS_MASKDRAWN           0x00000800
#define GPUSTATUS_DRAWINGALLOWED      0x00000400
#define GPUSTATUS_DITHER              0x00000200

#define STATUSREG lGPUstatusRet

#define GPUIsBusy (STATUSREG &= ~GPUSTATUS_IDLE)
#define GPUIsIdle (STATUSREG |= GPUSTATUS_IDLE)

#define GPUIsNotReadyForCommands (STATUSREG &= ~GPUSTATUS_READYFORCOMMANDS)
#define GPUIsReadyForCommands (STATUSREG |= GPUSTATUS_READYFORCOMMANDS)

/////////////////////////////////////////////////////////////////////////////

#define KEY_RESETTEXSTORE   1
#define KEY_SHOWFPS         2
#define KEY_RESETOPAQUE     4
#define KEY_RESETDITHER     8
#define KEY_RESETFILTER     16
#define KEY_RESETADVBLEND   32
#define KEY_BLACKWHITE      64
#define KEY_TOGGLEFBTEXTURE 128
#define KEY_STEPDOWN        256
#define KEY_TOGGLEFBREAD    512

/////////////////////////////////////////////////////////////////////////////
// Linux is missing some stuff, eh? ;)

#ifndef _WINDOWS
#include <gccore.h>
#define FALSE 0
#define TRUE  1
#define LOWORD(l)           ((unsigned short)(l))
#define HIWORD(l)           ((unsigned short)(((u32)(l) >> 16) & 0xFFFF))
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#define DWORD u32
#define __int64 s32 s32 int 

typedef struct RECTTAG
{
 int left;
 int top;
 int right;
 int bottom;
}RECT;

#endif

/////////////////////////////////////////////////////////////////////////////

typedef struct VRAMLOADTAG
{
 short x;
 short y;
 short Width;
 short Height;
 short RowsRemaining;
 short ColsRemaining;
 unsigned short *ImagePtr;
} VRAMLoad_t;

typedef struct PSXPOINTTAG
{
 s32 x;
 s32 y;
} PSXPoint_t;

typedef struct PSXSPOINTTAG
{
 short x;
 short y;
} PSXSPoint_t;

typedef struct PSXRECTTAG
{
 short x0;
 short x1;
 short y0;
 short y1;
} PSXRect_t;

/////////////////////////////////////////////////////////////////////////////

typedef struct TWINTAG
{
 PSXRect_t  Position;
 PSXRect_t  OPosition;
 PSXPoint_t TextureSize;
 float      UScaleFactor;
 float      VScaleFactor;
} TWin_t;

/////////////////////////////////////////////////////////////////////////////

typedef struct PSXDISPLAYTAG
{
 PSXPoint_t  DisplayModeNew;
 PSXPoint_t  DisplayMode;
 PSXPoint_t  DisplayPosition;
 PSXPoint_t  DisplayEnd;
 
 s32        Double;
 s32        Height;
 s32        PAL;
 s32        InterlacedNew;
 s32        Interlaced;
 s32        InterlacedTest;
 s32        RGB24New;
 s32        RGB24;
 PSXSPoint_t DrawOffset;
 PSXRect_t   DrawArea;
 PSXPoint_t  GDrawOffset;
 PSXPoint_t  CumulOffset;
 s32        Disabled;
 PSXRect_t   Range;
} PSXDisplay_t;

/////////////////////////////////////////////////////////////////////////////

typedef struct OGLVertexTag 
{
 float x;
 float y;
 float z;

 float sow;
 float tow;

 union
COLTAG
  {
   unsigned char col[4];
   u32 lcol;
  } c;

} OGLVertex;

typedef union EXShortTag
{
 unsigned char  c[2];
 unsigned short s;
} EXShort;

typedef union EXLongTag
{
 unsigned char c[4];
 u32 l;
 EXShort       s[2];
} EXLong;


/////////////////////////////////////////////////////////////////////////////

#ifdef _WINDOWS

extern HINSTANCE hInst;

#endif
                                                      
//-----------------------------------------------------//

#ifndef _IN_DRAW

extern int            iResX;
extern int            iResY;
extern BOOL           bKeepRatio;
extern RECT           rRatioRect;
extern BOOL           bSnapShot;
extern BOOL           bSmallAlpha;
extern BOOL           bOpaquePass;
extern BOOL           bAdvancedBlend;
extern BOOL           bUseLines;
extern int            iTexQuality;
extern BOOL           bUseAntiAlias;
extern BOOL           bGLExt;
extern BOOL           bGLFastMovie;
extern BOOL           bGLSoft;
extern BOOL           bGLBlend;

#ifndef __GX__
extern PFNGLBLENDEQU      glBlendEquationEXTEx;
extern PFNGLCOLORTABLEEXT glColorTableEXTEx;
#else
extern bool     glBlendEquationEXTEx;
extern bool 	glColorTableEXTEx;
#endif

extern unsigned char  gl_ux[8];
extern unsigned char  gl_vy[8];
extern OGLVertex      vertex[4];
extern short          sprtY,sprtX,sprtH,sprtW;
#ifdef _WINDOWS
extern HWND           hWWindow;
#endif
extern BOOL           bIsFirstFrame;
extern int            iWinSize;
extern int            iZBufferDepth;
extern GLbitfield     uiBufferBits;
extern int            iUseMask;
extern int            iSetMask;
extern int            iDepthFunc;
extern BOOL           bCheckMask;
extern unsigned short sSetMask;
extern u32  lSetMask;
extern int            iShowFPS;
extern BOOL           bSetClip;
extern int            iForceVSync;
extern int            iUseExts;
extern int            iUsePalTextures;
extern u32  gTexScanName;

#endif

//-----------------------------------------------------//

#ifndef _IN_SOFT

extern s32           GlobalTextAddrX,GlobalTextAddrY,GlobalTextTP;
extern s32           GlobalTextREST,GlobalTextABR,GlobalTextPAGE;
extern short          ly0,lx0,ly1,lx1,ly2,lx2,ly3,lx3;
extern short          g_m1;
extern short          g_m2;
extern short          g_m3;
extern short          DrawSemiTrans;

#endif

//-----------------------------------------------------//

#ifndef _IN_PRIMDRAW
#include "texture.h"
extern BOOL          bNeedUploadTest;
extern BOOL          bNeedUploadAfter;
extern BOOL          bTexEnabled;
extern BOOL          bBlendEnable;
#ifdef __GX__
extern int           iDither;
#define bDrawDither  iDither
#else
extern BOOL          bDrawDither;
#endif
extern int           iFilterType; 
extern BOOL          bFullVRam;
extern BOOL          bUseMultiPass;
extern int           iOffscreenDrawing;
extern BOOL          bOldSmoothShaded;
extern BOOL          bUsingTWin;
extern BOOL          bUsingMovie;
extern PSXRect_t     xrMovieArea;
extern PSXRect_t     xrUploadArea;
extern PSXRect_t     xrUploadAreaIL;
extern PSXRect_t     xrUploadAreaRGB24;
extern textureWndCacheEntry*        gTexName;
extern BOOL          bDrawNonShaded;
extern BOOL          bDrawMultiPass;
extern unsigned char ubGloColAlpha;
extern unsigned char ubGloAlpha;
extern short         sSprite_ux2;
extern short         sSprite_vy2;
extern BOOL          bRenderFrontBuffer;
extern u32 ulOLDCOL;
extern u32 ulClutID;
extern void (*primTableJ[256])(unsigned char *);
extern void (*primTableSkip[256])(unsigned char *);
extern unsigned short  usMirror;
extern u32 dwCfgFixes;
extern u32 dwActFixes;
extern u32 dwEmuFixes;
extern BOOL          bUseFixes;
extern int           iSpriteTex;
extern int           iDrawnSomething;

extern s32 drawX;
extern s32 drawY;
extern s32 drawW;
extern s32 drawH;
extern short sxmin;
extern short sxmax;
extern short symin;
extern short symax;

#endif

//-----------------------------------------------------//

#ifndef _IN_TEXTURE
#include "texture.h"
extern unsigned char  ubOpaqueDraw;
extern GLint          giWantedRGBA;
extern GLint          giWantedFMT;
extern GLint          giWantedTYPE;
extern void           (*LoadSubTexFn) (int,int,short,short);
extern s32           GlobalTexturePage;
extern u32  (*TCF[]) (u32);
extern unsigned short (*PTCF[]) (unsigned short);
extern u32  (*PalTexturedColourFn) (u32);
extern BOOL           bUseFastMdec;
extern BOOL           bUse15bitMdec;
extern int            iFrameTexType;
extern int            iFrameReadType;
extern int            iClampType;
extern int            iSortTexCnt;
extern BOOL           bFakeFrontBuffer; 
extern textureWndCacheEntry*         gTexFrameName;
extern textureWndCacheEntry*         gTexBlurName;
extern int            iVRamSize;
extern int            iTexGarbageCollection;
extern int            iFTexA;
extern int            iFTexB;
extern int            iHiResTextures;
extern BOOL           bIgnoreNextTile;


#endif

//-----------------------------------------------------//

#ifndef _IN_GPU

extern VRAMLoad_t     VRAMWrite;
extern VRAMLoad_t     VRAMRead;
extern int            iDataWriteMode;
extern int            iDataReadMode;
extern int            iColDepth;
extern BOOL           bChangeRes;
extern BOOL           bWindowMode;
extern char           szDispBuf[];
extern char           szGPUKeys[];
extern PSXDisplay_t   PSXDisplay;
extern PSXDisplay_t   PreviousPSXDisplay;
extern u32  ulKeybits;
extern TWin_t         TWin;
extern BOOL           bDisplayNotSet;
extern s32           lGPUstatusRet;
extern short          imageX0,imageX1;
extern short          imageY0,imageY1;
extern s32           lClearOnSwap,lClearOnSwapColor;
extern unsigned char  * psxVub;
extern signed char    * psxVsb;
extern unsigned short * psxVuw;
extern signed short   * psxVsw;
extern u32  * psxVul;
extern s32  * psxVsl;
extern GLfloat        gl_z;
extern BOOL           bNeedRGB24Update;
extern BOOL           bChangeWinMode;
extern GLuint         uiScanLine;
extern int            iUseScanLines;
extern s32           lSelectedSlot;
extern int            iScanBlend;
extern BOOL           bInitCap;
extern int            iBlurBuffer;
extern int            iLastRGB24;
extern int            iRenderFVR;
extern int            iNoScreenSaver;
extern u32  ulGPUInfoVals[];
extern BOOL           bNeedInterlaceUpdate;
extern BOOL           bNeedWriteUpload;
extern BOOL           bSkipNextFrame;


#ifndef _WINDOWS
extern int bFullScreen;
#endif

#endif

//-----------------------------------------------------//

#ifndef _IN_MENU

extern u32  dwCoreFlags;
extern GLuint         gTexPicName;
extern PSXPoint_t     ptCursorPoint[];
extern unsigned short usCursorActive;

#endif

//-----------------------------------------------------//

#ifndef _IN_CFG

#ifndef _WINDOWS
extern char * pConfigFile;
#endif

#endif

//-----------------------------------------------------//

#ifndef _IN_FPS

extern BOOL           bUseFrameLimit;
extern BOOL           bUseFrameSkip;
extern float          fFrameRate;
extern float          fFrameRateHz;
extern int            iFrameLimit;
extern float          fps_skip;
extern float          fps_cur;

#endif

//-----------------------------------------------------//

#ifndef _IN_KEY

extern u32  ulKeybits;

#endif

//-----------------------------------------------------//

#ifndef _IN_ZN

extern u32 dwGPUVersion;
extern int           iGPUHeight;
extern int           iGPUHeightMask;
extern int           GlobalTextIL;
extern int           iTileCheat;

#endif

//-----------------------------------------------------//
