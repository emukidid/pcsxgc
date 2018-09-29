/***************************************************************************
                          texture.c  -  description
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
  
////////////////////////////////////////////////////////////////////////////////////
// Texture related functions are here !
//
// The texture handling is heart and soul of this gpu. The plugin was developed
// 1999, by this time no shaders were available. Since the psx gpu is making
// heavy use of CLUT (="color lookup tables", aka palettized textures), it was 
// an interesting task to get those emulated at good speed on NV TNT cards 
// (which was my major goal when I created the first "gpuPeteTNT"). Later cards 
// (Geforce256) supported texture palettes by an OGL extension, but at some point
// this support was dropped again by gfx card vendors.
// Well, at least there is a certain advatage, if no texture palettes extension can
// be used: it is possible to modify the textures in any way, allowing "hi-res" 
// textures and other tweaks.
//
// My main texture caching is kinda complex: the plugin is allocating "n" 256x256 textures,
// and it places small psx texture parts inside them. The plugin keeps track what 
// part (with what palette) it had placed in which texture, so it can re-use this 
// part again. The more ogl textures it can use, the better (of course the managing/
// searching will be slower, but everything is faster than uploading textures again
// and again to a gfx card). My first card (TNT1) had 16 MB Vram, and it worked
// well with many games, but I recommend nowadays 64 MB Vram to get a good speed.
//
// Sadly, there is also a second kind of texture cache needed, for "psx texture windows".
// Those are "repeated" textures, so a psx "texture window" needs to be put in 
// a whole texture to use the GL_TEXTURE_WRAP_ features. This cache can get full very
// fast in games which are having an heavy "texture window" usage, like RRT4. As an 
// alternative, this plugin can use the OGL "palette" extension on texture windows, 
// if available. Nowadays also a fragment shader can easily be used to emulate
// texture wrapping in a texture atlas, so the main cache could hold the texture
// windows as well (that's what I am doing in the OGL2 plugin). But currently the
// OGL1 plugin is a "shader-free" zone, so heavy "texture window" games will cause
// much texture uploads.
//
// Some final advice: take care if you change things in here. I've removed my ASM
// handlers (they didn't cause much speed gain anyway) for readability/portability,
// but still the functions/data structures used here are easy to mess up. I guess it
// can be a pain in the ass to port the plugin to another byte order :)
//
////////////////////////////////////////////////////////////////////////////////////
 
#define _IN_TEXTURE

#ifdef __GX__
#include <gccore.h>
#include <ogc/lwp_heap.h>
#include <ogc/machine/processor.h>
#ifdef HW_RVL
#include "../Gamecube/MEM2.h"
#else
#include "../Gamecube/ARAM.h"
#endif
#include "../Gamecube/DEBUG.h"
#include "swap.h"
#include <malloc.h>
extern GXRModeObj *vmode;
extern void SysPrintf(char *fmt, ...);
#endif

#include "externals.h"
#include "texture.h"
#include "gpu.h"
#include "prim.h"

#define CLUTCHK   0x00060000
#define CLUTSHIFT 17

////////////////////////////////////////////////////////////////////////
// texture conversion buffer .. 
////////////////////////////////////////////////////////////////////////

int           iHiResTextures=0;
GLubyte       ubPaletteBuffer[256][4];
textureWndCacheEntry*        gTexMovieName=NULL;
textureWndCacheEntry*        gTexBlurName =NULL;
textureWndCacheEntry*        gTexFrameName=NULL;
int           iTexGarbageCollection=1;
u32 dwTexPageComp=0;
int           iVRamSize=0;
int           iClampType=GX_CLAMP;

void               (*LoadSubTexFn) (int,int,short,short);
u32      (*PalTexturedColourFn)  (u32);

////////////////////////////////////////////////////////////////////////
// defines
////////////////////////////////////////////////////////////////////////

#define PALCOL(x) PalTexturedColourFn (x)

#define CSUBSIZE  2048
#define CSUBSIZEA 8192
#define CSUBSIZES 4096

#define OFFA 0
#define OFFB 2048
#define OFFC 4096
#define OFFD 6144

#define XOFFA 0
#define XOFFB 512
#define XOFFC 1024
#define XOFFD 1536

#define SOFFA 0
#define SOFFB 1024
#define SOFFC 2048
#define SOFFD 3072

#define MAXWNDTEXCACHE 128

#define XCHECK(pos1,pos2) ((pos1.c[0]>=pos2.c[1])&&(pos1.c[1]<=pos2.c[0])&&(pos1.c[2]>=pos2.c[3])&&(pos1.c[3]<=pos2.c[2]))
#define INCHECK(pos2,pos1) ((pos1.c[0]<=pos2.c[0]) && (pos1.c[1]>=pos2.c[1]) && (pos1.c[2]<=pos2.c[2]) && (pos1.c[3]>=pos2.c[3]))

////////////////////////////////////////////////////////////////////////

unsigned char * CheckTextureInSubSCache(s32 TextureMode,u32 GivenClutId,unsigned short * pCache);
void            LoadSubTexturePageSort(int pageid, int mode, short cx, short cy);
void            LoadPackedSubTexturePageSort(int pageid, int mode, short cx, short cy);
void            DefineSubTextureSort(void);

////////////////////////////////////////////////////////////////////////
// some globals
////////////////////////////////////////////////////////////////////////

GLint giWantedRGBA=4;
GLint giWantedFMT=GL_RGBA;
GLint giWantedTYPE=GL_UNSIGNED_BYTE;
s32  GlobalTexturePage;
GLint XTexS;
GLint YTexS;
GLint DXTexS;
GLint DYTexS;
int   iSortTexCnt=32;
BOOL  bUseFastMdec=FALSE;
BOOL  bUse15bitMdec=FALSE;
int   iFrameTexType=0;
int   iFrameReadType=0;

u32  (*TCF[2]) (u32);
unsigned short (*PTCF[2]) (unsigned short);

////////////////////////////////////////////////////////////////////////
// texture cache implementation
////////////////////////////////////////////////////////////////////////

#ifdef _WINDOWS
#pragma pack(1)
#endif


#ifdef _WINDOWS
#pragma pack()
#endif

//---------------------------------------------

#define MAXTPAGES_MAX  64
#define MAXSORTTEX_MAX 196

//---------------------------------------------
#ifdef __GX__
heap_cntrl* GXtexCache;
#define malloc_gx(size) __lwp_heap_allocate(GXtexCache,size)
#define free_gx(ptr) __lwp_heap_free(GXtexCache,ptr)
#endif //__GX__

textureWndCacheEntry     wcWndtexStore[MAXWNDTEXCACHE];
textureSubCacheEntryS*   pscSubtexStore[3][MAXTPAGES_MAX];
EXLong *                 pxSsubtexLeft [MAXSORTTEX_MAX];
textureWndCacheEntry*    uiStexturePage[MAXSORTTEX_MAX];

unsigned short           usLRUTexPage=0;

int                      iMaxTexWnds=0;
int                      iTexWndTurn=0;
int                      iTexWndLimit=MAXWNDTEXCACHE/2;

GLubyte *                texturepart=NULL;
GLubyte *                texturebuffer=NULL;
u32            g_x1,g_y1,g_x2,g_y2;
unsigned char            ubOpaqueDraw=0;

unsigned short MAXTPAGES     = 32;
unsigned short CLUTMASK      = 0x7fff;
unsigned short CLUTYMASK     = 0x1ff;
unsigned short MAXSORTTEX    = 196;

//////////////////////
// GX Texture stuff //
//////////////////////

inline u32 GXGetRGBA8888_RGBA8( u64 *src, u16 x, u16 i, u8 palette )
{
//set palette = 0 for AR texels and palette = 1 for GB texels
	u32 c = ((u32*)src)[x^i]; // 0xAARRGGBB
	u16 color = (palette & 1) ? /* GGBB */ (u16) (c & 0xFFFF) : /* AARR */ (u16) ((c >> 16) & 0xFFFF);
	return (u32) color;
}


void createGXTexture(textureWndCacheEntry *entry, u32 width, u32 height) {

	u32 GXsize = 4;
	entry->GXtexfmt = GX_TF_RGBA8;
	//If realWidth or realHeight do not match the GX texture block size, then specify a compatible blocksize
	int blockWidth = (GXsize == 1) ? 8 : 4;
	int blockHeight = 4;
	if(width % blockWidth || width == 0)
	{
		entry->GXrealWidth = width + blockWidth - (width % blockWidth);
	}
	else
		entry->GXrealWidth = width;
	if(height % blockHeight || height == 0)
	{
		entry->GXrealHeight = height + blockHeight - (height % blockHeight);
	}
	else
		entry->GXrealHeight = height;

	u32 textureBytes = (entry->GXrealWidth * entry->GXrealHeight) * GXsize;
	
	if (textureBytes > 0)
	{
		entry->GXtexture = (u16*)malloc_gx(textureBytes);
		memset(entry->GXtexture, 0, textureBytes);	// TODO remove this.
	}
	GX_InitTexObj(&entry->GXtexObj, entry->GXtexture, entry->GXrealWidth
		, entry->GXrealHeight, entry->GXtexfmt, iClampType, iClampType, GX_FALSE); 
}

void updateGXSubTexture(textureWndCacheEntry *entry, u8 *rgba8888Tex, u32 xStartPos, u32 yStartPos,
                 u32 width, u32 height) {

	//SysPrintf("Update texture: tex size %i,%i update at %i,%i with %i,%i\r\n"
	//		, entry->GXrealWidth, entry->GXrealHeight, xStartPos, yStartPos, width, height);
	u8 *dest = NULL;
	u32	k, l;
	u8 *src;
	u32 bpl;
	u32 x, y, j, tx, ty;

	dest = (u8*)entry->GXtexture;
	
	bpl = width << 3 >> 1;

	u32 gxBytesPerLine = (256 / 4) * 64;
	u32 gxBytesAtLineStart = (xStartPos / 4) * 64;
	u32 roundedWidth = width % 4 == 0 ? width : (width + (4-width%4));
	u32 gxBytesActualTex = (roundedWidth / 4) * 64;
	dest+= gxBytesPerLine * (yStartPos / 4);
	
	u16 clampSClamp = width - 1;
	u16 clampTClamp = height - 1;
	
	for (y = 0; y < height; y+=4)
	{
		dest += gxBytesAtLineStart;
		for (x = 0; x < width; x+=4)
		{
			j = 0;
			__asm__ volatile("dcbz %y0" : "=Z"(dest[0]) :: "memory");
			__asm__ volatile("dcbz %y0" : "=Z"(dest[32]) :: "memory");
			for (k = 0; k < 4; k++)
			{
				ty = min(y+k, clampTClamp);
				src = &rgba8888Tex[(bpl * ty)];
				for (l = 0; l < 4; l++)
				{
					tx = min(x+l, clampSClamp);
					((u16*)dest)[j] =		(u16) GXGetRGBA8888_RGBA8( (u64*)src, tx, 0, 0 );	// AARR texels
					((u16*)dest)[j+16] =	(u16) GXGetRGBA8888_RGBA8( (u64*)src, tx, 0, 1 );	// GGBB texels -> next 32B block
					j++;
				}
			}
			__asm__ volatile("dcbf %y0" : "=Z"(dest[0]) :: "memory");
			__asm__ volatile("dcbf %y0" : "=Z"(dest[32]) :: "memory");
			dest += 64;
		}
		if(width != entry->GXrealWidth)
			dest += (gxBytesPerLine - (gxBytesAtLineStart+gxBytesActualTex));
		_sync();
	}
	
	//if(width == 17 && height == 241) {
	//	char filename[1024];
	//	memset(filename, 0, 1024);
	//	sprintf(filename, "sd:/orig_%i_%i_%i_%i.raw",  0, 0, width, height);
	//	FILE* f = fopen(filename, "wb" );
	//	fwrite(texturepart, 1, 256*256*4, f);
	//	fclose(f);
	//	
	//	memset(filename, 0, 1024);
	//	sprintf(filename, "sd:/tex_%i_%i_%i_%i.raw",  0, 0, 256, 256);
	//	f = fopen(filename, "wb" );
	//	fwrite(gTexName->GXtexture, 1, 256*256*4, f);
	//	fclose(f);
	//	SysPrintf("Done!\r\n");
	//	exit(1);
	//}
}

////////////////////////////////////////////////////////////////////////
// Texture color conversions... all my ASM funcs are removed for easier
// porting... and honestly: nowadays the speed gain would be pointless 
////////////////////////////////////////////////////////////////////////

u32 XP8RGBA(u32 BGR)
{
	SysPrintf("1\r\n");
 if(!(BGR&0xffff)) return SWAP32(0x50000000);
 if(DrawSemiTrans && !(BGR&0x8000)) 
  {ubOpaqueDraw=1;return SWAP32(((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff));}
 return SWAP32(((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000);
}

u32 XP8RGBAEx(u32 BGR)
{
	SysPrintf("2\r\n");
 if(!(BGR&0xffff)) return SWAP32(0x03000000);
 if(DrawSemiTrans && !(BGR&0x8000)) 
  {ubOpaqueDraw=1;return SWAP32(((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff));}
 return SWAP32(((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000);
}

u32 CP8RGBA(u32 BGR)
{
	SysPrintf("3\r\n");
 u32 l;
 if(!(BGR&0xffff)) return SWAP32(0x50000000);
 if(DrawSemiTrans && !(BGR&0x8000)) 
  {ubOpaqueDraw=1;return SWAP32(((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff));}
 l=((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000;
 if(l==0xffffff00) l=0xff000000;
 return SWAP32(l);
}

u32 CP8RGBAEx(u32 BGR)
{
	SysPrintf("4\r\n");
 u32 l;
 if(!(BGR&0xffff)) return SWAP32(0x03000000);
 if(DrawSemiTrans && !(BGR&0x8000)) 
  {ubOpaqueDraw=1;return SWAP32(((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff));}
 l=((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000;
 if(l==0xffffff00) l=0xff000000;
 return SWAP32(l);
}

#define CLUT_BLUE(v) ((v & 0x7C00)>>10)
#define CLUT_GREEN(v) ((v & 0x03E0)>>5)
#define CLUT_RED(v) ((v & 0x001F))
u32 XP8RGBA_0(u32 BGR) {
 BGR = SWAP16(BGR);
 if(!(BGR&0xffff)) return 0x50000000;

 u32 ret = 0xFF000000|(((CLUT_RED(BGR) * 255) / 31)<<16)
	|(((CLUT_GREEN(BGR)* 255) / 31)<<8)|(((CLUT_BLUE(BGR)* 255) / 31));
 return ret;
}

u32 XP8RGBAEx_0(u32 BGR)
{
//SysPrintf("6\r\n");
 BGR = SWAP16(BGR);
 if(!(BGR&0xffff)) return 0x03000000;
 u32 ret = 0xFF000000|(((CLUT_RED(BGR) * 255) / 31)<<16)
	|(((CLUT_GREEN(BGR)* 255) / 31)<<8)|(((CLUT_BLUE(BGR)* 255) / 31));
 return ret;
}

u32 XP8BGRA_0(u32 BGR)
{
	SysPrintf("7\r\n");
 if(!(BGR&0xffff)) return SWAP32(0x50000000);
 return SWAP32(((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff)|0xff000000);
}

u32 XP8BGRAEx_0(u32 BGR)
{
	SysPrintf("8\r\n");
 if(!(BGR&0xffff)) return SWAP32(0x03000000);
 return SWAP32(((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff)|0xff000000);
}

u32 CP8RGBA_0(u32 BGR)
{
	SysPrintf("9\r\n");
 u32 l;

 if(!(BGR&0xffff)) return SWAP32(0x50000000);
 l=((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000;
 if(l==0xfff8f800) l=0xff000000;
 return SWAP32(l);
}

u32 CP8RGBAEx_0(u32 BGR)
{
	SysPrintf("10\r\n");
 u32 l;

 if(!(BGR&0xffff)) return SWAP32(0x03000000);
 l=((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000;
 if(l==0xfff8f800) l=0xff000000;
 return SWAP32(l);
}

u32 CP8BGRA_0(u32 BGR)
{
	SysPrintf("11\r\n");
 u32 l;

 if(!(BGR&0xffff)) return SWAP32(0x50000000);
 l=((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff)|0xff000000;
 if(l==0xff00f8f8) l=0xff000000;
 return SWAP32(l);
}

u32 CP8BGRAEx_0(u32 BGR)
{
	SysPrintf("12\r\n");
 u32 l;

 if(!(BGR&0xffff)) return SWAP32(0x03000000);
 l=((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff)|0xff000000;
 if(l==0xff00f8f8) l=0xff000000;
 return SWAP32(l);
}

u32 XP8RGBA_1(u32 BGR)
{
 //SysPrintf("13\r\n");
 BGR= SWAP16(BGR);
 if(!(BGR&0xffff)) return (0x50000000);
 u32 ret = (((CLUT_RED(BGR) * 255) / 31)<<16)|(((CLUT_GREEN(BGR)* 255) / 31)<<8)|((CLUT_BLUE(BGR)* 255)/31); 
 if(!(BGR&0x8000)) {ubOpaqueDraw=1;return ret;}
 return (ret|0xff000000);
}

u32 XP8RGBAEx_1(u32 BGR)
{
	//SysPrintf("14\r\n");
 BGR=SWAP16(BGR);
 if(!(BGR&0xffff)) return 0x03000000;
 u32 ret = (((CLUT_RED(BGR) * 255) / 31)<<16)
	|(((CLUT_GREEN(BGR)* 255) / 31)<<8)|(((CLUT_BLUE(BGR)* 255) / 31));
 if(!(BGR&0x8000)) {ubOpaqueDraw=1;return ret;}
 return ret|0xff000000;
}

u32 XP8BGRA_1(u32 BGR)
{
	SysPrintf("15\r\n");
 if(!(BGR&0xffff)) return SWAP32(0x50000000);
 if(!(BGR&0x8000)) {ubOpaqueDraw=1;return SWAP32(((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff));}
 return SWAP32(((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff)|0xff000000);
}

u32 XP8BGRAEx_1(u32 BGR)
{
	SysPrintf("16\r\n");
 if(!(BGR&0xffff)) return SWAP32(0x03000000);
 if(!(BGR&0x8000)) {ubOpaqueDraw=1;return SWAP32(((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff));}
 return SWAP32(((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff)|0xff000000);
}

u32 P8RGBA(u32 BGR)
{
 if(!(BGR&0xffff)) return 0;
	SysPrintf("17\r\n");
	BGR=SWAP16(BGR);
	 u32 ret = 0xFF000000|(((CLUT_RED(BGR) * 255) / 31)<<16)
	|(((CLUT_GREEN(BGR)* 255) / 31)<<8)|(((CLUT_BLUE(BGR)* 255) / 31));
 return ret|0xFF000000;//SWAP32(((((BGR<<3)&0xf8)|((BGR<<6)&0xf800)|((BGR<<9)&0xf80000))&0xffffff)|0xff000000);
}

u32 P8BGRA(u32 BGR)
{
	SysPrintf("18\r\n");
 if(!(BGR&0xffff)) return 0;
 return SWAP32(((((BGR>>7)&0xf8)|((BGR<<6)&0xf800)|((BGR<<19)&0xf80000))&0xffffff)|0xff000000);
}

unsigned short XP5RGBA(unsigned short BGR)
{
	SysPrintf("19\r\n");
 if(!BGR) return 0;
 if(DrawSemiTrans && !(BGR&0x8000)) 
  {ubOpaqueDraw=1;return ((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)));}
 return ((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)))|1;
}

unsigned short XP5RGBA_0 (unsigned short BGR)
{
	SysPrintf("20\r\n");
 if(!BGR) return 0;

 return ((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)))|1;
}

unsigned short CP5RGBA_0 (unsigned short BGR)
{
	SysPrintf("21\r\n");
 unsigned short s;

 if(!BGR) return 0;

 s=((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)))|1;
 if(s==0x07ff) s=1;
 return s;
}

unsigned short XP5RGBA_1(unsigned short BGR)
{
	SysPrintf("22\r\n");
 if(!BGR) return 0;
 if(!(BGR&0x8000)) 
  {ubOpaqueDraw=1;return ((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)));}
 return ((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)))|1;
}

unsigned short P5RGBA(unsigned short BGR)
{
	SysPrintf("23\r\n");
 if(!BGR) return 0;
 return ((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)))|1;
}

unsigned short XP4RGBA(unsigned short BGR)
{
	SysPrintf("24\r\n");
 if(!BGR) return 6;
 if(DrawSemiTrans && !(BGR&0x8000)) 
  {ubOpaqueDraw=1;return ((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)));}
 return (((((BGR&0x1e)<<11))|((BGR&0x7800)>>7)|((BGR&0x3c0)<<2)))|0xf;
}

unsigned short XP4RGBA_0 (unsigned short BGR)
{
	SysPrintf("25\r\n");
 if(!BGR) return 6;
 return (((((BGR&0x1e)<<11))|((BGR&0x7800)>>7)|((BGR&0x3c0)<<2)))|0xf;
}

unsigned short CP4RGBA_0 (unsigned short BGR)
{
	SysPrintf("26\r\n");
 unsigned short s;
 if(!BGR) return 6;
 s=(((((BGR&0x1e)<<11))|((BGR&0x7800)>>7)|((BGR&0x3c0)<<2)))|0xf;
 if(s==0x0fff) s=0x000f;
 return s;
}

unsigned short XP4RGBA_1(unsigned short BGR)
{
	SysPrintf("27\r\n");
 if(!BGR) return 6;
 if(!(BGR&0x8000)) 
  {ubOpaqueDraw=1;return ((((BGR<<11))|((BGR>>9)&0x3e)|((BGR<<1)&0x7c0)));}
 return (((((BGR&0x1e)<<11))|((BGR&0x7800)>>7)|((BGR&0x3c0)<<2)))|0xf;
}

unsigned short P4RGBA(unsigned short BGR)
{
	SysPrintf("28\r\n");
 if(!BGR) return 0;
 return (((((BGR&0x1e)<<11))|((BGR&0x7800)>>7)|((BGR&0x3c0)<<2)))|0xf;
}

////////////////////////////////////////////////////////////////////////
// CHECK TEXTURE MEM (on plugin startup)
////////////////////////////////////////////////////////////////////////

int iFTexA=512;
int iFTexB=512;

void CheckTextureMemory(void)
{
 GLboolean b;GLboolean * bDetail;
 int i,iCnt,iRam=iVRamSize*1024*1024;
 int iTSize;char * p;
 
 if(iBlurBuffer)
  {
   if(iResX>1024) iFTexA=2048;
   else
   if(iResX>512)  iFTexA=1024;
   else           iFTexA=512;
   if(iResY>1024) iFTexB=2048;
   else
   if(iResY>512)  iFTexB=1024;
   else           iFTexB=512;
#ifndef __GX__
   glGenTextures(1, &gTexBlurName->texname);
   gTexName=gTexBlurName;
   glBindTexture(GL_TEXTURE_2D, gTexName->texname);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   char * p=(char *)malloc(iFTexA*iFTexB*4);
   memset(p,0,iFTexA*iFTexB*4);
   glTexImage2D(GL_TEXTURE_2D, 0, 3, iFTexA, iFTexB, 0, GL_RGB, GL_UNSIGNED_BYTE, p);
   free(p);
   glGetError();
#else
   if(gTexBlurName==NULL)
    {
     gTexBlurName=malloc(sizeof(textureWndCacheEntry));
     memset(gTexBlurName, 0, sizeof(textureWndCacheEntry));
    }
   gTexName=gTexBlurName;
   createGXTexture(gTexName, iFTexA, iFTexB);
   GX_InitTexObjFilterMode(&gTexName->GXtexObj,GX_NEAR,GX_NEAR);
   GX_InitTexObjWrapMode(&gTexName->GXtexObj,GX_CLAMP,GX_CLAMP);
#endif
   iRam-=iFTexA*iFTexB*3;
   iFTexA=(iResX*256)/iFTexA;
   iFTexB=(iResY*256)/iFTexB;
  }

 if(iVRamSize)
  {
   int ts;

   iRam-=(iResX*iResY*8);
   iRam-=(iResX*iResY*(iZBufferDepth/8));

   if(iTexQuality==0 || iTexQuality==3) ts=4;
   else                                 ts=2;

   if(iHiResTextures)
        iSortTexCnt=iRam/(512*512*ts);
   else iSortTexCnt=iRam/(256*256*ts);

   if(iSortTexCnt>MAXSORTTEX) 
    {
     iSortTexCnt=MAXSORTTEX-min(1,iHiResTextures);
    }
   else
    {
     iSortTexCnt-=3+min(1,iHiResTextures);
     if(iSortTexCnt<8) iSortTexCnt=8;
    }

   for(i=0;i<MAXSORTTEX;i++)
    uiStexturePage[i]=NULL;
   return;
  }


 if(iHiResTextures) iTSize=512;
 else               iTSize=256;
 p=(char *)malloc(iTSize*iTSize*4);

 iCnt=0;
#ifndef __GX__
 // TODO fix broken uiStexturePage stuff here
 glGenTextures(MAXSORTTEX,uiStexturePage);
 for(i=0;i<MAXSORTTEX;i++)
  {
   glBindTexture(GL_TEXTURE_2D,uiStexturePage[i]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, iClampType);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, iClampType);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexImage2D(GL_TEXTURE_2D, 0, giWantedRGBA, iTSize, iTSize, 0,GL_RGBA, giWantedTYPE, p);
  }
 glBindTexture(GL_TEXTURE_2D,0);
#else

#endif
 free(p);

 bDetail=malloc(MAXSORTTEX*sizeof(GLboolean));
 memset(bDetail,0,MAXSORTTEX*sizeof(GLboolean));
#ifndef __GX__
 b=glAreTexturesResident(MAXSORTTEX,uiStexturePage,bDetail);

 glDeleteTextures(MAXSORTTEX,uiStexturePage);
#endif
 for(i=0;i<MAXSORTTEX;i++)
  {
   if(bDetail[i]) iCnt++;
   uiStexturePage[i]=NULL;
  }

 free(bDetail);

 if(b) iSortTexCnt=MAXSORTTEX-min(1,iHiResTextures);
 else  iSortTexCnt=iCnt-3+min(1,iHiResTextures);       // place for menu&texwnd

 if(iSortTexCnt<8) iSortTexCnt=8;
} 

////////////////////////////////////////////////////////////////////////
// Main init of textures
////////////////////////////////////////////////////////////////////////

void InitializeTextureStore() 
{
 int i,j;

#ifdef __GX__
 //Init texture cache heap if not yet inited
 if(!GXtexCache)
 {
  GXtexCache = (heap_cntrl*)memalign(32, sizeof(heap_cntrl));
  __lwp_heap_init(GXtexCache, TEXCACHE_LO,TEXCACHE_SIZE, 32);
 }
#endif //__GX__
 
 if(iGPUHeight==1024)
  {
   MAXTPAGES     = 64;
   CLUTMASK      = 0xffff;
   CLUTYMASK     = 0x3ff;
   MAXSORTTEX    = 128;
   iTexGarbageCollection=0;
  }
 else
  {
   MAXTPAGES     = 32;
   CLUTMASK      = 0x7fff;
   CLUTYMASK     = 0x1ff;
   MAXSORTTEX    = 196;
  }

 memset(vertex,0,4*sizeof(OGLVertex));                 // init vertices

 gTexName=NULL;                                        // init main tex name

 iTexWndLimit=MAXWNDTEXCACHE;
 if(!iUsePalTextures) iTexWndLimit/=2;

 memset(wcWndtexStore,0,sizeof(textureWndCacheEntry)*
                        MAXWNDTEXCACHE);
 texturepart=(GLubyte *)malloc(256*256*4);
 memset(texturepart,0,256*256*4);
 if(iHiResTextures)
 texturebuffer=(GLubyte *)malloc(512*512*4);
 else texturebuffer=NULL;

 for(i=0;i<3;i++)                                    // -> info for 32*3
  for(j=0;j<MAXTPAGES;j++)
   {                                               
    pscSubtexStore[i][j]=(textureSubCacheEntryS *)malloc_gx(CSUBSIZES*sizeof(textureSubCacheEntryS));
    memset(pscSubtexStore[i][j],0,CSUBSIZES*sizeof(textureSubCacheEntryS));
   }
 for(i=0;i<MAXSORTTEX;i++)                           // -> info 0..511
  {
   pxSsubtexLeft[i]=(EXLong *)malloc_gx(CSUBSIZE*sizeof(EXLong));
   memset(pxSsubtexLeft[i],0,CSUBSIZE*sizeof(EXLong));
   uiStexturePage[i]=NULL;
  }
}

////////////////////////////////////////////////////////////////////////
// Clean up on exit
////////////////////////////////////////////////////////////////////////

void CleanupTextureStore() 
{
 int i,j;textureWndCacheEntry * tsx;
#ifndef __GX__
 //----------------------------------------------------//
 glBindTexture(GL_TEXTURE_2D,0);
 //----------------------------------------------------//
#endif
 free(texturepart);                                    // free tex part
 texturepart=0;
 if(texturebuffer)
  {
   free(texturebuffer);
   texturebuffer=0;
  }
 //----------------------------------------------------//
 tsx=wcWndtexStore;                                    // loop tex window cache
 for(i=0;i<MAXWNDTEXCACHE;i++,tsx++)
  {
#ifndef __GX__
   if(tsx->texname)                                    // -> some tex?
    glDeleteTextures(1,&tsx->texname);                 // --> delete it
#else
	if(tsx->GXtexture)
	 free_gx(tsx->GXtexture);
#endif
  }
 iMaxTexWnds=0;                                        // no more tex wnds
 //----------------------------------------------------//
#ifndef __GX__
 if(gTexMovieName!=NULL)                               // some movie tex?
  glDeleteTextures(1, &gTexMovieName->texname);        // -> delete it
#else
 if(gTexMovieName != NULL && gTexMovieName->GXtexture)
  free_gx(gTexMovieName->GXtexture);
#endif
 gTexMovieName=NULL;                                   // no more movie tex
 //----------------------------------------------------//
#ifndef __GX__
 if(gTexFrameName!=NULL)                               // some 15bit framebuffer tex?
  glDeleteTextures(1, &gTexFrameName->texname);        // -> delete it
#else
 if(gTexFrameName != NULL && gTexFrameName->GXtexture)
  free_gx(gTexFrameName->GXtexture);
#endif
 gTexFrameName=NULL;                                   // no more movie tex
 //----------------------------------------------------//
#ifndef __GX__
 if(gTexBlurName!=NULL)                                // some 15bit framebuffer tex?
  glDeleteTextures(1, &gTexBlurName->texname);         // -> delete it
#else
 if(gTexBlurName != NULL && gTexBlurName->GXtexture)
  free_gx(gTexBlurName->GXtexture);
#endif
 gTexBlurName=NULL;                                    // no more movie tex
 //----------------------------------------------------//
 for(i=0;i<3;i++)                                    // -> loop
  for(j=0;j<MAXTPAGES;j++)                           // loop tex pages
   {
    free_gx(pscSubtexStore[i][j]);                      // -> clean mem
   }
 for(i=0;i<MAXSORTTEX;i++)
  {
   if(uiStexturePage[i] != NULL)                     // --> tex used ?
    {
#ifndef __GX__
     glDeleteTextures(1,&uiStexturePage[i]->texname);
#else
     if(uiStexturePage[i]->GXtexture)
      free_gx(uiStexturePage[i]->GXtexture);
#endif
     uiStexturePage[i]=NULL;                         // --> delete it
    }
   free_gx(pxSsubtexLeft[i]);                           // -> clean mem
  }
 //----------------------------------------------------//
}

////////////////////////////////////////////////////////////////////////
// Reset textures in game...
////////////////////////////////////////////////////////////////////////

void ResetTextureArea(BOOL bDelTex)
{
 int i,j;textureSubCacheEntryS * tss;EXLong * lu;
 textureWndCacheEntry * tsx;
 //----------------------------------------------------//

 dwTexPageComp=0;
#ifndef __GX__
 //----------------------------------------------------//
 if(bDelTex) {glBindTexture(GL_TEXTURE_2D,0);gTexName=NULL;}
 //----------------------------------------------------//
#else
 if(bDelTex) {if(gTexName->GXtexture != NULL) free_gx(gTexName->GXtexture); gTexName=NULL;}
#endif
 tsx=wcWndtexStore;
 for(i=0;i<MAXWNDTEXCACHE;i++,tsx++)
  {
   tsx->used=0;
   if(bDelTex && tsx->texname)
    {
#ifndef __GX__
     glDeleteTextures(1,&tsx->texname);
#else
	 if(tsx->GXtexture != NULL) free_gx(tsx->GXtexture);
#endif
     tsx->texname=0;
    }
  }
 iMaxTexWnds=0;
 //----------------------------------------------------//

 for(i=0;i<3;i++)
  for(j=0;j<MAXTPAGES;j++)
   {
    tss=pscSubtexStore[i][j];
    (tss+SOFFA)->pos.l=0;
    (tss+SOFFB)->pos.l=0;
    (tss+SOFFC)->pos.l=0;
    (tss+SOFFD)->pos.l=0;
   }

 for(i=0;i<iSortTexCnt;i++)
  {
   lu=pxSsubtexLeft[i];
   lu->l=0;
#ifndef __GX__
   if(bDelTex && uiStexturePage[i] != NULL)
    {glDeleteTextures(1,&uiStexturePage[i]->texname);uiStexturePage[i]=NULL;}
#else
   if(bDelTex && uiStexturePage[i] != NULL)
    {if(uiStexturePage[i]->GXtexture != NULL) free_gx(uiStexturePage[i]->GXtexture); uiStexturePage[i]=NULL;}
#endif
  }
}


////////////////////////////////////////////////////////////////////////
// Invalidate tex windows
////////////////////////////////////////////////////////////////////////

void InvalidateWndTextureArea(s32 X,s32 Y,s32 W, s32 H)
{
 int i,px1,px2,py1,py2,iYM=1;
 textureWndCacheEntry * tsw=wcWndtexStore;

 W+=X-1;      
 H+=Y-1;
 if(X<0) X=0;if(X>1023) X=1023;
 if(W<0) W=0;if(W>1023) W=1023;
 if(Y<0) Y=0;if(Y>iGPUHeightMask)  Y=iGPUHeightMask;
 if(H<0) H=0;if(H>iGPUHeightMask)  H=iGPUHeightMask;
 W++;H++;

 if(iGPUHeight==1024) iYM=3;

 py1=min(iYM,Y>>8);
 py2=min(iYM,H>>8);                                    // y: 0 or 1

 px1=max(0,(X>>6));
 px2=min(15,(W>>6));

 if(py1==py2)
  {
   py1=py1<<4;px1+=py1;px2+=py1;                       // change to 0-31
   for(i=0;i<iMaxTexWnds;i++,tsw++)
    {
     if(tsw->used)
      {
       if(tsw->pageid>=px1 && tsw->pageid<=px2)
        {
         tsw->used=0;
        }
      }
    }
  }
 else
  {
   py1=px1+16;py2=px2+16;
   for(i=0;i<iMaxTexWnds;i++,tsw++)
    {
     if(tsw->used)
      {
       if((tsw->pageid>=px1 && tsw->pageid<=px2) ||
          (tsw->pageid>=py1 && tsw->pageid<=py2))
        {
         tsw->used=0;
        }
      }
    }
  }

 // adjust tex window count
 tsw=wcWndtexStore+iMaxTexWnds-1;
 while(iMaxTexWnds && !tsw->used) {iMaxTexWnds--;tsw--;}
}



////////////////////////////////////////////////////////////////////////
// same for sort textures
////////////////////////////////////////////////////////////////////////

void MarkFree(textureSubCacheEntryS * tsx)
{
 EXLong * ul, * uls;
 int j,iMax;unsigned char x1,y1,dx,dy;

 uls=pxSsubtexLeft[tsx->cTexID];
 iMax=uls->l;ul=uls+1;

 if(!iMax) return;

 for(j=0;j<iMax;j++,ul++)
  if(ul->l==0xffffffff) break;

 if(j<CSUBSIZE-2)
  {
   if(j==iMax) uls->l=uls->l+1;

   x1=tsx->posTX;dx=tsx->pos.c[2]-tsx->pos.c[3];
   if(tsx->posTX) {x1--;dx+=3;}
   y1=tsx->posTY;dy=tsx->pos.c[0]-tsx->pos.c[1];
   if(tsx->posTY) {y1--;dy+=3;}

   ul->c[3]=x1;
   ul->c[2]=dx;
   ul->c[1]=y1;
   ul->c[0]=dy;
  }
}

void InvalidateSubSTextureArea(s32 X,s32 Y,s32 W, s32 H)
{

 int i,j,k,iMax,px,py,px1,px2,py1,py2,iYM=1;
 EXLong npos;textureSubCacheEntryS * tsb;
 s32 x1,x2,y1,y2,xa,sw;

 W+=X-1;      
 H+=Y-1;
 if(X<0) X=0;if(X>1023) X=1023;
 if(W<0) W=0;if(W>1023) W=1023;
 if(Y<0) Y=0;if(Y>iGPUHeightMask)  Y=iGPUHeightMask;
 if(H<0) H=0;if(H>iGPUHeightMask)  H=iGPUHeightMask;
 W++;H++;

 if(iGPUHeight==1024) iYM=3;

 py1=min(iYM,Y>>8);
 py2=min(iYM,H>>8);                                    // y: 0 or 1
 px1=max(0,(X>>6)-3);                                   
 px2=min(15,(W>>6)+3);                                 // x: 0-15
 return;
 for(py=py1;py<=py2;py++)
  {
   j=(py<<4)+px1;                                      // get page

   y1=py*256;y2=y1+255;

   if(H<y1)  continue;
   if(Y>y2)  continue;

   if(Y>y1)  y1=Y;
   if(H<y2)  y2=H;
   if(y2<y1) {sw=y1;y1=y2;y2=sw;}
   y1=((y1%256)<<8);
   y2=(y2%256);

   for(px=px1;px<=px2;px++,j++)
    {
     for(k=0;k<3;k++)
      {
       xa=x1=px<<6;
       if(W<x1) continue;
       x2=x1+(64<<k)-1;
       if(X>x2) continue;

       if(X>x1)  x1=X;
       if(W<x2)  x2=W;
       if(x2<x1) {sw=x1;x1=x2;x2=sw;}

       if (dwGPUVersion == 2)
        npos.l=0x00ff00ff;
       else
        npos.l=((x1-xa)<<(26-k))|((x2-xa)<<(18-k))|y1|y2;

        {
         tsb=pscSubtexStore[k][j]+SOFFA;iMax=tsb->pos.l;tsb++;
         for(i=0;i<iMax;i++,tsb++)
          if(tsb->ClutID && XCHECK(tsb->pos,npos)) {tsb->ClutID=0;MarkFree(tsb);}

//         if(npos.l & 0x00800000)
          {
           tsb=pscSubtexStore[k][j]+SOFFB;iMax=tsb->pos.l;tsb++;
           for(i=0;i<iMax;i++,tsb++)
            if(tsb->ClutID && XCHECK(tsb->pos,npos)) {tsb->ClutID=0;MarkFree(tsb);}
          }

//         if(npos.l & 0x00000080)
          {
           tsb=pscSubtexStore[k][j]+SOFFC;iMax=tsb->pos.l;tsb++;
           for(i=0;i<iMax;i++,tsb++)
            if(tsb->ClutID && XCHECK(tsb->pos,npos)) {tsb->ClutID=0;MarkFree(tsb);}
          }

//         if(npos.l & 0x00800080)
          {
           tsb=pscSubtexStore[k][j]+SOFFD;iMax=tsb->pos.l;tsb++;
           for(i=0;i<iMax;i++,tsb++)
            if(tsb->ClutID && XCHECK(tsb->pos,npos)) {tsb->ClutID=0;MarkFree(tsb);}
          }
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////
// Invalidate some parts of cache: main routine
////////////////////////////////////////////////////////////////////////

void InvalidateTextureAreaEx(void)
{
 short W=sxmax-sxmin;
 short H=symax-symin;

 if(W==0 && H==0) return;

 if(iMaxTexWnds) 
  InvalidateWndTextureArea(sxmin,symin,W,H);

 InvalidateSubSTextureArea(sxmin,symin,W,H);
}

////////////////////////////////////////////////////////////////////////

void InvalidateTextureArea(s32 X,s32 Y,s32 W, s32 H)
{
 if(W==0 && H==0) return;

 if(iMaxTexWnds) InvalidateWndTextureArea(X,Y,W,H); 

 InvalidateSubSTextureArea(X,Y,W,H);
}


////////////////////////////////////////////////////////////////////////
// tex window: define
////////////////////////////////////////////////////////////////////////

void DefineTextureWnd(void)
{
#ifndef __GX__
 if(gTexName==NULL) {
  gTexName=malloc(sizeof(textureWndCacheEntry));
  memset(gTexName, 0, sizeof(textureWndCacheEntry));
  glGenTextures(1, &gTexName->texname);
 }

 glBindTexture(GL_TEXTURE_2D, gTexName->texname);

 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
 
 if(iFilterType && iFilterType<3 && iHiResTextures!=2)
  {
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
 else
  {
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

 glTexImage2D(GL_TEXTURE_2D, 0,giWantedRGBA, 
              TWin.Position.x1, 
              TWin.Position.y1, 
              0, giWantedFMT, giWantedTYPE, texturepart);
#else
 if(gTexName==NULL)
 {
  gTexName=malloc(sizeof(textureWndCacheEntry));
  memset(gTexName, 0, sizeof(textureWndCacheEntry));
 }
  u8 magFilt = iFilterType && iFilterType<3 && iHiResTextures!=2 ? GX_LINEAR : GX_NEAR;
  createGXTexture(gTexName, TWin.Position.x1, TWin.Position.y1);
  updateGXSubTexture(gTexName, texturepart, 0, 0, TWin.Position.x1, TWin.Position.y1);
  GX_InitTexObjFilterMode(&gTexName->GXtexObj,magFilt,magFilt);
  GX_InitTexObjWrapMode(&gTexName->GXtexObj,GX_REPEAT,GX_REPEAT);
  GX_LoadTexObj(&gTexName->GXtexObj, GX_TEXMAP0);
#endif
}

////////////////////////////////////////////////////////////////////////
// tex window: load packed stretch
////////////////////////////////////////////////////////////////////////

void LoadStretchPackedWndTexturePage(int pageid, int mode, short cx, short cy)
{
 u32 start,row,column,j,sxh,sxm,ldx,ldy,ldxo;
 unsigned int   palstart;
 unsigned short *px,*pa,*ta;
 unsigned char  *cSRCPtr,*cOSRCPtr;
 unsigned short *wSRCPtr,*wOSRCPtr;
 u32  LineOffset;unsigned short s;
 int pmult=pageid/16;
 unsigned short (*LPTCOL)(unsigned short);

 LPTCOL=PTCF[DrawSemiTrans];

 ldxo=TWin.Position.x1-TWin.OPosition.x1;
 ldy =TWin.Position.y1-TWin.OPosition.y1;

 pa=px=(unsigned short *)ubPaletteBuffer;
 ta=(unsigned short *)texturepart;
 palstart=cx+(cy*1024);

 ubOpaqueDraw=0;

 switch(mode)
  {
   //--------------------------------------------------// 
   // 4bit texture load ..
   case 0:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;
      for(row=0;row<16;row++)
       *px++=LPTCOL(*wSRCPtr++);

      column=g_y2-ldy;
      for(TXV=g_y1;TXV<=column;TXV++)
       {
        ldx=ldxo;
        for(TXU=g_x1;TXU<=g_x2-ldxo;TXU++)
         {
  		  n_xi = ( ( TXU >> 2 ) & ~0x3c ) + ( ( TXV << 2 ) & 0x3c );
		  n_yi = ( TXV & ~0xf ) + ( ( TXU >> 4 ) & 0xf );

          s=pa[(psxVuw[((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi] >> ( ( TXU & 0x03 ) << 2 ) ) & 0x0f];
          *ta++=s;

          if(ldx) {*ta++=s;ldx--;}
         }

        if(ldy) 
         {ldy--;
          for(TXU=g_x1;TXU<=g_x2;TXU++)
           *ta++=ta[-(g_x2-g_x1)];
         }
       }
      DefineTextureWnd();
      break;
     }


    start=((pageid-16*pmult)*128)+256*2048*pmult;

    // convert CLUT to 32bits .. and then use THAT as a lookup table

    wSRCPtr=psxVuw+palstart;
    for(row=0;row<16;row++)
     *px++=LPTCOL(*wSRCPtr++);

    sxm=g_x1&1;sxh=g_x1>>1;
    if(sxm) j=g_x1+1; else j=g_x1;
    cSRCPtr = psxVub + start + (2048*g_y1) + sxh;
    for(column=g_y1;column<=g_y2;column++)
     {
      cOSRCPtr=cSRCPtr;ldx=ldxo;
      if(sxm) *ta++=pa[(*cSRCPtr++ >> 4) & 0xF];
      
      for(row=j;row<=g_x2-ldxo;row++)
       {
        s=pa[*cSRCPtr & 0xF];
        *ta++=s;
        if(ldx) {*ta++=s;ldx--;}
        row++;
        if(row<=g_x2-ldxo) 
         {
          s=pa[(*cSRCPtr >> 4) & 0xF];
          *ta++=s; 
          if(ldx) {*ta++=s;ldx--;}
         }
        cSRCPtr++;
       }

      if(ldy && column&1) 
           {ldy--;cSRCPtr = cOSRCPtr;}
      else cSRCPtr = psxVub + start + (2048*(column+1)) + sxh;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // 8bit texture load ..
   case 1:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;
      for(row=0;row<256;row++)
       *px++=LPTCOL(*wSRCPtr++);

      column=g_y2-ldy;
      for(TXV=g_y1;TXV<=column;TXV++)
       {
        ldx=ldxo;
        for(TXU=g_x1;TXU<=g_x2-ldxo;TXU++)
         {
  		  n_xi = ( ( TXU >> 1 ) & ~0x78 ) + ( ( TXU << 2 ) & 0x40 ) + ( ( TXV << 3 ) & 0x38 );
		  n_yi = ( TXV & ~0x7 ) + ( ( TXU >> 5 ) & 0x7 );

          s=pa[(psxVuw[((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi] >> ( ( TXU & 0x01 ) << 3 )) & 0xff];

          *ta++=s;
          if(ldx) {*ta++=s;ldx--;}
         }

        if(ldy) 
         {ldy--;
          for(TXU=g_x1;TXU<=g_x2;TXU++)
           *ta++=ta[-(g_x2-g_x1)];
         }
       }
      DefineTextureWnd();
      break;
     }

    start=((pageid-16*pmult)*128)+256*2048*pmult;

    // not using a lookup table here... speeds up smaller texture areas
    cSRCPtr = psxVub + start + (2048*g_y1) + g_x1;
    LineOffset = 2048 - (g_x2-g_x1+1) +ldxo; 

    for(column=g_y1;column<=g_y2;column++)
     {
      cOSRCPtr=cSRCPtr;ldx=ldxo;
      for(row=g_x1;row<=g_x2-ldxo;row++)
       {
        s=LPTCOL(psxVuw[palstart+ *cSRCPtr++]);
        *ta++=s;
        if(ldx) {*ta++=s;ldx--;}
       }
      if(ldy && column&1) {ldy--;cSRCPtr=cOSRCPtr;}
      else                cSRCPtr+=LineOffset;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // 16bit texture load ..
   case 2:
    start=((pageid-16*pmult)*64)+256*1024*pmult;
    wSRCPtr = psxVuw + start + (1024*g_y1) + g_x1;
    LineOffset = 1024 - (g_x2-g_x1+1) +ldxo; 
                                
    for(column=g_y1;column<=g_y2;column++)
     {
      wOSRCPtr=wSRCPtr;ldx=ldxo;
      for(row=g_x1;row<=g_x2-ldxo;row++)
       {
        s=LPTCOL(*wSRCPtr++);
        *ta++=s;
        if(ldx) {*ta++=s;ldx--;}
       }
      if(ldy && column&1) {ldy--;wSRCPtr=wOSRCPtr;}
      else                 wSRCPtr+=LineOffset;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // others are not possible !
  }
}

////////////////////////////////////////////////////////////////////////
// tex window: load stretched
////////////////////////////////////////////////////////////////////////

void LoadStretchWndTexturePage(int pageid, int mode, short cx, short cy)
{
 u32 start,row,column,j,sxh,sxm,ldx,ldy,ldxo,s;
 unsigned int   palstart;
 u32  *px,*pa,*ta;
 unsigned char  *cSRCPtr,*cOSRCPtr;
 unsigned short *wSRCPtr,*wOSRCPtr;
 u32  LineOffset;
 int pmult=pageid/16;
 u32 (*LTCOL)(u32);
 
 LTCOL=TCF[DrawSemiTrans];

 ldxo=TWin.Position.x1-TWin.OPosition.x1;
 ldy =TWin.Position.y1-TWin.OPosition.y1;

 pa=px=(u32 *)ubPaletteBuffer;
 ta=(u32 *)texturepart;
 palstart=cx+(cy*1024);

 ubOpaqueDraw=0;

 switch(mode)
  {
   //--------------------------------------------------// 
   // 4bit texture load ..
   case 0:
    //------------------- ZN STUFF
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;

      row=4;do
       {
        *px    =LTCOL(wSRCPtr[0]);
        *(px+1)=LTCOL(wSRCPtr[1]);
        *(px+2)=LTCOL(wSRCPtr[2]);
        *(px+3)=LTCOL(wSRCPtr[3]);
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      column=g_y2-ldy;
      for(TXV=g_y1;TXV<=column;TXV++)
       {
        ldx=ldxo;
        for(TXU=g_x1;TXU<=g_x2-ldxo;TXU++)
         {
  		  n_xi = ( ( TXU >> 2 ) & ~0x3c ) + ( ( TXV << 2 ) & 0x3c );
		  n_yi = ( TXV & ~0xf ) + ( ( TXU >> 4 ) & 0xf );

          s=pa[(psxVuw[((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi] >> ( ( TXU & 0x03 ) << 2 ) ) & 0x0f];
          *ta++=s;

          if(ldx) {*ta++=s;ldx--;}
         }

        if(ldy) 
         {ldy--;
          for(TXU=g_x1;TXU<=g_x2;TXU++)
           *ta++=ta[-(g_x2-g_x1)];
         }
       }

      DefineTextureWnd();

      break;
     }

    //-------------------

    start=((pageid-16*pmult)*128)+256*2048*pmult;
    // convert CLUT to 32bits .. and then use THAT as a lookup table

    wSRCPtr=psxVuw+palstart;
    for(row=0;row<16;row++)
     *px++=LTCOL(*wSRCPtr++);

    sxm=g_x1&1;sxh=g_x1>>1;
    if(sxm) j=g_x1+1; else j=g_x1;
    cSRCPtr = psxVub + start + (2048*g_y1) + sxh;
    for(column=g_y1;column<=g_y2;column++)
     {
      cOSRCPtr=cSRCPtr;ldx=ldxo;
      if(sxm) *ta++=pa[(*cSRCPtr++ >> 4) & 0xF];
      
      for(row=j;row<=g_x2-ldxo;row++)
       {
        s=*(pa+(*cSRCPtr & 0xF));
        *ta++=s;
        if(ldx) {*ta++=s;ldx--;}
        row++;
        if(row<=g_x2-ldxo) 
         {
          s=pa[(*cSRCPtr >> 4) & 0xF];
          *ta++=s; 
          if(ldx) {*ta++=s;ldx--;}
         }
        cSRCPtr++;
       }
      if(ldy && column&1) 
           {ldy--;cSRCPtr = cOSRCPtr;}
      else cSRCPtr = psxVub + start + (2048*(column+1)) + sxh;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------//
   // 8bit texture load ..
   case 1:
    //------------ ZN STUFF
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;

      row=64;do
       {
        *px    =LTCOL(wSRCPtr[0]);
        *(px+1)=LTCOL(wSRCPtr[1]);
        *(px+2)=LTCOL(wSRCPtr[2]);
        *(px+3)=LTCOL(wSRCPtr[3]);
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      column=g_y2-ldy;
      for(TXV=g_y1;TXV<=column;TXV++)
       {
        ldx=ldxo;
        for(TXU=g_x1;TXU<=g_x2-ldxo;TXU++)
         {
  		  n_xi = ( ( TXU >> 1 ) & ~0x78 ) + ( ( TXU << 2 ) & 0x40 ) + ( ( TXV << 3 ) & 0x38 );
		  n_yi = ( TXV & ~0x7 ) + ( ( TXU >> 5 ) & 0x7 );

          s=pa[(psxVuw[((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi] >> ( ( TXU & 0x01 ) << 3 ) ) & 0xff];
          *ta++=s;
          if(ldx) {*ta++=s;ldx--;}
         }

        if(ldy) 
         {ldy--;
          for(TXU=g_x1;TXU<=g_x2;TXU++)
           *ta++=ta[-(g_x2-g_x1)];
         }

       }

      DefineTextureWnd();

      break;
     }
    //------------

    start=((pageid-16*pmult)*128)+256*2048*pmult;    

    // not using a lookup table here... speeds up smaller texture areas
    cSRCPtr = psxVub + start + (2048*g_y1) + g_x1;
    LineOffset = 2048 - (g_x2-g_x1+1) +ldxo; 

    for(column=g_y1;column<=g_y2;column++)
     {
      cOSRCPtr=cSRCPtr;ldx=ldxo;
      for(row=g_x1;row<=g_x2-ldxo;row++)
       {
        s=LTCOL(psxVuw[palstart+ *cSRCPtr++]);
        *ta++=s;
        if(ldx) {*ta++=s;ldx--;}
       }
      if(ldy && column&1) {ldy--;cSRCPtr=cOSRCPtr;}
      else                cSRCPtr+=LineOffset;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // 16bit texture load ..
   case 2:
    start=((pageid-16*pmult)*64)+256*1024*pmult;

    wSRCPtr = psxVuw + start + (1024*g_y1) + g_x1;
    LineOffset = 1024 - (g_x2-g_x1+1) +ldxo; 

    for(column=g_y1;column<=g_y2;column++)
     {
      wOSRCPtr=wSRCPtr;ldx=ldxo;
      for(row=g_x1;row<=g_x2-ldxo;row++)
       {
        s=LTCOL(*wSRCPtr++);
        *ta++=s;
        if(ldx) {*ta++=s;ldx--;}
       }
      if(ldy && column&1) {ldy--;wSRCPtr=wOSRCPtr;}
      else                 wSRCPtr+=LineOffset;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // others are not possible !
  }
}

////////////////////////////////////////////////////////////////////////
// tex window: load packed simple
////////////////////////////////////////////////////////////////////////

void LoadPackedWndTexturePage(int pageid, int mode, short cx, short cy)
{
 u32 start,row,column,j,sxh,sxm;
 unsigned int   palstart;
 unsigned short *px,*pa,*ta;
 unsigned char  *cSRCPtr;
 unsigned short *wSRCPtr;
 u32  LineOffset;
 int pmult=pageid/16;
 unsigned short (*LPTCOL)(unsigned short);

 LPTCOL=PTCF[DrawSemiTrans];

 pa=px=(unsigned short *)ubPaletteBuffer;
 ta=(unsigned short *)texturepart;
 palstart=cx+(cy*1024);

 ubOpaqueDraw=0;

 switch(mode)
  {
   //--------------------------------------------------// 
   // 4bit texture load ..
   case 0:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;
      for(row=0;row<16;row++)
       *px++=LPTCOL(*wSRCPtr++);

      for(TXV=g_y1;TXV<=g_y2;TXV++)
       {
        for(TXU=g_x1;TXU<=g_x2;TXU++)
         {
  		  n_xi = ( ( TXU >> 2 ) & ~0x3c ) + ( ( TXV << 2 ) & 0x3c );
		  n_yi = ( TXV & ~0xf ) + ( ( TXU >> 4 ) & 0xf );

          *ta++=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x03 ) << 2 ) ) & 0x0f ));
         }
       }

      DefineTextureWnd();

      break;
     }

    start=((pageid-16*pmult)*128)+256*2048*pmult;

    // convert CLUT to 32bits .. and then use THAT as a lookup table

    wSRCPtr=psxVuw+palstart;
    for(row=0;row<16;row++)
     *px++=LPTCOL(*wSRCPtr++);

    sxm=g_x1&1;sxh=g_x1>>1;
    if(sxm) j=g_x1+1; else j=g_x1;
    cSRCPtr = psxVub + start + (2048*g_y1) + sxh;
    for(column=g_y1;column<=g_y2;column++)
     {
      cSRCPtr = psxVub + start + (2048*column) + sxh;
    
      if(sxm) *ta++=*(pa+((*cSRCPtr++ >> 4) & 0xF));
      
      for(row=j;row<=g_x2;row++)
       {
        *ta++=*(pa+(*cSRCPtr & 0xF)); row++;
        if(row<=g_x2) *ta++=*(pa+((*cSRCPtr >> 4) & 0xF)); 
        cSRCPtr++;
       }
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // 8bit texture load ..
   case 1:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;
      for(row=0;row<256;row++)
       *px++=LPTCOL(*wSRCPtr++);

      for(TXV=g_y1;TXV<=g_y2;TXV++)
       {
        for(TXU=g_x1;TXU<=g_x2;TXU++)
         {
  		  n_xi = ( ( TXU >> 1 ) & ~0x78 ) + ( ( TXU << 2 ) & 0x40 ) + ( ( TXV << 3 ) & 0x38 );
		  n_yi = ( TXV & ~0x7 ) + ( ( TXU >> 5 ) & 0x7 );

          *ta++=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x01 ) << 3 ) ) & 0xff));
         }
       }

      DefineTextureWnd();

      break;
     }

    start=((pageid-16*pmult)*128)+256*2048*pmult;

    // not using a lookup table here... speeds up smaller texture areas
    cSRCPtr = psxVub + start + (2048*g_y1) + g_x1;
    LineOffset = 2048 - (g_x2-g_x1+1); 

    for(column=g_y1;column<=g_y2;column++)
     {
      for(row=g_x1;row<=g_x2;row++)
       *ta++=LPTCOL(psxVuw[palstart+ *cSRCPtr++]);
      cSRCPtr+=LineOffset;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // 16bit texture load ..
   case 2:
    start=((pageid-16*pmult)*64)+256*1024*pmult;
    wSRCPtr = psxVuw + start + (1024*g_y1) + g_x1;
    LineOffset = 1024 - (g_x2-g_x1+1); 

    for(column=g_y1;column<=g_y2;column++)
     {
      for(row=g_x1;row<=g_x2;row++)
       *ta++=LPTCOL(*wSRCPtr++);
      wSRCPtr+=LineOffset;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // others are not possible !
  }
}

////////////////////////////////////////////////////////////////////////
// tex window: load simple
////////////////////////////////////////////////////////////////////////

void LoadWndTexturePage(int pageid, int mode, short cx, short cy)
{
 u32 start,row,column,j,sxh,sxm;
 unsigned int   palstart;
 u32  *px,*pa,*ta;
 unsigned char  *cSRCPtr;
 unsigned short *wSRCPtr;
 u32  LineOffset;
 int pmult=pageid/16;
 u32 (*LTCOL)(u32);
 
 LTCOL=TCF[DrawSemiTrans];

 pa=px=(u32 *)ubPaletteBuffer;
 ta=(u32 *)texturepart;
 palstart=cx+(cy*1024);

 ubOpaqueDraw=0;

 switch(mode)
  {
   //--------------------------------------------------// 
   // 4bit texture load ..
   case 0:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;

      row=4;do
       {
        *px    =LTCOL(*wSRCPtr);
        *(px+1)=LTCOL(*(wSRCPtr+1));
        *(px+2)=LTCOL(*(wSRCPtr+2));
        *(px+3)=LTCOL(*(wSRCPtr+3));
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      for(TXV=g_y1;TXV<=g_y2;TXV++)
       {
        for(TXU=g_x1;TXU<=g_x2;TXU++)
         {
  		  n_xi = ( ( TXU >> 2 ) & ~0x3c ) + ( ( TXV << 2 ) & 0x3c );
		  n_yi = ( TXV & ~0xf ) + ( ( TXU >> 4 ) & 0xf );

          *ta++=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x03 ) << 2 ) ) & 0x0f ));
         }
       }

      DefineTextureWnd();

      break;
     }

    start=((pageid-16*pmult)*128)+256*2048*pmult;

    // convert CLUT to 32bits .. and then use THAT as a lookup table

    wSRCPtr=psxVuw+palstart;
    for(row=0;row<16;row++)
     *px++=LTCOL(*wSRCPtr++);

    sxm=g_x1&1;sxh=g_x1>>1;
    if(sxm) j=g_x1+1; else j=g_x1;
    cSRCPtr = psxVub + start + (2048*g_y1) + sxh;
    for(column=g_y1;column<=g_y2;column++)
     {
      cSRCPtr = psxVub + start + (2048*column) + sxh;
    
      if(sxm) *ta++=*(pa+((*cSRCPtr++ >> 4) & 0xF));
      
      for(row=j;row<=g_x2;row++)
       {
        *ta++=*(pa+(*cSRCPtr & 0xF)); row++;
        if(row<=g_x2) *ta++=*(pa+((*cSRCPtr >> 4) & 0xF)); 
        cSRCPtr++;
       }
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------//
   // 8bit texture load ..
   case 1:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;

      row=64;do
       {
        *px    =LTCOL(*wSRCPtr);
        *(px+1)=LTCOL(*(wSRCPtr+1));
        *(px+2)=LTCOL(*(wSRCPtr+2));
        *(px+3)=LTCOL(*(wSRCPtr+3));
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      for(TXV=g_y1;TXV<=g_y2;TXV++)
       {
        for(TXU=g_x1;TXU<=g_x2;TXU++)
         {
  		  n_xi = ( ( TXU >> 1 ) & ~0x78 ) + ( ( TXU << 2 ) & 0x40 ) + ( ( TXV << 3 ) & 0x38 );
		  n_yi = ( TXV & ~0x7 ) + ( ( TXU >> 5 ) & 0x7 );

          *ta++=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x01 ) << 3 ) ) & 0xff));
         }
       }

      DefineTextureWnd();

      break;
     }

    start=((pageid-16*pmult)*128)+256*2048*pmult;

    // not using a lookup table here... speeds up smaller texture areas
    cSRCPtr = psxVub + start + (2048*g_y1) + g_x1;
    LineOffset = 2048 - (g_x2-g_x1+1); 

    for(column=g_y1;column<=g_y2;column++)
     {
      for(row=g_x1;row<=g_x2;row++)
       *ta++=LTCOL(psxVuw[palstart+ *cSRCPtr++]);
      cSRCPtr+=LineOffset;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // 16bit texture load ..
   case 2:
    start=((pageid-16*pmult)*64)+256*1024*pmult;

    wSRCPtr = psxVuw + start + (1024*g_y1) + g_x1;
    LineOffset = 1024 - (g_x2-g_x1+1); 

    for(column=g_y1;column<=g_y2;column++)
     {
      for(row=g_x1;row<=g_x2;row++)
       *ta++=LTCOL(*wSRCPtr++);
      wSRCPtr+=LineOffset;
     }

    DefineTextureWnd();
    break;
   //--------------------------------------------------// 
   // others are not possible !
  }
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void UploadTexWndPal(int mode,short cx,short cy)
{
 unsigned int i,iSize;
 unsigned short * wSrcPtr;
 u32 * ta=(u32 *)texturepart;

 wSrcPtr=psxVuw+cx+(cy*1024);
 if(mode==0) i=4; else i=64;
 iSize=i<<2;
 ubOpaqueDraw=0;

 do
  {
   *ta    =PALCOL(*wSrcPtr);
   *(ta+1)=PALCOL(*(wSrcPtr+1));
   *(ta+2)=PALCOL(*(wSrcPtr+2));
   *(ta+3)=PALCOL(*(wSrcPtr+3));
   ta+=4;wSrcPtr+=4;i--;
  }
 while(i);
#ifndef __GX__
 (*glColorTableEXTEx)(GL_TEXTURE_2D,GL_RGBA8,iSize,
                    GL_RGBA,GL_UNSIGNED_BYTE,texturepart);
#endif
}

////////////////////////////////////////////////////////////////////////

void DefinePalTextureWnd(void)
{
#ifndef __GX__
 if(gTexName==NULL) {
  gTexName=malloc(sizeof(textureWndCacheEntry));
  memset(gTexName, 0, sizeof(textureWndCacheEntry));
  glGenTextures(1, &gTexName->texname);
 }

 glBindTexture(GL_TEXTURE_2D, gTexName->texname);

 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
 
 if(iFilterType && iFilterType<3 && iHiResTextures!=2)
  {
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
 else
  {
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

 glTexImage2D(GL_TEXTURE_2D, 0,GL_COLOR_INDEX8_EXT, 
              TWin.Position.x1, 
              TWin.Position.y1, 
              0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE,texturepart);
#else
 if(gTexName==NULL)
 {
  gTexName=malloc(sizeof(textureWndCacheEntry));
  memset(gTexName, 0, sizeof(textureWndCacheEntry));
 }
  u8 magFilt = iFilterType && iFilterType<3 && iHiResTextures!=2 ? GX_LINEAR : GX_NEAR;
  createGXTexture(gTexName, TWin.Position.x1, TWin.Position.y1);
  updateGXSubTexture(gTexName, texturepart, 0, 0, TWin.Position.x1, TWin.Position.y1);
  GX_InitTexObjFilterMode(&gTexName->GXtexObj,magFilt,magFilt);
  GX_InitTexObjWrapMode(&gTexName->GXtexObj,GX_REPEAT,GX_REPEAT);
  GX_LoadTexObj(&gTexName->GXtexObj, GX_TEXMAP0);
#endif
}

///////////////////////////////////////////////////////

void LoadPalWndTexturePage(int pageid, int mode, short cx, short cy)
{
 u32 start,row,column,j,sxh,sxm;
 unsigned char  *ta;
 unsigned char  *cSRCPtr;
 u32  LineOffset;
 int pmult=pageid/16;

 ta=(unsigned char *)texturepart;

 switch(mode)
  {
   //--------------------------------------------------// 
   // 4bit texture load ..
   case 0:
    start=((pageid-16*pmult)*128)+256*2048*pmult;

    sxm=g_x1&1;sxh=g_x1>>1;
    if(sxm) j=g_x1+1; else j=g_x1;
    cSRCPtr = psxVub + start + (2048*g_y1) + sxh;
    for(column=g_y1;column<=g_y2;column++)
     {
      cSRCPtr = psxVub + start + (2048*column) + sxh;
    
      if(sxm) *ta++=((*cSRCPtr++ >> 4) & 0xF);
      
      for(row=j;row<=g_x2;row++)
       {
        *ta++=(*cSRCPtr & 0xF); row++;
        if(row<=g_x2) *ta++=((*cSRCPtr >> 4) & 0xF); 
        cSRCPtr++;
       }
     }

    DefinePalTextureWnd();
    break;
   //--------------------------------------------------// 
   // 8bit texture load ..
   case 1:
    start=((pageid-16*pmult)*128)+256*2048*pmult;

    // not using a lookup table here... speeds up smaller texture areas
    cSRCPtr = psxVub + start + (2048*g_y1) + g_x1;
    LineOffset = 2048 - (g_x2-g_x1+1); 

    for(column=g_y1;column<=g_y2;column++)
     {
      for(row=g_x1;row<=g_x2;row++)
       *ta++=*cSRCPtr++;
      cSRCPtr+=LineOffset;
     }

    DefinePalTextureWnd();
    break;
  }
 UploadTexWndPal(mode,cx,cy);
}

////////////////////////////////////////////////////////////////////////

void LoadStretchPalWndTexturePage(int pageid, int mode, short cx, short cy)
{
 u32 start,row,column,j,sxh,sxm,ldx,ldy,ldxo;
 unsigned char  *ta,s;
 unsigned char  *cSRCPtr,*cOSRCPtr;
 u32  LineOffset;
 int pmult=pageid/16;

 ldxo=TWin.Position.x1-TWin.OPosition.x1;
 ldy =TWin.Position.y1-TWin.OPosition.y1;

 ta=(unsigned char *)texturepart;

 switch(mode)
  {
   //--------------------------------------------------// 
   // 4bit texture load ..
   case 0:
    start=((pageid-16*pmult)*128)+256*2048*pmult;

    sxm=g_x1&1;sxh=g_x1>>1;
    if(sxm) j=g_x1+1; else j=g_x1;
    cSRCPtr = psxVub + start + (2048*g_y1) + sxh;
    for(column=g_y1;column<=g_y2;column++)
     {
      cOSRCPtr=cSRCPtr;ldx=ldxo;
      if(sxm) *ta++=((*cSRCPtr++ >> 4) & 0xF);
      
      for(row=j;row<=g_x2-ldxo;row++)
       {
        s=(*cSRCPtr & 0xF);
        *ta++=s;
        if(ldx) {*ta++=s;ldx--;}
        row++;
        if(row<=g_x2-ldxo) 
         {
          s=((*cSRCPtr >> 4) & 0xF);
          *ta++=s; 
          if(ldx) {*ta++=s;ldx--;}
         }
        cSRCPtr++;
       }
      if(ldy && column&1) 
           {ldy--;cSRCPtr = cOSRCPtr;}
      else cSRCPtr = psxVub + start + (2048*(column+1)) + sxh;
     }

    DefinePalTextureWnd();
    break;
   //--------------------------------------------------// 
   // 8bit texture load ..
   case 1:
    start=((pageid-16*pmult)*128)+256*2048*pmult;

    cSRCPtr = psxVub + start + (2048*g_y1) + g_x1;
    LineOffset = 2048 - (g_x2-g_x1+1) +ldxo; 

    for(column=g_y1;column<=g_y2;column++)
     {
      cOSRCPtr=cSRCPtr;ldx=ldxo;
      for(row=g_x1;row<=g_x2-ldxo;row++)
       {
        s=*cSRCPtr++;
        *ta++=s;
        if(ldx) {*ta++=s;ldx--;}
       }
      if(ldy && column&1) {ldy--;cSRCPtr=cOSRCPtr;}
      else                cSRCPtr+=LineOffset;
     }

    DefinePalTextureWnd();
    break;
  }
 UploadTexWndPal(mode,cx,cy);
}

////////////////////////////////////////////////////////////////////////
// tex window: main selecting, cache handler included
////////////////////////////////////////////////////////////////////////

textureWndCacheEntry* LoadTextureWnd(s32 pageid,s32 TextureMode,u32 GivenClutId)
{
 textureWndCacheEntry * ts, * tsx=NULL;
 int i;short cx,cy;
 EXLong npos;

 npos.c[3]=TWin.Position.x0;
 npos.c[2]=TWin.OPosition.x1;
 npos.c[1]=TWin.Position.y0;
 npos.c[0]=TWin.OPosition.y1;

 g_x1=TWin.Position.x0;g_x2=g_x1+TWin.Position.x1-1;
 g_y1=TWin.Position.y0;g_y2=g_y1+TWin.Position.y1-1;

 if(TextureMode==2) {GivenClutId=0;cx=cy=0;}
 else  
  {
   cx=((GivenClutId << 4) & 0x3F0);cy=((GivenClutId >> 6) & CLUTYMASK);
   GivenClutId=(GivenClutId&CLUTMASK)|(DrawSemiTrans<<30);

   // palette check sum
    {
     u32 l=0,row;
     u32 * lSRCPtr=(u32 *)(psxVuw+cx+(cy*1024));
     if(TextureMode==1) for(row=1;row<129;row++) l+=((*lSRCPtr++)-1)*row;
     else               for(row=1;row<9;row++)   l+=((*lSRCPtr++)-1)<<row;
     l=(l+HIWORD(l))&0x3fffL;
     GivenClutId|=(l<<16);
    }

  }

 ts=wcWndtexStore;

 for(i=0;i<iMaxTexWnds;i++,ts++)
  {
   if(ts->used)
    {
     if(ts->pos.l==npos.l &&
        ts->pageid==pageid &&
        ts->textureMode==TextureMode)
      {
       if(ts->ClutID==GivenClutId)
        {
         ubOpaqueDraw=ts->Opaque;
         return ts;
        }
       else if(glColorTableEXTEx && TextureMode!=2)
        {
         ts->ClutID=GivenClutId;
         if(ts!=gTexName)
          {
           gTexName=ts;
#ifndef __GX__
           glBindTexture(GL_TEXTURE_2D, gTexName->texname);
#else
		   GX_LoadTexObj(&gTexName->GXtexObj, GX_TEXMAP0);
#endif
          }
         UploadTexWndPal(TextureMode,cx,cy);
         ts->Opaque=ubOpaqueDraw;
         return gTexName;
        }
      }
    }
   else tsx=ts;
  }

 if(!tsx) 
  {
   if(iMaxTexWnds==iTexWndLimit)
    {
     tsx=wcWndtexStore+iTexWndTurn;
     iTexWndTurn++; 
     if(iTexWndTurn==iTexWndLimit) iTexWndTurn=0;
    }
   else
    {
     tsx=wcWndtexStore+iMaxTexWnds;
     iMaxTexWnds++;
    }
  }

 gTexName=tsx;

 if(TWin.OPosition.y1==TWin.Position.y1 &&
    TWin.OPosition.x1==TWin.Position.x1)
  {
   if(glColorTableEXTEx && TextureMode!=2)
    LoadPalWndTexturePage(pageid,TextureMode,cx,cy);
   else
   if(bGLExt)
    LoadPackedWndTexturePage(pageid,TextureMode,cx,cy);
   else
    LoadWndTexturePage(pageid,TextureMode,cx,cy);
  }       
 else
  {
   if(glColorTableEXTEx && TextureMode!=2)
    LoadStretchPalWndTexturePage(pageid,TextureMode,cx,cy);
   else
   if(bGLExt)
    LoadStretchPackedWndTexturePage(pageid,TextureMode,cx,cy);
   else
    LoadStretchWndTexturePage(pageid,TextureMode,cx,cy);
  }

 tsx->Opaque=ubOpaqueDraw;
 tsx->pos.l=npos.l;
 tsx->ClutID=GivenClutId;
 tsx->pageid=pageid;
 tsx->textureMode=TextureMode;
 tsx=gTexName;
 tsx->used=1;
       
 return gTexName;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// movie texture: define
////////////////////////////////////////////////////////////////////////

void DefinePackedTextureMovie(void)
{
#ifndef __GX__
 if(gTexMovieName==NULL)
  {
   gTexMovieName=malloc(sizeof(textureWndCacheEntry));
   memset(gTexMovieName, 0, sizeof(textureWndCacheEntry));
   glGenTextures(1, &gTexMovieName->texname);
   gTexName=gTexMovieName;
   glBindTexture(GL_TEXTURE_2D, gTexName->texname);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, iClampType);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, iClampType);

   if(!bUseFastMdec) 
    {
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
   else
    {
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
                                 
   glTexImage2D(GL_TEXTURE_2D, 0, //giWantedRGBA, 
                GL_RGB5_A1,
                256, 256, 0, GL_RGBA, giWantedTYPE, texturepart);
  }
 else 
  {
   gTexName=gTexMovieName;glBindTexture(GL_TEXTURE_2D, gTexName->texname);
  }

 glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                 (xrMovieArea.x1-xrMovieArea.x0), 
                 (xrMovieArea.y1-xrMovieArea.y0), 
                 GL_RGBA,
                 GL_UNSIGNED_SHORT_5_5_5_1_EXT,
                 texturepart);
#else
 if(gTexMovieName==NULL)
 {
  gTexMovieName=malloc(sizeof(textureWndCacheEntry));
  memset(gTexMovieName, 0, sizeof(textureWndCacheEntry));
  gTexName=gTexMovieName;
  createGXTexture(gTexName, 256, 256);
  u8 magFilt = !bUseFastMdec ? GX_LINEAR : GX_NEAR;
  GX_InitTexObjFilterMode(&gTexName->GXtexObj,magFilt,magFilt);
  GX_InitTexObjWrapMode(&gTexName->GXtexObj,iClampType,iClampType);
 }
 else {
  gTexName=gTexMovieName;
 }
 updateGXSubTexture(gTexName, texturepart, 0, 0, (xrMovieArea.x1-xrMovieArea.x0), (xrMovieArea.y1-xrMovieArea.y0));
 GX_LoadTexObj(&gTexName->GXtexObj, GX_TEXMAP0);
#endif
}

////////////////////////////////////////////////////////////////////////

void DefineTextureMovie(void)
{
#ifndef __GX__
 if(gTexMovieName==NULL)
  {
   gTexMovieName=malloc(sizeof(textureWndCacheEntry));
   memset(gTexMovieName, 0, sizeof(textureWndCacheEntry));
   glGenTextures(1, &gTexMovieName->texname);
   gTexName=gTexMovieName;
   glBindTexture(GL_TEXTURE_2D, gTexName->texname);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, iClampType);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, iClampType);
 
   if(!bUseFastMdec) 
    {
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
   else
    {
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

   glTexImage2D(GL_TEXTURE_2D, 0, giWantedRGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, texturepart);
  }
 else 
  {
   gTexName=gTexMovieName;glBindTexture(GL_TEXTURE_2D, gTexName->texname);
  }

 glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                 (xrMovieArea.x1-xrMovieArea.x0), 
                 (xrMovieArea.y1-xrMovieArea.y0), 
                 GL_RGBA, GL_UNSIGNED_BYTE, texturepart);
#else
 if(gTexMovieName==NULL)
 {
  gTexMovieName=malloc(sizeof(textureWndCacheEntry));
  memset(gTexMovieName, 0, sizeof(textureWndCacheEntry));
  gTexName=gTexMovieName;
  createGXTexture(gTexName, 256, 256);
  u8 magFilt = !bUseFastMdec ? GX_LINEAR : GX_NEAR;
  GX_InitTexObjFilterMode(&gTexName->GXtexObj,magFilt,magFilt);
  GX_InitTexObjWrapMode(&gTexName->GXtexObj,GX_REPEAT,GX_REPEAT);
 }
 else {
  gTexName=gTexMovieName;
 }
  updateGXSubTexture(gTexName, texturepart, 0, 0, (xrMovieArea.x1-xrMovieArea.x0), (xrMovieArea.y1-xrMovieArea.y0));
  GX_LoadTexObj(&gTexName->GXtexObj, GX_TEXMAP0);
#endif
}

////////////////////////////////////////////////////////////////////////
// movie texture: load
////////////////////////////////////////////////////////////////////////

#define MRED(x)   ((x>>3) & 0x1f)
#define MGREEN(x) ((x>>6) & 0x3e0)
#define MBLUE(x)  ((x>>9) & 0x7c00)

#define XMGREEN(x) ((x>>5)  & 0x07c0)
#define XMRED(x)   ((x<<8)  & 0xf800)
#define XMBLUE(x)  ((x>>18) & 0x003e)

////////////////////////////////////////////////////////////////////////
// movie texture: load
////////////////////////////////////////////////////////////////////////

unsigned char * LoadDirectMovieFast(void)
{
 s32 row,column;
 unsigned int startxy;

 u32 * ta=(u32 *)texturepart;

 if(PSXDisplay.RGB24)
  {
   unsigned char * pD;

   startxy=((1024)*xrMovieArea.y0)+xrMovieArea.x0;

   for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++,startxy+=1024)
    {
     pD=(unsigned char *)&psxVuw[startxy];
     for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
      {
       *ta++=(*((u32 *)pD)>>8)|0xff000000;
       pD+=3;
      }
    }
  }
 else
  {
   u32 (*LTCOL)(u32);

   LTCOL=XP8RGBA_0;//TCF[0];

   ubOpaqueDraw=0;

   for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
    {
     startxy=((1024)*column)+xrMovieArea.x0;
     for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
      *ta++=LTCOL(psxVuw[startxy++]|0x8000);
    }
  }
 
 return texturepart;
}

////////////////////////////////////////////////////////////////////////

textureWndCacheEntry* LoadTextureMovieFast(void)
{
 s32 row,column;
 unsigned int start,startxy;

 if(bGLFastMovie)
  {
   if(PSXDisplay.RGB24)
    {
     unsigned char * pD;u32 lu1,lu2;
     unsigned short * ta=(unsigned short *)texturepart;
     short sx0=xrMovieArea.x1-1;

     start=0;

     startxy=((1024)*xrMovieArea.y0)+xrMovieArea.x0;
     for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
      {
       pD=(unsigned char *)&psxVuw[startxy];
       startxy+=1024;

       for(row=xrMovieArea.x0;row<sx0;row+=2)
        {
         lu1=*((u32 *)pD);pD+=3;
         lu2=*((u32 *)pD);pD+=3;
 
         *((u32 *)ta)=
           (XMBLUE(lu1)|XMGREEN(lu1)|XMRED(lu1)|1)|
           ((XMBLUE(lu2)|XMGREEN(lu2)|XMRED(lu2)|1)<<16);
         ta+=2;
        }
       if(row==sx0) 
        {
         lu1=*((u32 *)pD);
         *ta++=XMBLUE(lu1)|XMGREEN(lu1)|XMRED(lu1)|1;
        }
      }
    }
   else
    {
     unsigned short *ta=(unsigned short *)texturepart;
     u32 lc;
     short sx0=xrMovieArea.x1-1;

     for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
      {
       startxy=((1024)*column)+xrMovieArea.x0;
       for(row=xrMovieArea.x0;row<sx0;row+=2)
        {
         lc=*((u32 *)&psxVuw[startxy]);
         *((u32 *)ta)=
          ((lc&0x001f001f)<<11)|((lc&0x03e003e0)<<1)|((lc&0x7c007c00)>>9)|0x00010001;
         ta+=2;startxy+=2;
        }
       if(row==sx0) *ta++=(psxVuw[startxy]<<1)|1;
      }
    }
   DefinePackedTextureMovie();
  }
 else
  {
   if(PSXDisplay.RGB24)
    {
     unsigned char * pD;
     u32 * ta=(u32 *)texturepart;

     startxy=((1024)*xrMovieArea.y0)+xrMovieArea.x0;

     for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++,startxy+=1024)
      {
       //startxy=((1024)*column)+xrMovieArea.x0;
       pD=(unsigned char *)&psxVuw[startxy];
       for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
        {
         *ta++=(*((u32 *)pD)>>8)|0xff000000;
         pD+=3;
        }
      }
    }
   else
    {
     u32 (*LTCOL)(u32);
     u32 *ta;

     LTCOL=XP8RGBA_0;//TCF[0];

     ubOpaqueDraw=0;
     ta=(u32 *)texturepart;

     for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
      {
       startxy=((1024)*column)+xrMovieArea.x0;
       for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
        *ta++=LTCOL(psxVuw[startxy++]|0x8000);
      }
    }
   DefineTextureMovie();
  }
 return gTexName;   
}

////////////////////////////////////////////////////////////////////////

textureWndCacheEntry* LoadTextureMovie(void)
{
 short row,column,dx;
 unsigned int startxy;
 BOOL b_X,b_Y;

 if(bUseFastMdec) return LoadTextureMovieFast();

 b_X=FALSE;b_Y=FALSE;

 if((xrMovieArea.x1-xrMovieArea.x0)<255)  b_X=TRUE;
 if((xrMovieArea.y1-xrMovieArea.y0)<255)  b_Y=TRUE;

 if(bGLFastMovie)
  {
   unsigned short c;

   if(PSXDisplay.RGB24)
    {
     unsigned char * pD;u32 lu;
     unsigned short * ta=(unsigned short *)texturepart;

     if(b_X)
      {
       for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
        {
         startxy=((1024)*column)+xrMovieArea.x0;
         pD=(unsigned char *)&psxVuw[startxy];
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          {
           lu=*((u32 *)pD);pD+=3;
           *ta++=XMBLUE(lu)|XMGREEN(lu)|XMRED(lu)|1;
         }
         *ta++=ta[-1];
        }
       if(b_Y)
        {
         dx=xrMovieArea.x1-xrMovieArea.x0+1;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          *ta++=ta[-dx];
         *ta++=ta[-1];
        }
      }
     else
      {
       for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
        {
         startxy=((1024)*column)+xrMovieArea.x0;
         pD=(unsigned char *)&psxVuw[startxy];
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          {
           lu=*((u32 *)pD);pD+=3;
           *ta++=XMBLUE(lu)|XMGREEN(lu)|XMRED(lu)|1;
          }
        }
       if(b_Y)
        {
         dx=xrMovieArea.x1-xrMovieArea.x0;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          *ta++=ta[-dx];
        }
      }
    }
   else
    {
     unsigned short *ta;

     ubOpaqueDraw=0;

     ta=(unsigned short *)texturepart;

     if(b_X)
      {
       for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
        {
         startxy=((1024)*column)+xrMovieArea.x0;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          {
           c=psxVuw[startxy++];
           *ta++=((c&0x1f)<<11)|((c&0x3e0)<<1)|((c&0x7c00)>>9)|1;
          }

         *ta++=ta[-1];
        }
       if(b_Y)
        {
         dx=xrMovieArea.x1-xrMovieArea.x0+1;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          *ta++=ta[-dx];
         *ta++=ta[-1];
        }
      }
     else
      {
       for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
        {
         startxy=((1024)*column)+xrMovieArea.x0;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          {
           c=psxVuw[startxy++];
           *ta++=((c&0x1f)<<11)|((c&0x3e0)<<1)|((c&0x7c00)>>9)|1;
          }
        }
       if(b_Y)
        {
         dx=xrMovieArea.x1-xrMovieArea.x0;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          *ta++=ta[-dx];
        }
      }
    }
   xrMovieArea.x1+=b_X;xrMovieArea.y1+=b_Y;
   DefinePackedTextureMovie();
   xrMovieArea.x1-=b_X;xrMovieArea.y1-=b_Y;
  }
 else
  {
   if(PSXDisplay.RGB24)
    {
     unsigned char * pD;
     u32 * ta=(u32 *)texturepart;

     if(b_X)
      {
       for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
        {
         startxy=((1024)*column)+xrMovieArea.x0;
         pD=(unsigned char *)&psxVuw[startxy];
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          {
		   *ta++=(*((u32 *)pD)>>8)|0xff000000;
           pD+=3;
          }
         *ta++=ta[-1];
        }
       if(b_Y)
        {
         dx=xrMovieArea.x1-xrMovieArea.x0+1;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          *ta++=ta[-dx];
         *ta++=ta[-1];
        }
      }
     else
      {
       for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
        {
         startxy=((1024)*column)+xrMovieArea.x0;
         pD=(unsigned char *)&psxVuw[startxy];
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          {
           *ta++=(*((u32 *)pD)>>8)|0xff000000;
           pD+=3;
          }
        }
       if(b_Y)
        {
         dx=xrMovieArea.x1-xrMovieArea.x0;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          *ta++=ta[-dx];
        }
      }
    }
   else
    {
     u32 (*LTCOL)(u32);
     u32 *ta;

     LTCOL=XP8RGBA_0;//TCF[0];

     ubOpaqueDraw=0;
     ta=(u32 *)texturepart;

     if(b_X)
      {
       for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
        {
         startxy=((1024)*column)+xrMovieArea.x0;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          *ta++=LTCOL(psxVuw[startxy++]|0x8000);
         *ta++=ta[-1];
        }

       if(b_Y)
        {
         dx=xrMovieArea.x1-xrMovieArea.x0+1;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          *ta++=ta[-dx];
         *ta++=ta[-1];
        }
      }
     else
      {
       for(column=xrMovieArea.y0;column<xrMovieArea.y1;column++)
        {
         startxy=((1024)*column)+xrMovieArea.x0;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          *ta++=LTCOL(psxVuw[startxy++]|0x8000);
        }

       if(b_Y)
        {
         dx=xrMovieArea.x1-xrMovieArea.x0;
         for(row=xrMovieArea.x0;row<xrMovieArea.x1;row++)
          *ta++=ta[-dx];
        }
      }
    }

   xrMovieArea.x1+=b_X;xrMovieArea.y1+=b_Y;
   DefineTextureMovie();
   xrMovieArea.x1-=b_X;xrMovieArea.y1-=b_Y;
  }
 return gTexName;   
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

textureWndCacheEntry* BlackFake15BitTexture(void)
{
 s32 pmult;short x1,x2,y1,y2;

 if(PSXDisplay.InterlacedTest) return 0;
 
 pmult=GlobalTexturePage/16;
 x1=gl_ux[7];
 x2=gl_ux[6]-gl_ux[7];
 y1=gl_ux[5];
 y2=gl_ux[4]-gl_ux[5];

 if(iSpriteTex)
  {
   if(x2<255) x2++;
   if(y2<255) y2++;
  }

 y1+=pmult*256;
 x1+=((GlobalTexturePage-16*pmult)<<6);

 if(   FastCheckAgainstFrontScreen(x1,y1,x2,y2)
    || FastCheckAgainstScreen(x1,y1,x2,y2))
  {
   if(gTexFrameName == NULL)
    {
	 gTexFrameName=malloc(sizeof(textureWndCacheEntry));
	 memset(gTexFrameName, 0, sizeof(textureWndCacheEntry));
     gTexName=gTexFrameName;
#ifndef __GX__
     glGenTextures(1, &gTexFrameName->texname);
     glBindTexture(GL_TEXTURE_2D, gTexName->texname);

     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, iClampType);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, iClampType);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 
     if(bGLExt)
      {
       unsigned short s;unsigned short * ta;

       if(giWantedTYPE==GL_UNSIGNED_SHORT_4_4_4_4_EXT)
            s=0x000f;
       else s=0x0001;

       ta=(unsigned short *)texturepart;
       for(y1=0;y1<=4;y1++)
        for(x1=0;x1<=4;x1++)
         *ta++=s;
      }
     else
#endif
      {
       u32 * ta=(u32 *)texturepart;
       for(y1=0;y1<=4;y1++)
        for(x1=0;x1<=4;x1++)
         *ta++=0xff000000;
      }
#ifndef __GX__
     glTexImage2D(GL_TEXTURE_2D, 0, giWantedRGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, texturepart);
#else
	 createGXTexture(gTexName, 4, 4);
	 updateGXSubTexture(gTexName, texturepart, 0, 0, 4, 4);
	 GX_InitTexObjFilterMode(&gTexName->GXtexObj,GX_NEAR,GX_NEAR);
	 GX_InitTexObjWrapMode(&gTexName->GXtexObj,iClampType,iClampType);
	 GX_LoadTexObj(&gTexName->GXtexObj, GX_TEXMAP0);
#endif
    }
   else
    {
     gTexName=gTexFrameName;
#ifndef __GX__
     glBindTexture(GL_TEXTURE_2D, gTexName->texname);
#else
	 GX_LoadTexObj(&gTexName->GXtexObj, GX_TEXMAP0);
#endif
    }
   ubOpaqueDraw=0;
   return gTexName;
  }
 return NULL;
}

/////////////////////////////////////////////////////////////////////////////

BOOL bFakeFrontBuffer=FALSE;
BOOL bIgnoreNextTile =FALSE;

int iFTex=512;

textureWndCacheEntry* Fake15BitTexture(void)
{
 s32 pmult;short x1,x2,y1,y2;int iYAdjust;
 float ScaleX,ScaleY;RECT rSrc;

 if(iFrameTexType==1) return BlackFake15BitTexture();
 if(PSXDisplay.InterlacedTest) return 0;
 
 pmult=GlobalTexturePage/16;
 x1=gl_ux[7];
 x2=gl_ux[6]-gl_ux[7];
 y1=gl_ux[5];
 y2=gl_ux[4]-gl_ux[5];

 y1+=pmult*256;
 x1+=((GlobalTexturePage-16*pmult)<<6);

 if(iFrameTexType==3)
  {
   if(iFrameReadType==4) return 0;

   if(!FastCheckAgainstFrontScreen(x1,y1,x2,y2) &&
      !FastCheckAgainstScreen(x1,y1,x2,y2))
    return 0;

   if(bFakeFrontBuffer) bIgnoreNextTile=TRUE;
   CheckVRamReadEx(x1,y1,x1+x2,y1+y2);
   return 0;
  }

 /////////////////////////

 if(FastCheckAgainstFrontScreen(x1,y1,x2,y2))
  {
   x1-=PSXDisplay.DisplayPosition.x;
   y1-=PSXDisplay.DisplayPosition.y;
  }
 else
 if(FastCheckAgainstScreen(x1,y1,x2,y2))
  {
   x1-=PreviousPSXDisplay.DisplayPosition.x;
   y1-=PreviousPSXDisplay.DisplayPosition.y;
  }
 else return 0;

 bDrawMultiPass = FALSE;

 if(gTexFrameName == NULL)
  {
   char * p;

   if(iResX>1280 || iResY>1024) iFTex=2048;
   else
   if(iResX>640  || iResY>480)  iFTex=1024;
   else                         iFTex=512; 
   gTexFrameName=malloc(sizeof(textureWndCacheEntry));
   memset(gTexFrameName, 0, sizeof(textureWndCacheEntry));
   gTexName=gTexFrameName;
#ifndef __GX__
   p=(char *)malloc(iFTex*iFTex*4);
   memset(p,0,iFTex*iFTex*4);
   glGenTextures(1, &gTexFrameName->texname);
   glBindTexture(GL_TEXTURE_2D, gTexName->texname);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, iClampType);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, iClampType);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexImage2D(GL_TEXTURE_2D, 0, 3, iFTex, iFTex, 0, GL_RGB, GL_UNSIGNED_BYTE, p);
   glGetError();
   free(p);
#else
   p=(char *)malloc(iFTex*iFTex*4);
   memset(p,0,iFTex*iFTex*4);
   createGXTexture(gTexName, iFTex, iFTex);
   updateGXSubTexture(gTexName, texturepart, 0, 0, iFTex, iFTex);
   GX_InitTexObjFilterMode(&gTexName->GXtexObj,GX_NEAR, GX_NEAR);
   GX_InitTexObjWrapMode(&gTexName->GXtexObj,iClampType, iClampType);
   free(p);
   GX_LoadTexObj(&gTexName->GXtexObj, GX_TEXMAP0);
#endif
  }
 else 
  {
    gTexName=gTexFrameName;
#ifndef __GX__
   glBindTexture(GL_TEXTURE_2D, gTexName->texname);
#else
   GX_LoadTexObj(&gTexName->GXtexObj, GX_TEXMAP0);
#endif
  }
 x1+=PreviousPSXDisplay.Range.x0;
 y1+=PreviousPSXDisplay.Range.y0;

 if(PSXDisplay.DisplayMode.x)
      ScaleX=(float)rRatioRect.right/(float)PSXDisplay.DisplayMode.x;
 else ScaleX=1.0f;
 if(PSXDisplay.DisplayMode.y)
      ScaleY=(float)rRatioRect.bottom/(float)PSXDisplay.DisplayMode.y;
 else ScaleY=1.0f;

 rSrc.left  =max(x1*ScaleX,0);
 rSrc.right =min((x1+x2)*ScaleX+0.99f,iResX-1);
 rSrc.top   =max(y1*ScaleY,0);
 rSrc.bottom=min((y1+y2)*ScaleY+0.99f,iResY-1);

 iYAdjust=(y1+y2)-PSXDisplay.DisplayMode.y;
 if(iYAdjust>0)
      iYAdjust=(int)((float)iYAdjust*ScaleY)+1;
 else iYAdjust=0;
          
 gl_vy[0]=255-gl_vy[0];
 gl_vy[1]=255-gl_vy[1];
 gl_vy[2]=255-gl_vy[2];
 gl_vy[3]=255-gl_vy[3];

 y1=min(gl_vy[0],min(gl_vy[1],min(gl_vy[2],gl_vy[3])));

 gl_vy[0]-=y1;
 gl_vy[1]-=y1;
 gl_vy[2]-=y1;
 gl_vy[3]-=y1;
 gl_ux[0]-=gl_ux[7];
 gl_ux[1]-=gl_ux[7];
 gl_ux[2]-=gl_ux[7];
 gl_ux[3]-=gl_ux[7];

 ScaleX*=256.0f/((float)(iFTex));
 ScaleY*=256.0f/((float)(iFTex));

 y1=((float)gl_vy[0]*ScaleY); if(y1>255) y1=255;
 gl_vy[0]=y1;
 y1=((float)gl_vy[1]*ScaleY); if(y1>255) y1=255;
 gl_vy[1]=y1;
 y1=((float)gl_vy[2]*ScaleY); if(y1>255) y1=255;
 gl_vy[2]=y1;
 y1=((float)gl_vy[3]*ScaleY); if(y1>255) y1=255;
 gl_vy[3]=y1;

 x1=((float)gl_ux[0]*ScaleX); if(x1>255) x1=255;
 gl_ux[0]=x1;
 x1=((float)gl_ux[1]*ScaleX); if(x1>255) x1=255;
 gl_ux[1]=x1;
 x1=((float)gl_ux[2]*ScaleX); if(x1>255) x1=255;
 gl_ux[2]=x1;
 x1=((float)gl_ux[3]*ScaleX); if(x1>255) x1=255;
 gl_ux[3]=x1;

 x1=rSrc.right-rSrc.left;
 if(x1<=0)             x1=1;
 if(x1>iFTex)          x1=iFTex;

 y1=rSrc.bottom-rSrc.top;
 if(y1<=0)             y1=1;
 if(y1+iYAdjust>iFTex) y1=iFTex-iYAdjust;
#ifndef __GX__
 if(bFakeFrontBuffer) glReadBuffer(GL_FRONT);

 glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 
                      0,
                      iYAdjust,
                      rSrc.left+rRatioRect.left,
                      iResY-rSrc.bottom-rRatioRect.top,
                      x1,y1);

 if(glGetError()) 
  {
   char * p=(char *)malloc(iFTex*iFTex*4);
   memset(p,0,iFTex*iFTex*4);
   glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, iFTex, iFTex,
                   GL_RGB, GL_UNSIGNED_BYTE, p);
   free(p);
  }

 if(bFakeFrontBuffer) 
  {glReadBuffer(GL_BACK);bIgnoreNextTile=TRUE;}
#else
    //Note: texture realWidth and realHeight should be multiple of 2!
    GX_SetTexCopySrc(0, iYAdjust,(u16) x1,(u16) y1);
    GX_SetTexCopyDst((u16) x1,(u16) y1, gTexName->GXtexfmt, GX_FALSE);
    GX_SetCopyFilter(GX_FALSE, NULL, GX_FALSE, NULL);
    if (gTexName->GXtexture) GX_CopyTex(gTexName->GXtexture, GX_FALSE);
    GX_PixModeSync();
    GX_SetCopyFilter(vmode->aa, vmode->sample_pattern, GX_TRUE, vmode->vfilter);
	
 if(bFakeFrontBuffer) 
  bIgnoreNextTile=TRUE;
#endif
 ubOpaqueDraw=0;

 if(iSpriteTex)
  {
   sprtW=gl_ux[1]-gl_ux[0];    
   sprtH=-(gl_vy[0]-gl_vy[2]);
  }

 return gTexName;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// load texture part (unpacked)
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void LoadSubTexturePageSort(int pageid, int mode, short cx, short cy)
{
 u32  start,row,column,j,sxh,sxm;
 unsigned int   palstart;
 u32  *px,*pa,*ta;
 unsigned char  *cSRCPtr;
 unsigned short *wSRCPtr;
 u32  LineOffset;
 u32  x2a,xalign=0;
 u32  x1=gl_ux[7];
 u32  x2=gl_ux[6];
 u32  y1=gl_ux[5];
 u32  y2=gl_ux[4];
 u32  dx=x2-x1+1;
 u32  dy=y2-y1+1;
 int pmult=pageid/16;
 u32 (*LTCOL)(u32);
 unsigned int a,r,g,b,cnt,h;
 u32 scol[8];
 
 LTCOL=TCF[DrawSemiTrans];

 pa=px=(u32 *)ubPaletteBuffer;
 ta=(u32 *)texturepart;
 palstart=cx+(cy<<10);

 ubOpaqueDraw=0;

 if(YTexS) {ta+=dx;if(XTexS) ta+=2;}
 if(XTexS) {ta+=1;xalign=2;}

 switch(mode)
  {
   //--------------------------------------------------// 
   // 4bit texture load ..
   case 0:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;

      row=4;do
       {
        *px    =LTCOL(*wSRCPtr);
        *(px+1)=LTCOL(*(wSRCPtr+1));
        *(px+2)=LTCOL(*(wSRCPtr+2));
        *(px+3)=LTCOL(*(wSRCPtr+3));
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      for(TXV=y1;TXV<=y2;TXV++)
       {
        for(TXU=x1;TXU<=x2;TXU++)
         {
  		  n_xi = ( ( TXU >> 2 ) & ~0x3c ) + ( ( TXV << 2 ) & 0x3c );
		  n_yi = ( TXV & ~0xf ) + ( ( TXU >> 4 ) & 0xf );

          *ta++=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x03 ) << 2 ) ) & 0x0f ));
         }
        ta+=xalign;
       }
      break;
     }

    start=((pageid-16*pmult)<<7)+524288*pmult;
    // convert CLUT to 32bits .. and then use THAT as a lookup table

    wSRCPtr=psxVuw+palstart;

    row=4;do
     {
      *px    =LTCOL((wSRCPtr[0]));
      *(px+1)=LTCOL((wSRCPtr[1]));
      *(px+2)=LTCOL(((wSRCPtr[2])));
      *(px+3)=LTCOL(((wSRCPtr[3])));
      row--;px+=4;wSRCPtr+=4;
	  
     }
    while (row);

	
    x2a=x2?(x2-1):0;
    sxm=x1&1;sxh=x1>>1;
    j=sxm?(x1+1):x1;
    for(column=y1;column<=y2;column++)
     {
      cSRCPtr = psxVub + start + (column<<11) + sxh;
    
      if(sxm) *ta++=pa[(*cSRCPtr++ >> 4) & 0xF];

      for(row=j;row<x2a;row+=2)
       {
        *ta    =pa[*cSRCPtr & 0xF]; 
        *(ta+1)=pa[(*cSRCPtr >> 4) & 0xF];
        cSRCPtr++;ta+=2;
       }
      if(row<=x2) 
       {
        *ta++=pa[*cSRCPtr & 0xF]; row++;
        if(row<=x2) *ta++=pa[(*cSRCPtr >> 4) & 0xF];
       }
      ta+=xalign;
     }


    break;
   //--------------------------------------------------// 
   // 8bit texture load ..
   case 1:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;

      row=64;do
       {
        *px    =LTCOL(wSRCPtr[0]);
        *(px+1)=LTCOL(wSRCPtr[1]);
        *(px+2)=LTCOL(wSRCPtr[2]);
        *(px+3)=LTCOL(wSRCPtr[3]);
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      for(TXV=y1;TXV<=y2;TXV++)
       {
        for(TXU=x1;TXU<=x2;TXU++)
         {
  		  n_xi = ( ( TXU >> 1 ) & ~0x78 ) + ( ( TXU << 2 ) & 0x40 ) + ( ( TXV << 3 ) & 0x38 );
		  n_yi = ( TXV & ~0x7 ) + ( ( TXU >> 5 ) & 0x7 );

          *ta++=pa[(*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x01 ) << 3 ) ) & 0xff];
         }
        ta+=xalign;
       }

      break;
     }

    start=((pageid-16*pmult)<<7)+524288*pmult;

    cSRCPtr = psxVub + start + (y1<<11) + x1;
    LineOffset = 2048 - dx; 

    if(dy*dx>384)
     {
      wSRCPtr=psxVuw+palstart;

      row=64;do
       {
        *px    =LTCOL(wSRCPtr[0]);
        *(px+1)=LTCOL(wSRCPtr[1]);
        *(px+2)=LTCOL(wSRCPtr[2]);
        *(px+3)=LTCOL(wSRCPtr[3]);
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      column=dy;do 
       {
        row=dx;
        do {*ta++=*(pa+(*cSRCPtr++));row--;} while(row);
        ta+=xalign;
        cSRCPtr+=LineOffset;column--;
       }
      while(column);
     }
    else
     {
      wSRCPtr=psxVuw+palstart;

      column=dy;do 
       {
        row=dx;
        do {*ta++=LTCOL(wSRCPtr[*cSRCPtr++]);row--;} while(row);
        ta+=xalign;
        cSRCPtr+=LineOffset;column--;
       }
      while(column);
     }

    break;
   //--------------------------------------------------// 
   // 16bit texture load ..
   case 2:
    start=((pageid-16*pmult)<<6)+262144*pmult;

    wSRCPtr = psxVuw + start + (y1<<10) + x1;
    LineOffset = 1024 - dx; 

    column=dy;do 
     {
      row=dx;
      do {*ta++=LTCOL(*wSRCPtr++);row--;} while(row);
      ta+=xalign;
      wSRCPtr+=LineOffset;column--;
     }
    while(column);

    break;
   //--------------------------------------------------//
   // others are not possible !
  }

 x2a=dx+xalign;

 if(YTexS)
  {
   ta=(u32 *)texturepart;
   pa=(u32 *)texturepart+x2a;
   row=x2a;do {*ta++=*pa++;row--;} while(row);        
   pa=(u32 *)texturepart+dy*x2a;
   ta=pa+x2a;
   row=x2a;do {*ta++=*pa++;row--;} while(row);
   YTexS--;
   dy+=2;
  }

 if(XTexS)
  {
   ta=(u32 *)texturepart;
   pa=ta+1;
   row=dy;do {*ta=*pa;ta+=x2a;pa+=x2a;row--;} while(row);
   pa=(u32 *)texturepart+dx;
   ta=pa+1;
   row=dy;do {*ta=*pa;ta+=x2a;pa+=x2a;row--;} while(row);
   XTexS--;
   dx+=2;
  }

 DXTexS=dx;DYTexS=dy;

 if(!iFilterType) {DefineSubTextureSort();return;}
 if(iFilterType!=2 && iFilterType!=4 && iFilterType!=6) {DefineSubTextureSort();return;}
 if((iFilterType==4 || iFilterType==6) && ly0==ly1 && ly2==ly3 && lx0==lx3 && lx1==lx2)
  {DefineSubTextureSort();return;}

 ta=(u32 *)texturepart;
 x1=dx-1;
 y1=dy-1;

 if(bOpaquePass)
  {
   if(bSmallAlpha)
    {
     for(column=0;column<dy;column++)
      {
       for(row=0;row<dx;row++)
        {
         if(*ta==0x03000000)
          {
           cnt=0;

           if(           column     && *(ta-dx)  >>24 !=0x03) scol[cnt++]=*(ta-dx);
           if(row                   && *(ta-1)   >>24 !=0x03) scol[cnt++]=*(ta-1);
           if(row!=x1               && *(ta+1)   >>24 !=0x03) scol[cnt++]=*(ta+1);
           if(           column!=y1 && *(ta+dx)  >>24 !=0x03) scol[cnt++]=*(ta+dx);

           if(row     && column     && *(ta-dx-1)>>24 !=0x03) scol[cnt++]=*(ta-dx-1);
           if(row!=x1 && column     && *(ta-dx+1)>>24 !=0x03) scol[cnt++]=*(ta-dx+1);
           if(row     && column!=y1 && *(ta+dx-1)>>24 !=0x03) scol[cnt++]=*(ta+dx-1);
           if(row!=x1 && column!=y1 && *(ta+dx+1)>>24 !=0x03) scol[cnt++]=*(ta+dx+1);

           if(cnt)
            {
             r=g=b=a=0;
             for(h=0;h<cnt;h++)
              {
               r+=(scol[h]>>16)&0xff;
               g+=(scol[h]>>8)&0xff;
               b+=scol[h]&0xff;
              }
             r/=cnt;b/=cnt;g/=cnt;

             *ta=(r<<16)|(g<<8)|b;
             *ta|=0x03000000;
            }
          }
         ta++;
        }
      }
    }
   else
    {
     for(column=0;column<dy;column++)
      {
       for(row=0;row<dx;row++)
        {
         if(*ta==0x50000000)
          {
           cnt=0;

           if(           column     && *(ta-dx)  !=0x50000000 && *(ta-dx)>>24!=1) scol[cnt++]=*(ta-dx);
           if(row                   && *(ta-1)   !=0x50000000 && *(ta-1)>>24!=1) scol[cnt++]=*(ta-1);
           if(row!=x1               && *(ta+1)   !=0x50000000 && *(ta+1)>>24!=1) scol[cnt++]=*(ta+1);
           if(           column!=y1 && *(ta+dx)  !=0x50000000 && *(ta+dx)>>24!=1) scol[cnt++]=*(ta+dx);

           if(row     && column     && *(ta-dx-1)!=0x50000000 && *(ta-dx-1)>>24!=1) scol[cnt++]=*(ta-dx-1);
           if(row!=x1 && column     && *(ta-dx+1)!=0x50000000 && *(ta-dx+1)>>24!=1) scol[cnt++]=*(ta-dx+1);
           if(row     && column!=y1 && *(ta+dx-1)!=0x50000000 && *(ta+dx-1)>>24!=1) scol[cnt++]=*(ta+dx-1);
           if(row!=x1 && column!=y1 && *(ta+dx+1)!=0x50000000 && *(ta+dx+1)>>24!=1) scol[cnt++]=*(ta+dx+1);

           if(cnt)
            {
             r=g=b=a=0;
             for(h=0;h<cnt;h++)
              {
               a+=(scol[h]>>24);
               r+=(scol[h]>>16)&0xff;
               g+=(scol[h]>>8)&0xff;
               b+=scol[h]&0xff;
              }
             r/=cnt;b/=cnt;g/=cnt;

             *ta=(r<<16)|(g<<8)|b;
             if(a) *ta|=0x50000000;
             else  *ta|=0x01000000;
            }
          }
         ta++;
        }
      }
    }
  }
 else {
  for(column=0;column<dy;column++)
  {
   for(row=0;row<dx;row++)
    {
     if(*ta==0x00000000)
      {
       cnt=0;

       if(row!=x1               && *(ta+1)   !=0x00000000) scol[cnt++]=*(ta+1);
       if(           column!=y1 && *(ta+dx)  !=0x00000000) scol[cnt++]=*(ta+dx);

       if(cnt)
        {
         r=g=b=0;
         for(h=0;h<cnt;h++)
          {
           r+=(scol[h]>>16)&0xff;
           g+=(scol[h]>>8)&0xff;
           b+=scol[h]&0xff;
          }
         r/=cnt;b/=cnt;g/=cnt;
         *ta=(r<<16)|(g<<8)|b;
        }
      }
     ta++;
    }
  }
 }

 DefineSubTextureSort();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// load texture part (packed)
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void LoadPackedSubTexturePageSort(int pageid, int mode, short cx, short cy)
{
 u32  start,row,column,j,sxh,sxm;
 unsigned int   palstart;
 unsigned short *px,*pa,*ta;
 unsigned char  *cSRCPtr;
 unsigned short *wSRCPtr;
 u32  LineOffset;
 u32  x2a,xalign=0;
 u32  x1=gl_ux[7];
 u32  x2=gl_ux[6];
 u32  y1=gl_ux[5];
 u32  y2=gl_ux[4];
 u32  dx=x2-x1+1;
 u32  dy=y2-y1+1;
 int pmult=pageid/16;
 unsigned short (*LPTCOL)(unsigned short);
 unsigned int a,r,g,b,cnt,h;
 unsigned short scol[8];

 LPTCOL=PTCF[DrawSemiTrans];

 pa=px=(unsigned short *)ubPaletteBuffer;
 ta=(unsigned short *)texturepart;
 palstart=cx+(cy<<10);

 ubOpaqueDraw=0;

 if(YTexS) {ta+=dx;if(XTexS) ta+=2;}
 if(XTexS) {ta+=1;xalign=2;}

 switch(mode)
  {
   //--------------------------------------------------// 
   // 4bit texture load ..
   case 0:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;
      row=4;do
       {
        *px    =LPTCOL(*wSRCPtr);
        *(px+1)=LPTCOL(*(wSRCPtr+1));
        *(px+2)=LPTCOL(*(wSRCPtr+2));
        *(px+3)=LPTCOL(*(wSRCPtr+3));
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      for(TXV=y1;TXV<=y2;TXV++)
       {
        for(TXU=x1;TXU<=x2;TXU++)
         {
  		  n_xi = ( ( TXU >> 2 ) & ~0x3c ) + ( ( TXV << 2 ) & 0x3c );
		  n_yi = ( TXV & ~0xf ) + ( ( TXU >> 4 ) & 0xf );

          *ta++=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x03 ) << 2 ) ) & 0x0f ));
         }
        ta+=xalign;
       }
      break;
     }

    start=((pageid-16*pmult)<<7)+524288*pmult;

    wSRCPtr=psxVuw+palstart;
    row=4;do
     {
      *px    =LPTCOL(*wSRCPtr);
      *(px+1)=LPTCOL(*(wSRCPtr+1));
      *(px+2)=LPTCOL(*(wSRCPtr+2));
      *(px+3)=LPTCOL(*(wSRCPtr+3));
      row--;px+=4;wSRCPtr+=4;
     }
    while (row);

    x2a=x2?(x2-1):0;//if(x2) x2a=x2-1; else x2a=0;
    sxm=x1&1;sxh=x1>>1;
    j=sxm?(x1+1):x1;//if(sxm) j=x1+1; else j=x1;

    for(column=y1;column<=y2;column++)
     {
      cSRCPtr = psxVub + start + (column<<11) + sxh;
    
      if(sxm) *ta++=*(pa+((*cSRCPtr++ >> 4) & 0xF));
 
      for(row=j;row<x2a;row+=2)
       {
        *ta    =*(pa+(*cSRCPtr & 0xF));
        *(ta+1)=*(pa+((*cSRCPtr >> 4) & 0xF)); 
        cSRCPtr++;ta+=2;
       }

      if(row<=x2)
       {
        *ta++=*(pa+(*cSRCPtr & 0xF));row++;
        if(row<=x2) *ta++=*(pa+((*cSRCPtr >> 4) & 0xF));
       }

      ta+=xalign;
     }
    break;
   //--------------------------------------------------// 
   // 8bit texture load ..
   case 1:
    if(GlobalTextIL)
     {
      unsigned int TXV,TXU,n_xi,n_yi;

      wSRCPtr=psxVuw+palstart;

      row=64;do
       {
        *px    =LPTCOL(*wSRCPtr);
        *(px+1)=LPTCOL(*(wSRCPtr+1));
        *(px+2)=LPTCOL(*(wSRCPtr+2));
        *(px+3)=LPTCOL(*(wSRCPtr+3));
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      for(TXV=y1;TXV<=y2;TXV++)
       {
        for(TXU=x1;TXU<=x2;TXU++)
         {
  		  n_xi = ( ( TXU >> 1 ) & ~0x78 ) + ( ( TXU << 2 ) & 0x40 ) + ( ( TXV << 3 ) & 0x38 );
		  n_yi = ( TXV & ~0x7 ) + ( ( TXU >> 5 ) & 0x7 );

          *ta++=*(pa+((*( psxVuw + ((GlobalTextAddrY + n_yi)*1024) + GlobalTextAddrX + n_xi ) >> ( ( TXU & 0x01 ) << 3 ) ) & 0xff));
         }
        ta+=xalign;
       }

      break;
     }

    start=((pageid-16*pmult)<<7)+524288*pmult;

    cSRCPtr = psxVub + start + (y1<<11) + x1;
    LineOffset = 2048 - dx;

    if(dy*dx>384)                                      // more pix? use lut
     {
      wSRCPtr=psxVuw+palstart;

      row=64;do
       {
        *px    =LPTCOL(*wSRCPtr);
        *(px+1)=LPTCOL(*(wSRCPtr+1));
        *(px+2)=LPTCOL(*(wSRCPtr+2));
        *(px+3)=LPTCOL(*(wSRCPtr+3));
        row--;px+=4;wSRCPtr+=4;
       }
      while (row);

      column=dy;do 
       {
        row=dx;
        do {*ta++=*(pa+(*cSRCPtr++));row--;} while(row);

        ta+=xalign;

        cSRCPtr+=LineOffset;column--;
       }
      while(column);
     }
    else                                               // small area? no lut
     {                                            
      wSRCPtr=psxVuw+palstart;

      column=dy;do 
       {
        row=dx;
        do {*ta++=LPTCOL(*(wSRCPtr+*cSRCPtr++));row--;} while(row);

        ta+=xalign;

        cSRCPtr+=LineOffset;column--;
       }
      while(column);
     }
    break;
   //--------------------------------------------------// 
   // 16bit texture load ..
   case 2:
    start=((pageid-16*pmult)<<6)+262144*pmult;

    wSRCPtr = psxVuw + start + (y1<<10) + x1;
    LineOffset = 1024 - dx; 

    column=dy;do 
     {
      row=dx;
      do {*ta++=LPTCOL(*wSRCPtr++);row--;} while(row);

      ta+=xalign;

      wSRCPtr+=LineOffset;column--;
     }
    while(column);
    break;
   //--------------------------------------------------// 
   // others are not possible !
  }

 ////////////////////////////////////////////////////////

 x2a=dx+xalign;

 if(YTexS)
  {
   ta=(unsigned short *)texturepart;
   pa=(unsigned short *)texturepart+x2a;
   row=x2a;do {*ta++=*pa++;row--;} while(row);

   pa=(unsigned short *)texturepart+dy*x2a;
   ta=pa+x2a;
   row=x2a;do {*ta++=*pa++;row--;} while(row);

   YTexS--;
   dy+=2;
  }

 if(XTexS)
  {
   ta=(unsigned short *)texturepart;
   pa=ta+1;
   row=dy;do {*ta=*pa;ta+=x2a;pa+=x2a;row--;} while(row);

   pa=(unsigned short *)texturepart+dx;
   ta=pa+1;
   row=dy;do {*ta=*pa;ta+=x2a;pa+=x2a;row--;} while(row);

   XTexS--;
   dx+=2;
  }

 DXTexS=dx;DYTexS=dy;

 if(!iFilterType) {DefineSubTextureSort();return;}
 if(iFilterType!=2 && iFilterType!=4 && iFilterType!=6) {DefineSubTextureSort();return;}
 if((iFilterType==4 || iFilterType==6) && ly0==ly1 && ly2==ly3 && lx0==lx3 && lx1==lx2)
  {DefineSubTextureSort();return;}

 ta=(unsigned short *)texturepart;
 x1=dx-1;
 y1=dy-1;      

 if(iTexQuality==1)

  {
   if(bOpaquePass)
   for(column=0;column<dy;column++)
    {
     for(row=0;row<dx;row++)
      {
       if(*ta==0x0006)
        {
         cnt=0;

         if(           column     && *(ta-dx)  != 0x0006 && *(ta-dx)!=0) scol[cnt++]=*(ta-dx);
         if(row                   && *(ta-1)   != 0x0006 && *(ta-1) !=0) scol[cnt++]=*(ta-1);
         if(row!=x1               && *(ta+1)   != 0x0006 && *(ta+1) !=0) scol[cnt++]=*(ta+1);
         if(           column!=y1 && *(ta+dx)  != 0x0006 && *(ta+dx)!=0) scol[cnt++]=*(ta+dx);
 
         if(row     && column     && *(ta-dx-1)!= 0x0006 && *(ta-dx-1)!=0) scol[cnt++]=*(ta-dx-1);
         if(row!=x1 && column     && *(ta-dx+1)!= 0x0006 && *(ta-dx+1)!=0) scol[cnt++]=*(ta-dx+1);
         if(row     && column!=y1 && *(ta+dx-1)!= 0x0006 && *(ta+dx-1)!=0) scol[cnt++]=*(ta+dx-1);
         if(row!=x1 && column!=y1 && *(ta+dx+1)!= 0x0006 && *(ta+dx+1)!=0) scol[cnt++]=*(ta+dx+1);

         if(cnt)
          {
           r=g=b=a=0;
           for(h=0;h<cnt;h++)
            {
             a+=scol[h]&0xf;
             r+=scol[h]>>12;
             g+=(scol[h]>>8)&0xf;
             b+=(scol[h]>>4)&0xf;
            }
           r/=cnt;b/=cnt;g/=cnt;
           *ta=(r<<12)|(g<<8)|(b<<4);
           if(a) *ta|=6;
           else  *ta=0;
          }
        }
       ta++;
      }
    }
   else
   for(column=0;column<dy;column++)
    {
     for(row=0;row<dx;row++)
      {
       if(*ta==0x0000)
        {
         cnt=0;

         if(           column     && *(ta-dx)  != 0x0000) scol[cnt++]=*(ta-dx);
         if(row                   && *(ta-1)   != 0x0000) scol[cnt++]=*(ta-1);
         if(row!=x1               && *(ta+1)   != 0x0000) scol[cnt++]=*(ta+1);
         if(           column!=y1 && *(ta+dx)  != 0x0000) scol[cnt++]=*(ta+dx);
 
         if(row     && column     && *(ta-dx-1)!= 0x0000) scol[cnt++]=*(ta-dx-1);
         if(row!=x1 && column     && *(ta-dx+1)!= 0x0000) scol[cnt++]=*(ta-dx+1);
         if(row     && column!=y1 && *(ta+dx-1)!= 0x0000) scol[cnt++]=*(ta+dx-1);
         if(row!=x1 && column!=y1 && *(ta+dx+1)!= 0x0000) scol[cnt++]=*(ta+dx+1);

         if(cnt)
          {
           r=g=b=0;
           for(h=0;h<cnt;h++)
            {
             r+=scol[h]>>12;
             g+=(scol[h]>>8)&0xf;
             b+=(scol[h]>>4)&0xf;
            }
           r/=cnt;b/=cnt;g/=cnt;
           *ta=(r<<12)|(g<<8)|(b<<4);
          }
        }
       ta++;
      }
    }
  }
 else
  {
   for(column=0;column<dy;column++)
    {
     for(row=0;row<dx;row++)
      {
       if(*ta==0)
        {
         cnt=0;

         if(           column     && *(ta-dx)  &1) scol[cnt++]=*(ta-dx);
         if(row                   && *(ta-1)   &1) scol[cnt++]=*(ta-1);
         if(row!=x1               && *(ta+1)   &1) scol[cnt++]=*(ta+1);
         if(           column!=y1 && *(ta+dx)  &1) scol[cnt++]=*(ta+dx);

         if(row     && column     && *(ta-dx-1)&1) scol[cnt++]=*(ta-dx-1);
         if(row!=x1 && column     && *(ta-dx+1)&1) scol[cnt++]=*(ta-dx+1);
         if(row     && column!=y1 && *(ta+dx-1)&1) scol[cnt++]=*(ta+dx-1);
         if(row!=x1 && column!=y1 && *(ta+dx+1)&1) scol[cnt++]=*(ta+dx+1);

         if(cnt)
          {
           r=g=b=0;
           for(h=0;h<cnt;h++)
            {
             r+=scol[h]>>11;
             g+=(scol[h]>>6)&0x1f;
             b+=(scol[h]>>1)&0x1f;
            }
           r/=cnt;b/=cnt;g/=cnt;
           *ta=(r<<11)|(g<<6)|(b<<1);
          }
        }
       ta++;
      }
    }
  }

 DefineSubTextureSort();
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// hires texture funcs
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


#define GET_RESULT(A, B, C, D) ((A != C || A != D) - (B != C || B != D))

////////////////////////////////////////////////////////////////////////

#define colorMask8     0x00FEFEFE
#define lowPixelMask8  0x00010101
#define qcolorMask8    0x00FCFCFC
#define qlowpixelMask8 0x00030303


#define INTERPOLATE8_02(A, B) (((((A & colorMask8) >> 1) + ((B & colorMask8) >> 1) + (A & B & lowPixelMask8))|((((A&0xFF000000)==0x03000000)?0x03000000:(((B&0xFF000000)==0x03000000)?0x03000000:(((A&0xFF000000)==0x00000000)?0x00000000:(((B&0xFF000000)==0x00000000)?0x00000000:0xFF000000)))))))

#define Q_INTERPOLATE8_02(A, B, C, D) (((((A & qcolorMask8) >> 2) + ((B & qcolorMask8) >> 2) + ((C & qcolorMask8) >> 2) + ((D & qcolorMask8) >> 2) + ((((A & qlowpixelMask8) + (B & qlowpixelMask8) + (C & qlowpixelMask8) + (D & qlowpixelMask8)) >> 2) & qlowpixelMask8))|((((A&0xFF000000)==0x03000000)?0x03000000:(((B&0xFF000000)==0x03000000)?0x03000000:(((C&0xFF000000)==0x03000000)?0x03000000:(((D&0xFF000000)==0x03000000)?0x03000000:(((A&0xFF000000)==0x00000000)?0x00000000:(((B&0xFF000000)==0x00000000)?0x00000000:(((C&0xFF000000)==0x00000000)?0x00000000:(((D&0xFF000000)==0x00000000)?0x00000000:0xFF000000)))))))))))

#define INTERPOLATE8(A, B) (((((A & colorMask8) >> 1) + ((B & colorMask8) >> 1) + (A & B & lowPixelMask8))|((((A&0xFF000000)==0x50000000)?0x50000000:(((B&0xFF000000)==0x50000000)?0x50000000:(((A&0xFF000000)==0x00000000)?0x00000000:(((B&0xFF000000)==0x00000000)?0x00000000:0xFF000000)))))))

#define Q_INTERPOLATE8(A, B, C, D) (((((A & qcolorMask8) >> 2) + ((B & qcolorMask8) >> 2) + ((C & qcolorMask8) >> 2) + ((D & qcolorMask8) >> 2) + ((((A & qlowpixelMask8) + (B & qlowpixelMask8) + (C & qlowpixelMask8) + (D & qlowpixelMask8)) >> 2) & qlowpixelMask8))|((((A&0xFF000000)==0x50000000)?0x50000000:(((B&0xFF000000)==0x50000000)?0x50000000:(((C&0xFF000000)==0x50000000)?0x50000000:(((D&0xFF000000)==0x50000000)?0x50000000:(((A&0xFF000000)==0x00000000)?0x00000000:(((B&0xFF000000)==0x00000000)?0x00000000:(((C&0xFF000000)==0x00000000)?0x00000000:(((D&0xFF000000)==0x00000000)?0x00000000:0xFF000000)))))))))))

void Super2xSaI_ex8_Ex(unsigned char *srcPtr, DWORD srcPitch,
	            unsigned char  *dstBitmap, int width, int height)
{
 DWORD dstPitch = srcPitch * 2;
 DWORD line;
 DWORD *dP;
 DWORD *bP;
 int   width2 = width*2;
 int iXA,iXB,iXC,iYA,iYB,iYC,finish;
 DWORD color4, color5, color6;
 DWORD color1, color2, color3;
 DWORD colorA0, colorA1, colorA2, colorA3,
       colorB0, colorB1, colorB2, colorB3,
       colorS1, colorS2;
 DWORD product1a, product1b,
       product2a, product2b;

 line = 0;

  {
   for (; height; height-=1)
	{
     bP = (DWORD *)srcPtr;
	 dP = (DWORD *)(dstBitmap + line*dstPitch);
     for (finish = width; finish; finish -= 1 )
      {
//---------------------------------------    B1 B2
//                                         4  5  6 S2
//                                         1  2  3 S1
//                                           A1 A2
       if(finish==width) iXA=0;
       else              iXA=1;
       if(finish>4) {iXB=1;iXC=2;}
       else
       if(finish>3) {iXB=1;iXC=1;}
       else         {iXB=0;iXC=0;}
       if(line==0) iYA=0;
       else        iYA=width;
       if(height>4) {iYB=width;iYC=width2;}
       else
       if(height>3) {iYB=width;iYC=width;}
       else         {iYB=0;iYC=0;}


       colorB0 = *(bP- iYA - iXA);
       colorB1 = *(bP- iYA);
       colorB2 = *(bP- iYA + iXB);
       colorB3 = *(bP- iYA + iXC);

       color4 = *(bP  - iXA);
       color5 = *(bP);
       color6 = *(bP  + iXB);
       colorS2 = *(bP + iXC);

       color1 = *(bP  + iYB  - iXA);
       color2 = *(bP  + iYB);
       color3 = *(bP  + iYB  + iXB);
       colorS1= *(bP  + iYB  + iXC);

       colorA0 = *(bP + iYC - iXA);
       colorA1 = *(bP + iYC);
       colorA2 = *(bP + iYC + iXB);
       colorA3 = *(bP + iYC + iXC);

//--------------------------------------
       if (color2 == color6 && color5 != color3)
        {
         product2b = product1b = color2;
        }
       else
       if (color5 == color3 && color2 != color6)
        {
         product2b = product1b = color5;
        }
       else
       if (color5 == color3 && color2 == color6)
        {
         register int r = 0;

         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (color1&0x00ffffff),  (colorA1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (color4&0x00ffffff),  (colorB1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (colorA2&0x00ffffff), (colorS1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (colorB2&0x00ffffff), (colorS2&0x00ffffff));

         if (r > 0)
          product2b = product1b = color6;
         else
         if (r < 0)
          product2b = product1b = color5;
         else
          {
           product2b = product1b = INTERPOLATE8_02(color5, color6);
          }
        }
       else
        {
         if (color6 == color3 && color3 == colorA1 && color2 != colorA2 && color3 != colorA0)
             product2b = Q_INTERPOLATE8_02 (color3, color3, color3, color2);
         else
         if (color5 == color2 && color2 == colorA2 && colorA1 != color3 && color2 != colorA3)
             product2b = Q_INTERPOLATE8_02 (color2, color2, color2, color3);
         else
             product2b = INTERPOLATE8_02 (color2, color3);

         if (color6 == color3 && color6 == colorB1 && color5 != colorB2 && color6 != colorB0)
             product1b = Q_INTERPOLATE8_02 (color6, color6, color6, color5);
         else
         if (color5 == color2 && color5 == colorB2 && colorB1 != color6 && color5 != colorB3)
             product1b = Q_INTERPOLATE8_02 (color6, color5, color5, color5);
         else
             product1b = INTERPOLATE8_02 (color5, color6);
        }

       if (color5 == color3 && color2 != color6 && color4 == color5 && color5 != colorA2)
        product2a = INTERPOLATE8_02(color2, color5);
       else
       if (color5 == color1 && color6 == color5 && color4 != color2 && color5 != colorA0)
        product2a = INTERPOLATE8_02(color2, color5);
       else
        product2a = color2;

       if (color2 == color6 && color5 != color3 && color1 == color2 && color2 != colorB2)
        product1a = INTERPOLATE8_02(color2, color5);
       else
       if (color4 == color2 && color3 == color2 && color1 != color5 && color2 != colorB0)
        product1a = INTERPOLATE8_02(color2, color5);
       else
        product1a = color5;

       *dP=product1a;
       *(dP+1)=product1b;
       *(dP+(width2))=product2a;
       *(dP+1+(width2))=product2b;

       bP += 1;
       dP += 2;
      }//end of for ( finish= width etc..)

     line += 2;
     srcPtr += srcPitch;
	}; //endof: for (; height; height--)
  }
}


void Super2xSaI_ex8(unsigned char *srcPtr, DWORD srcPitch,
	            unsigned char  *dstBitmap, int width, int height)
{
 DWORD dstPitch = srcPitch * 2;
 DWORD line;
 DWORD *dP;
 DWORD *bP;
 int   width2 = width*2;
 int iXA,iXB,iXC,iYA,iYB,iYC,finish;
 DWORD color4, color5, color6;
 DWORD color1, color2, color3;
 DWORD colorA0, colorA1, colorA2, colorA3,
       colorB0, colorB1, colorB2, colorB3,
       colorS1, colorS2;
 DWORD product1a, product1b,
       product2a, product2b;

 line = 0;

  {
   for (; height; height-=1)
	{
     bP = (DWORD *)srcPtr;
	 dP = (DWORD *)(dstBitmap + line*dstPitch);
     for (finish = width; finish; finish -= 1 )
      {
//---------------------------------------    B1 B2
//                                         4  5  6 S2
//                                         1  2  3 S1
//                                           A1 A2
       if(finish==width) iXA=0;
       else              iXA=1;
       if(finish>4) {iXB=1;iXC=2;}
       else
       if(finish>3) {iXB=1;iXC=1;}
       else         {iXB=0;iXC=0;}
       if(line==0) iYA=0;
       else        iYA=width;
       if(height>4) {iYB=width;iYC=width2;}
       else
       if(height>3) {iYB=width;iYC=width;}
       else         {iYB=0;iYC=0;}


       colorB0 = *(bP- iYA - iXA);
       colorB1 = *(bP- iYA);
       colorB2 = *(bP- iYA + iXB);
       colorB3 = *(bP- iYA + iXC);

       color4 = *(bP  - iXA);
       color5 = *(bP);
       color6 = *(bP  + iXB);
       colorS2 = *(bP + iXC);

       color1 = *(bP  + iYB  - iXA);
       color2 = *(bP  + iYB);
       color3 = *(bP  + iYB  + iXB);
       colorS1= *(bP  + iYB  + iXC);

       colorA0 = *(bP + iYC - iXA);
       colorA1 = *(bP + iYC);
       colorA2 = *(bP + iYC + iXB);
       colorA3 = *(bP + iYC + iXC);

//--------------------------------------
       if (color2 == color6 && color5 != color3)
        {
         product2b = product1b = color2;
        }
       else
       if (color5 == color3 && color2 != color6)
        {
         product2b = product1b = color5;
        }
       else
       if (color5 == color3 && color2 == color6)
        {
         register int r = 0;

         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (color1&0x00ffffff),  (colorA1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (color4&0x00ffffff),  (colorB1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (colorA2&0x00ffffff), (colorS1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (colorB2&0x00ffffff), (colorS2&0x00ffffff));

         if (r > 0)
          product2b = product1b = color6;
         else
         if (r < 0)
          product2b = product1b = color5;
         else
          {
           product2b = product1b = INTERPOLATE8(color5, color6);
          }
        }
       else
        {
         if (color6 == color3 && color3 == colorA1 && color2 != colorA2 && color3 != colorA0)
             product2b = Q_INTERPOLATE8 (color3, color3, color3, color2);
         else
         if (color5 == color2 && color2 == colorA2 && colorA1 != color3 && color2 != colorA3)
             product2b = Q_INTERPOLATE8 (color2, color2, color2, color3);
         else
             product2b = INTERPOLATE8 (color2, color3);

         if (color6 == color3 && color6 == colorB1 && color5 != colorB2 && color6 != colorB0)
             product1b = Q_INTERPOLATE8 (color6, color6, color6, color5);
         else
         if (color5 == color2 && color5 == colorB2 && colorB1 != color6 && color5 != colorB3)
             product1b = Q_INTERPOLATE8 (color6, color5, color5, color5);
         else
             product1b = INTERPOLATE8 (color5, color6);
        }

       if (color5 == color3 && color2 != color6 && color4 == color5 && color5 != colorA2)
        product2a = INTERPOLATE8(color2, color5);
       else
       if (color5 == color1 && color6 == color5 && color4 != color2 && color5 != colorA0)
        product2a = INTERPOLATE8(color2, color5);
       else
        product2a = color2;

       if (color2 == color6 && color5 != color3 && color1 == color2 && color2 != colorB2)
        product1a = INTERPOLATE8(color2, color5);
       else
       if (color4 == color2 && color3 == color2 && color1 != color5 && color2 != colorB0)
        product1a = INTERPOLATE8(color2, color5);
       else
        product1a = color5;

       *dP=product1a;
       *(dP+1)=product1b;
       *(dP+(width2))=product2a;
       *(dP+1+(width2))=product2b;

       bP += 1;
       dP += 2;
      }//end of for ( finish= width etc..)

     line += 2;
     srcPtr += srcPitch;
	}; //endof: for (; height; height--)
  }
}
/////////////////////////////////////////////////////////////////////////////

#define colorMask4     0x0000EEE0
#define lowPixelMask4  0x00001110
#define qcolorMask4    0x0000CCC0
#define qlowpixelMask4 0x00003330

#define INTERPOLATE4(A, B) ((((A & colorMask4) >> 1) + ((B & colorMask4) >> 1) + (A & B & lowPixelMask4))|((((A&0x0000000F)==0x00000006)?0x00000006:(((B&0x0000000F)==0x00000006)?0x00000006:(((A&0x0000000F)==0x00000000)?0x00000000:(((B&0x0000000F)==0x00000000)?0x00000000:0x0000000F))))))

#define Q_INTERPOLATE4(A, B, C, D) ((((A & qcolorMask4) >> 2) + ((B & qcolorMask4) >> 2) + ((C & qcolorMask4) >> 2) + ((D & qcolorMask4) >> 2) + ((((A & qlowpixelMask4) + (B & qlowpixelMask4) + (C & qlowpixelMask4) + (D & qlowpixelMask4)) >> 2) & qlowpixelMask4))| ((((A&0x0000000F)==0x00000006)?0x00000006:(((B&0x0000000F)==0x00000006)?0x00000006:(((C&0x0000000F)==0x00000006)?0x00000006:(((D&0x0000000F)==0x00000006)?0x00000006:(((A&0x0000000F)==0x00000000)?0x00000000:(((B&0x0000000F)==0x00000000)?0x00000000:(((C&0x0000000F)==0x00000000)?0x00000000:(((D&0x0000000F)==0x00000000)?0x00000000:0x0000000F))))))))))

void Super2xSaI_ex4(unsigned char *srcPtr, DWORD srcPitch,
	            unsigned char  *dstBitmap, int width, int height)
{
 DWORD dstPitch = srcPitch * 2;
 DWORD line;
 unsigned short *dP;
 unsigned short *bP;
 int   width2 = width*2;
 int iXA,iXB,iXC,iYA,iYB,iYC,finish;
 DWORD color4, color5, color6;
 DWORD color1, color2, color3;
 DWORD colorA0, colorA1, colorA2, colorA3,
       colorB0, colorB1, colorB2, colorB3,
       colorS1, colorS2;
 DWORD product1a, product1b,
       product2a, product2b;

 line = 0;

  {
   for (; height; height-=1)
	{
     bP = (unsigned short *)srcPtr;
	 dP = (unsigned short *)(dstBitmap + line*dstPitch);
     for (finish = width; finish; finish -= 1 )
      {
//---------------------------------------    B1 B2
//                                         4  5  6 S2
//                                         1  2  3 S1
//                                           A1 A2
       if(finish==width) iXA=0;
       else              iXA=1;
       if(finish>4) {iXB=1;iXC=2;}
       else
       if(finish>3) {iXB=1;iXC=1;}
       else         {iXB=0;iXC=0;}
       if(line==0) iYA=0;
       else        iYA=width;
       if(height>4) {iYB=width;iYC=width2;}
       else
       if(height>3) {iYB=width;iYC=width;}
       else         {iYB=0;iYC=0;}


       colorB0 = *(bP- iYA - iXA);
       colorB1 = *(bP- iYA);
       colorB2 = *(bP- iYA + iXB);
       colorB3 = *(bP- iYA + iXC);

       color4 = *(bP  - iXA);
       color5 = *(bP);
       color6 = *(bP  + iXB);
       colorS2 = *(bP + iXC);

       color1 = *(bP  + iYB  - iXA);
       color2 = *(bP  + iYB);
       color3 = *(bP  + iYB  + iXB);
       colorS1= *(bP  + iYB  + iXC);

       colorA0 = *(bP + iYC - iXA);
       colorA1 = *(bP + iYC);
       colorA2 = *(bP + iYC + iXB);
       colorA3 = *(bP + iYC + iXC);

//--------------------------------------
       if (color2 == color6 && color5 != color3)
        {
         product2b = product1b = color2;
        }
       else
       if (color5 == color3 && color2 != color6)
        {
         product2b = product1b = color5;
        }
       else
       if (color5 == color3 && color2 == color6)
        {
         register int r = 0;

         r += GET_RESULT ((color6&0xfffffff0), (color5&0xfffffff0), (color1&0xfffffff0),  (colorA1&0xfffffff0));
         r += GET_RESULT ((color6&0xfffffff0), (color5&0xfffffff0), (color4&0xfffffff0),  (colorB1&0xfffffff0));
         r += GET_RESULT ((color6&0xfffffff0), (color5&0xfffffff0), (colorA2&0xfffffff0), (colorS1&0xfffffff0));
         r += GET_RESULT ((color6&0xfffffff0), (color5&0xfffffff0), (colorB2&0xfffffff0), (colorS2&0xfffffff0));

         if (r > 0)
          product2b = product1b = color6;
         else
         if (r < 0)
          product2b = product1b = color5;
         else
          {
           product2b = product1b = INTERPOLATE4 (color5, color6);
          }
        }
       else
        {
         if (color6 == color3 && color3 == colorA1 && color2 != colorA2 && color3 != colorA0)
             product2b = Q_INTERPOLATE4 (color3, color3, color3, color2);
         else
         if (color5 == color2 && color2 == colorA2 && colorA1 != color3 && color2 != colorA3)
             product2b = Q_INTERPOLATE4 (color2, color2, color2, color3);
         else
             product2b = INTERPOLATE4 (color2, color3);

         if (color6 == color3 && color6 == colorB1 && color5 != colorB2 && color6 != colorB0)
             product1b = Q_INTERPOLATE4 (color6, color6, color6, color5);
         else
         if (color5 == color2 && color5 == colorB2 && colorB1 != color6 && color5 != colorB3)
             product1b = Q_INTERPOLATE4 (color6, color5, color5, color5);
         else
             product1b = INTERPOLATE4 (color5, color6);
        }

       if (color5 == color3 && color2 != color6 && color4 == color5 && color5 != colorA2)
        product2a = INTERPOLATE4 (color2, color5);
       else
       if (color5 == color1 && color6 == color5 && color4 != color2 && color5 != colorA0)
        product2a = INTERPOLATE4(color2, color5);
       else
        product2a = color2;

       if (color2 == color6 && color5 != color3 && color1 == color2 && color2 != colorB2)
        product1a = INTERPOLATE4 (color2, color5);
       else
       if (color4 == color2 && color3 == color2 && color1 != color5 && color2 != colorB0)
        product1a = INTERPOLATE4(color2, color5);
       else
        product1a = color5;

       *dP=product1a;
       *(dP+1)=product1b;
       *(dP+(width2))=product2a;
       *(dP+1+(width2))=product2b;

       bP += 1;
       dP += 2;
      }//end of for ( finish= width etc..)

     line += 2;
     srcPtr += srcPitch;
	}; //endof: for (; height; height--)
  }
}

/////////////////////////////////////////////////////////////////////////////

#define colorMask5     0x0000F7BC
#define lowPixelMask5  0x00000842
#define qcolorMask5    0x0000E738
#define qlowpixelMask5 0x000018C6

#define INTERPOLATE5(A, B) ((((A & colorMask5) >> 1) + ((B & colorMask5) >> 1) + (A & B & lowPixelMask5))|((((A&0x00000001)==0x00000000)?0x00000000:(((B&0x00000001)==0x00000000)?0x00000000:0x00000001))))

#define Q_INTERPOLATE5(A, B, C, D) ((((A & qcolorMask5) >> 2) + ((B & qcolorMask5) >> 2) + ((C & qcolorMask5) >> 2) + ((D & qcolorMask5) >> 2) + ((((A & qlowpixelMask5) + (B & qlowpixelMask5) + (C & qlowpixelMask5) + (D & qlowpixelMask5)) >> 2) & qlowpixelMask5))| ((((A&0x00000001)==0x00000000)?0x00000000:(((B&0x00000001)==0x00000000)?0x00000000:(((C&0x00000001)==0x00000000)?0x00000000:(((D&0x00000001)==0x00000000)?0x00000000:0x00000001))))))

void Super2xSaI_ex5(unsigned char *srcPtr, DWORD srcPitch,
	            unsigned char  *dstBitmap, int width, int height)
{
 DWORD dstPitch = srcPitch * 2;
 DWORD line;
 unsigned short *dP;
 unsigned short *bP;
 int   width2 = width*2;
 int iXA,iXB,iXC,iYA,iYB,iYC,finish;
 DWORD color4, color5, color6;
 DWORD color1, color2, color3;
 DWORD colorA0, colorA1, colorA2, colorA3,
       colorB0, colorB1, colorB2, colorB3,
       colorS1, colorS2;
 DWORD product1a, product1b,
       product2a, product2b;

 line = 0;

  {
   for (; height; height-=1)
	{
     bP = (unsigned short *)srcPtr;
	 dP = (unsigned short *)(dstBitmap + line*dstPitch);
     for (finish = width; finish; finish -= 1 )
      {
//---------------------------------------    B1 B2
//                                         4  5  6 S2
//                                         1  2  3 S1
//                                           A1 A2
       if(finish==width) iXA=0;
       else              iXA=1;
       if(finish>4) {iXB=1;iXC=2;}
       else
       if(finish>3) {iXB=1;iXC=1;}
       else         {iXB=0;iXC=0;}
       if(line==0) iYA=0;
       else        iYA=width;
       if(height>4) {iYB=width;iYC=width2;}
       else
       if(height>3) {iYB=width;iYC=width;}
       else         {iYB=0;iYC=0;}


       colorB0 = *(bP- iYA - iXA);
       colorB1 = *(bP- iYA);
       colorB2 = *(bP- iYA + iXB);
       colorB3 = *(bP- iYA + iXC);

       color4 = *(bP  - iXA);
       color5 = *(bP);
       color6 = *(bP  + iXB);
       colorS2 = *(bP + iXC);

       color1 = *(bP  + iYB  - iXA);
       color2 = *(bP  + iYB);
       color3 = *(bP  + iYB  + iXB);
       colorS1= *(bP  + iYB  + iXC);

       colorA0 = *(bP + iYC - iXA);
       colorA1 = *(bP + iYC);
       colorA2 = *(bP + iYC + iXB);
       colorA3 = *(bP + iYC + iXC);

//--------------------------------------
       if (color2 == color6 && color5 != color3)
        {
         product2b = product1b = color2;
        }
       else
       if (color5 == color3 && color2 != color6)
        {
         product2b = product1b = color5;
        }
       else
       if (color5 == color3 && color2 == color6)
        {
         register int r = 0;

         r += GET_RESULT ((color6&0xfffffffe), (color5&0xfffffffe), (color1&0xfffffffe),  (colorA1&0xfffffffe));
         r += GET_RESULT ((color6&0xfffffffe), (color5&0xfffffffe), (color4&0xfffffffe),  (colorB1&0xfffffffe));
         r += GET_RESULT ((color6&0xfffffffe), (color5&0xfffffffe), (colorA2&0xfffffffe), (colorS1&0xfffffffe));
         r += GET_RESULT ((color6&0xfffffffe), (color5&0xfffffffe), (colorB2&0xfffffffe), (colorS2&0xfffffffe));

         if (r > 0)
          product2b = product1b = color6;
         else
         if (r < 0)
          product2b = product1b = color5;
         else
          {
           product2b = product1b = INTERPOLATE5 (color5, color6);
          }
        }
       else
        {
         if (color6 == color3 && color3 == colorA1 && color2 != colorA2 && color3 != colorA0)
             product2b = Q_INTERPOLATE5 (color3, color3, color3, color2);
         else
         if (color5 == color2 && color2 == colorA2 && colorA1 != color3 && color2 != colorA3)
             product2b = Q_INTERPOLATE5 (color2, color2, color2, color3);
         else
             product2b = INTERPOLATE5 (color2, color3);

         if (color6 == color3 && color6 == colorB1 && color5 != colorB2 && color6 != colorB0)
             product1b = Q_INTERPOLATE5 (color6, color6, color6, color5);
         else
         if (color5 == color2 && color5 == colorB2 && colorB1 != color6 && color5 != colorB3)
             product1b = Q_INTERPOLATE5 (color6, color5, color5, color5);
         else
             product1b = INTERPOLATE5 (color5, color6);
        }

       if (color5 == color3 && color2 != color6 && color4 == color5 && color5 != colorA2)
        product2a = INTERPOLATE5 (color2, color5);
       else
       if (color5 == color1 && color6 == color5 && color4 != color2 && color5 != colorA0)
        product2a = INTERPOLATE5(color2, color5);
       else
        product2a = color2;

       if (color2 == color6 && color5 != color3 && color1 == color2 && color2 != colorB2)
        product1a = INTERPOLATE5(color2, color5);
       else
       if (color4 == color2 && color3 == color2 && color1 != color5 && color2 != colorB0)
        product1a = INTERPOLATE5(color2, color5);
       else
        product1a = color5;

       *dP=product1a;
       *(dP+1)=product1b;
       *(dP+(width2))=product2a;
       *(dP+1+(width2))=product2b;

       bP += 1;
       dP += 2;
      }//end of for ( finish= width etc..)

     line += 2;
     srcPtr += srcPitch;
	}; //endof: for (; height; height--)
  }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// ogl texture defines
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void DefineSubTextureSortHiRes(void)
{
#ifndef __GX__
 int x,y,dx2;
 if(gTexName==NULL)             
  {
   gTexName=malloc(sizeof(textureWndCacheEntry));
   memset(gTexName, 0, sizeof(textureWndCacheEntry));
   glGenTextures(1, &gTexName->texname);
   glBindTexture(GL_TEXTURE_2D, gTexName->texname);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, iClampType);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, iClampType);

   if(iFilterType)
    {
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
   else
    {            
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }   
   glTexImage2D(GL_TEXTURE_2D, 0, giWantedRGBA, 512, 512, 0, giWantedFMT, giWantedTYPE, texturebuffer);
  }
 else glBindTexture(GL_TEXTURE_2D, gTexName->texname);

 if(bGLExt && (iTexQuality==1 || iTexQuality==2))
  {
   if(DXTexS < 4 || DYTexS < 4 || iHiResTextures==2) 
    {
     unsigned short * pS,*pD1,*pD2;
     dx2=(DXTexS<<1);
     pS=(unsigned short *)texturepart;
     pD1=(unsigned short *)texturebuffer;
     pD2=(unsigned short *)texturebuffer;
     pD2+=dx2;
     for(y=0;y<DYTexS;y++)
      {
       for(x=0;x<DXTexS;x++)
        {
         *(pD2+1)=*pD2=*(pD1+1)=*pD1=*pS;
         pS++;
         pD1+=2;           
         pD2+=2;
        }
       pD1+=dx2;
       pD2+=dx2;
      }
    }
   else
    {
     if(iTexQuality==1)
      Super2xSaI_ex4(texturepart, DXTexS<<1, texturebuffer, DXTexS, DYTexS);
     else
      Super2xSaI_ex5(texturepart, DXTexS<<1, texturebuffer, DXTexS, DYTexS);
    }
  }
 else
  {
   if(DXTexS < 4 || DYTexS < 4 || iHiResTextures==2) 
    {
     u32 * pS,*pD1,*pD2;
     dx2=(DXTexS<<1);
     pS=(u32 *)texturepart;
     pD1=(u32 *)texturebuffer;
     pD2=(u32 *)texturebuffer;
     pD2+=dx2;
     for(y=0;y<DYTexS;y++)
      {
       for(x=0;x<DXTexS;x++)
        {
         *(pD2+1)=*pD2=*(pD1+1)=*pD1=*pS;
         pS++;
         pD1+=2;           
         pD2+=2;
        }
       pD1+=dx2;
       pD2+=dx2;
      }
    }
   else
   if(bSmallAlpha)
    Super2xSaI_ex8_Ex(texturepart, DXTexS*4, texturebuffer, DXTexS, DYTexS);
   else
    Super2xSaI_ex8(texturepart, DXTexS*4, texturebuffer, DXTexS, DYTexS);
  }

 glTexSubImage2D(GL_TEXTURE_2D, 0, XTexS<<1, YTexS<<1,
                 DXTexS<<1, DYTexS<<1,
                 giWantedFMT, giWantedTYPE, texturebuffer);
#endif
}

void DefineSubTextureSort(void)
{
 if(iHiResTextures)
  {
   DefineSubTextureSortHiRes();
   return;
  }
#ifndef __GX__
 if(gTexName==NULL)
  {
   gTexName=malloc(sizeof(textureWndCacheEntry));
   memset(gTexName, 0, sizeof(textureWndCacheEntry));
   glGenTextures(1, &gTexName->texname);
   glBindTexture(GL_TEXTURE_2D, gTexName->texname);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, iClampType);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, iClampType);

   if(iFilterType)
    {
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
   else
    {
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
   glTexImage2D(GL_TEXTURE_2D, 0, giWantedRGBA, 256, 256, 0,giWantedFMT, giWantedTYPE, texturepart);
  }
 else glBindTexture(GL_TEXTURE_2D, gTexName->texname);

 glTexSubImage2D(GL_TEXTURE_2D, 0, XTexS, YTexS,
                 DXTexS, DYTexS,
                 giWantedFMT, giWantedTYPE, texturepart);
#else
 
 if(gTexName==NULL)
 {
  gTexName=memalign(32, sizeof(textureWndCacheEntry));
  memset(gTexName, 0, sizeof(textureWndCacheEntry));
  createGXTexture(gTexName, 256, 256);
  u8 magFilt = iFilterType ? GX_LINEAR : GX_NEAR;
  GX_InitTexObjFilterMode(&gTexName->GXtexObj,magFilt,magFilt);
  GX_InitTexObjWrapMode(&gTexName->GXtexObj,iClampType,iClampType);
 }
  updateGXSubTexture(gTexName, texturepart, XTexS, YTexS, DXTexS, DYTexS);
  GX_LoadTexObj(&gTexName->GXtexObj, GX_TEXMAP0);
#endif
}


/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// texture cache garbage collection
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void DoTexGarbageCollection(void)
{
 static unsigned short LRUCleaned=0;
 unsigned short iC,iC1,iC2;
 int i,j,iMax;textureSubCacheEntryS * tsb;

 iC=4;//=iSortTexCnt/2,
 LRUCleaned+=iC;                                       // we clean different textures each time
 if((LRUCleaned+iC)>=iSortTexCnt) LRUCleaned=0;        // wrap? wrap!
 iC1=LRUCleaned;                                       // range of textures to clean
 iC2=LRUCleaned+iC;

 for(iC=iC1;iC<iC2;iC++)                               // make some textures available
  {
   pxSsubtexLeft[iC]->l=0;
  }

 for(i=0;i<3;i++)                                      // remove all references to that textures
  for(j=0;j<MAXTPAGES;j++)
   for(iC=0;iC<4;iC++)                                 // loop all texture rect info areas
    {
     tsb=pscSubtexStore[i][j]+(iC*SOFFB);
     iMax=tsb->pos.l;
     if(iMax)
      do
       {
        tsb++;
        if(tsb->cTexID>=iC1 && tsb->cTexID<iC2)        // info uses the cleaned textures? remove info
         tsb->ClutID=0;
       } 
      while(--iMax);
     }

 usLRUTexPage=LRUCleaned;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// search cache for existing (already used) parts
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

unsigned char * CheckTextureInSubSCache(s32 TextureMode,u32 GivenClutId,unsigned short * pCache)
{
 heap_iblock heapBlock;
 __lwp_heap_getinfo(GXtexCache, &heapBlock);
 sprintf(txtbuffer,"tex used: %iKb free: %iKb\r\n", heapBlock.used_size/1024, heapBlock.free_size/1024);
 DEBUG_print(txtbuffer,DBG_GPU1);

 textureSubCacheEntryS * tsx, * tsb, *tsg;//, *tse=NULL;
 int i,iMax;EXLong npos;
 unsigned char cx,cy;
 int iC,j,k;u32 rx,ry,mx,my;
 EXLong * ul=0, * uls;
 EXLong rfree;
 unsigned char cXAdj,cYAdj;

 npos.l=*((u32 *)&gl_ux[4]);

 //--------------------------------------------------------------//
 // find matching texturepart first... speed up...
 //--------------------------------------------------------------//

 tsg=pscSubtexStore[TextureMode][GlobalTexturePage];
 tsg+=((GivenClutId&CLUTCHK)>>CLUTSHIFT)*SOFFB;

 iMax=tsg->pos.l;
 if(iMax)
  {
   i=iMax;
   tsb=tsg+1;                 
   do
    {
     if(GivenClutId==tsb->ClutID &&
        (INCHECK(tsb->pos,npos)))
      {
        {
         cx=tsb->pos.c[3]-tsb->posTX;
         cy=tsb->pos.c[1]-tsb->posTY;

         gl_ux[0]-=cx;
         gl_ux[1]-=cx;
         gl_ux[2]-=cx;
         gl_ux[3]-=cx;
         gl_vy[0]-=cy;
         gl_vy[1]-=cy;
         gl_vy[2]-=cy;
         gl_vy[3]-=cy;

         ubOpaqueDraw=tsb->Opaque;
         *pCache=tsb->cTexID;
         return NULL;
        }
      } 
     tsb++;
    }
   while(--i);
  }

 //----------------------------------------------------//

 cXAdj=1;cYAdj=1;

 rx=(int)gl_ux[6]-(int)gl_ux[7];
 rx+=(4-(rx%4));
 ry=(int)gl_ux[4]-(int)gl_ux[5];
 ry+=(4-(ry%4));

 tsx=NULL;tsb=tsg+1;
 for(i=0;i<iMax;i++,tsb++)
  {
   if(!tsb->ClutID) {tsx=tsb;break;}
  }

 if(!tsx) 
  {
   iMax++;
   if(iMax>=SOFFB-2) 
    {
     if(iTexGarbageCollection)                         // gc mode?
      {
       if(*pCache==0) 
        {
         dwTexPageComp|=(1<<GlobalTexturePage);
         *pCache=0xffff;
         return 0;
        }

       iMax--;
       tsb=tsg+1;

       for(i=0;i<iMax;i++,tsb++)                       // 1. search other slots with same cluts, and unite the area
        if(GivenClutId==tsb->ClutID)
         {
          if(!tsx) {tsx=tsb;rfree.l=npos.l;}           // 
          else      tsb->ClutID=0;
          rfree.c[3]=min(rfree.c[3],tsb->pos.c[3]);
          rfree.c[2]=max(rfree.c[2],tsb->pos.c[2]);
          rfree.c[1]=min(rfree.c[1],tsb->pos.c[1]);
          rfree.c[0]=max(rfree.c[0],tsb->pos.c[0]);
          MarkFree(tsb);
         }

       if(tsx)                                         // 3. if one or more found, create a new rect with bigger size
        {
         *((u32 *)&gl_ux[4])=npos.l=rfree.l;
         rx=(int)rfree.c[2]-(int)rfree.c[3];
         ry=(int)rfree.c[0]-(int)rfree.c[1];
         DoTexGarbageCollection();
       
         goto ENDLOOP3;
        }
      }

     iMax=1;
    }
   tsx=tsg+iMax;
   tsg->pos.l=iMax;
  }

 //----------------------------------------------------//
 // now get a free texture space
 //----------------------------------------------------//

 if(iTexGarbageCollection) usLRUTexPage=0;

ENDLOOP3:

 rx+=4;if(rx>255) {cXAdj=0;rx=255;}
 ry+=4;if(ry>255) {cYAdj=0;ry=255;}

 iC=usLRUTexPage;

 for(k=0;k<iSortTexCnt;k++)
  {
   uls=pxSsubtexLeft[iC];
   iMax=uls->l;ul=uls+1;

   //--------------------------------------------------//
   // first time

   if(!iMax) 
    {
     rfree.l=0;

     if(rx>252 && ry>252)
      {uls->l=1;ul->l=0xffffffff;ul=0;goto ENDLOOP;}

     if(rx<253)
      {
       uls->l=uls->l+1;
       ul->c[3]=rx;
       ul->c[2]=255-rx;
       ul->c[1]=0;
       ul->c[0]=ry;
       ul++;
      }

     if(ry<253)
      {
       uls->l=uls->l+1; 
       ul->c[3]=0;
       ul->c[2]=255;
       ul->c[1]=ry;
       ul->c[0]=255-ry;
      }
     ul=0;
     goto ENDLOOP;
    }
                                                       
   //--------------------------------------------------//
   for(i=0;i<iMax;i++,ul++)
    {
     if(ul->l!=0xffffffff && 
        ry<=ul->c[0]      && 
        rx<=ul->c[2])
      {
       rfree=*ul;
       mx=ul->c[2]-2;
       my=ul->c[0]-2;
       if(rx<mx && ry<my)
        {
         ul->c[3]+=rx;
         ul->c[2]-=rx;
         ul->c[0]=ry;

         for(ul=uls+1,j=0;j<iMax;j++,ul++)
          if(ul->l==0xffffffff) break;
 
         if(j<CSUBSIZE-2)
          {
           if(j==iMax) uls->l=uls->l+1;

           ul->c[3]=rfree.c[3];
           ul->c[2]=rfree.c[2];
           ul->c[1]=rfree.c[1]+ry;
           ul->c[0]=rfree.c[0]-ry;
          }
        }
       else if(rx<mx)
        {
         ul->c[3]+=rx;
         ul->c[2]-=rx;
        }
       else if(ry<my)
        {
         ul->c[1]+=ry;
         ul->c[0]-=ry;
        }
       else
        {
         ul->l=0xffffffff;
        }
       ul=0;
       goto ENDLOOP;
      }
    }

   //--------------------------------------------------//

   iC++; if(iC>=iSortTexCnt) iC=0;
  }

 //----------------------------------------------------//
 // check, if free space got
 //----------------------------------------------------//

ENDLOOP:
 if(ul)
  {
   //////////////////////////////////////////////////////

    {
     dwTexPageComp=0;

     for(i=0;i<3;i++)                                    // cleaning up
      for(j=0;j<MAXTPAGES;j++)
       {
        tsb=pscSubtexStore[i][j];
        (tsb+SOFFA)->pos.l=0;
        (tsb+SOFFB)->pos.l=0;
        (tsb+SOFFC)->pos.l=0;
        (tsb+SOFFD)->pos.l=0;
       }
     for(i=0;i<iSortTexCnt;i++)
      {ul=pxSsubtexLeft[i];ul->l=0;}
     usLRUTexPage=0;
    }

   //////////////////////////////////////////////////////
   iC=usLRUTexPage;
   uls=pxSsubtexLeft[usLRUTexPage];
   uls->l=0;ul=uls+1;
   rfree.l=0;

   if(rx>252 && ry>252)
    {uls->l=1;ul->l=0xffffffff;}
   else
    {
     if(rx<253)
      {
       uls->l=uls->l+1;
       ul->c[3]=rx;
       ul->c[2]=255-rx;
       ul->c[1]=0;
       ul->c[0]=ry;
       ul++;
      }
     if(ry<253)
      {
       uls->l=uls->l+1; 
       ul->c[3]=0;
       ul->c[2]=255;
       ul->c[1]=ry;
       ul->c[0]=255-ry;
      }
    }
   tsg->pos.l=1;tsx=tsg+1;
  }

 rfree.c[3]+=cXAdj;
 rfree.c[1]+=cYAdj;

 tsx->cTexID   =*pCache=iC;
 tsx->pos      = npos;
 tsx->ClutID   = GivenClutId;
 tsx->posTX    = rfree.c[3];
 tsx->posTY    = rfree.c[1];

 cx=gl_ux[7]-rfree.c[3];
 cy=gl_ux[5]-rfree.c[1];

 gl_ux[0]-=cx;
 gl_ux[1]-=cx;
 gl_ux[2]-=cx;
 gl_ux[3]-=cx;
 gl_vy[0]-=cy;
 gl_vy[1]-=cy;
 gl_vy[2]-=cy;
 gl_vy[3]-=cy;

 XTexS=rfree.c[3];
 YTexS=rfree.c[1];

 return &tsx->Opaque;
}
                   
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// search cache for free place (on compress)
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL GetCompressTexturePlace(textureSubCacheEntryS * tsx)
{
 int i,j,k,iMax,iC;u32 rx,ry,mx,my;
 EXLong * ul=0, * uls, rfree;
 unsigned char cXAdj=1,cYAdj=1;

 rx=(int)tsx->pos.c[2]-(int)tsx->pos.c[3];
 ry=(int)tsx->pos.c[0]-(int)tsx->pos.c[1];

 rx+=4;if(rx>255) {cXAdj=0;rx=255;}
 ry+=4;if(ry>255) {cYAdj=0;ry=255;}

 iC=usLRUTexPage;

 for(k=0;k<iSortTexCnt;k++)
  {
   uls=pxSsubtexLeft[iC];
   iMax=uls->l;ul=uls+1;

   //--------------------------------------------------//
   // first time

   if(!iMax)
    {
     rfree.l=0;

     if(rx>252 && ry>252)
      {uls->l=1;ul->l=0xffffffff;ul=0;goto TENDLOOP;}

     if(rx<253)
      {
       uls->l=uls->l+1;
       ul->c[3]=rx;
       ul->c[2]=255-rx;
       ul->c[1]=0;
       ul->c[0]=ry;
       ul++;
      }

     if(ry<253)
      {
       uls->l=uls->l+1;
       ul->c[3]=0;
       ul->c[2]=255;
       ul->c[1]=ry;
       ul->c[0]=255-ry;
      }
     ul=0;
     goto TENDLOOP;
    }

   //--------------------------------------------------//
   for(i=0;i<iMax;i++,ul++)
    {
     if(ul->l!=0xffffffff &&
        ry<=ul->c[0]      &&
        rx<=ul->c[2])
      {
       rfree=*ul;
       mx=ul->c[2]-2;
       my=ul->c[0]-2;

       if(rx<mx && ry<my)
        {
         ul->c[3]+=rx;
         ul->c[2]-=rx;
         ul->c[0]=ry;

         for(ul=uls+1,j=0;j<iMax;j++,ul++)
          if(ul->l==0xffffffff) break;

         if(j<CSUBSIZE-2)
          {
           if(j==iMax) uls->l=uls->l+1;

           ul->c[3]=rfree.c[3];
           ul->c[2]=rfree.c[2];
           ul->c[1]=rfree.c[1]+ry;
           ul->c[0]=rfree.c[0]-ry;
          }
        }
       else if(rx<mx)
        {
         ul->c[3]+=rx;
         ul->c[2]-=rx;
        }
       else if(ry<my)
        {
         ul->c[1]+=ry;
         ul->c[0]-=ry;
        }
       else
        {
         ul->l=0xffffffff;
        }
       ul=0;
       goto TENDLOOP;
      }
    }

   //--------------------------------------------------//

   iC++; if(iC>=iSortTexCnt) iC=0;
  }

 //----------------------------------------------------//
 // check, if free space got
 //----------------------------------------------------//

TENDLOOP:
 if(ul) return FALSE;

 rfree.c[3]+=cXAdj;
 rfree.c[1]+=cYAdj;

 tsx->cTexID   = iC;
 tsx->posTX    = rfree.c[3];
 tsx->posTY    = rfree.c[1];

 XTexS=rfree.c[3];
 YTexS=rfree.c[1];

 return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// compress texture cache (to make place for new texture part, if needed)
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void CompressTextureSpace(void)
{
 textureSubCacheEntryS * tsx, * tsg, * tsb;
 int i,j,k,m,n,iMax;EXLong * ul, r,opos;
 short sOldDST=DrawSemiTrans,cx,cy;
 s32  lOGTP=GlobalTexturePage;
 u32 l,row;
 u32 * lSRCPtr;

 opos.l=*((u32 *)&gl_ux[4]);

 // 1. mark all textures as free
 for(i=0;i<iSortTexCnt;i++)
  {ul=pxSsubtexLeft[i];ul->l=0;}
 usLRUTexPage=0;

 // 2. compress
 for(j=0;j<3;j++)
  {
   for(k=0;k<MAXTPAGES;k++)
    {
     tsg=pscSubtexStore[j][k];

     if((!(dwTexPageComp&(1<<k))))
      {
       (tsg+SOFFA)->pos.l=0;
       (tsg+SOFFB)->pos.l=0;
       (tsg+SOFFC)->pos.l=0;
       (tsg+SOFFD)->pos.l=0;
       continue;
      }

     for(m=0;m<4;m++,tsg+=SOFFB)
      {
       iMax=tsg->pos.l;

       tsx=tsg+1;
       for(i=0;i<iMax;i++,tsx++)
        {
         if(tsx->ClutID)
          {
           r.l=tsx->pos.l;
           for(n=i+1,tsb=tsx+1;n<iMax;n++,tsb++)
            {
             if(tsx->ClutID==tsb->ClutID)
              {
               r.c[3]=min(r.c[3],tsb->pos.c[3]);
               r.c[2]=max(r.c[2],tsb->pos.c[2]);
               r.c[1]=min(r.c[1],tsb->pos.c[1]);
               r.c[0]=max(r.c[0],tsb->pos.c[0]);
               tsb->ClutID=0;
              }
            }

//           if(r.l!=tsx->pos.l)
            {
             cx=((tsx->ClutID << 4) & 0x3F0);          
             cy=((tsx->ClutID >> 6) & CLUTYMASK);

             if(j!=2)
              {
               // palette check sum
               l=0;lSRCPtr=(u32 *)(psxVuw+cx+(cy*1024));
               if(j==1) for(row=1;row<129;row++) l+=((*lSRCPtr++)-1)*row;
               else     for(row=1;row<9;row++)   l+=((*lSRCPtr++)-1)<<row;
               l=((l+HIWORD(l))&0x3fffL)<<16;
               if(l!=(tsx->ClutID&(0x00003fff<<16)))
                {
                 tsx->ClutID=0;continue;
                }
              }

             tsx->pos.l=r.l;
             if(!GetCompressTexturePlace(tsx))         // no place?
              {
               for(i=0;i<3;i++)                        // -> clean up everything
                for(j=0;j<MAXTPAGES;j++)
                 {
                  tsb=pscSubtexStore[i][j];
                  (tsb+SOFFA)->pos.l=0;
                  (tsb+SOFFB)->pos.l=0;
                  (tsb+SOFFC)->pos.l=0;
                  (tsb+SOFFD)->pos.l=0;
                 }
               for(i=0;i<iSortTexCnt;i++)
                {ul=pxSsubtexLeft[i];ul->l=0;}
               usLRUTexPage=0;
               DrawSemiTrans=sOldDST;
               GlobalTexturePage=lOGTP;
               *((u32 *)&gl_ux[4])=opos.l;
               dwTexPageComp=0;

               return;
              }

             if(tsx->ClutID&(1<<30)) DrawSemiTrans=1;
             else                    DrawSemiTrans=0;
             *((u32 *)&gl_ux[4])=r.l;
   
             gTexName=uiStexturePage[tsx->cTexID];
             LoadSubTexFn(k,j,cx,cy);
             uiStexturePage[tsx->cTexID]=gTexName;
             tsx->Opaque=ubOpaqueDraw;
            }
          }
        }

       if(iMax)  
        {
         tsx=tsg+iMax;
         while(!tsx->ClutID && iMax) {tsx--;iMax--;}
         tsg->pos.l=iMax;
        }

      }                      
    }
  }

 if(dwTexPageComp==0xffffffff) dwTexPageComp=0;

 *((u32 *)&gl_ux[4])=opos.l;
 GlobalTexturePage=lOGTP;
 DrawSemiTrans=sOldDST;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// main entry for searching/creating textures, called from prim.c
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

textureWndCacheEntry* SelectSubTextureS(s32 TextureMode, u32 GivenClutId) 
{
 unsigned char * OPtr;unsigned short iCache;short cx,cy;

 // sort sow/tow infos for fast access

 unsigned char ma1,ma2,mi1,mi2;
 if(gl_ux[0]>gl_ux[1]) {mi1=gl_ux[1];ma1=gl_ux[0];}
 else                  {mi1=gl_ux[0];ma1=gl_ux[1];}
 if(gl_ux[2]>gl_ux[3]) {mi2=gl_ux[3];ma2=gl_ux[2];}
 else                  {mi2=gl_ux[2];ma2=gl_ux[3];}
 if(mi1>mi2) gl_ux[7]=mi2; 
 else        gl_ux[7]=mi1;
 if(ma1>ma2) gl_ux[6]=ma1; 
 else        gl_ux[6]=ma2;

 if(gl_vy[0]>gl_vy[1]) {mi1=gl_vy[1];ma1=gl_vy[0];}
 else                  {mi1=gl_vy[0];ma1=gl_vy[1];}
 if(gl_vy[2]>gl_vy[3]) {mi2=gl_vy[3];ma2=gl_vy[2];}
 else                  {mi2=gl_vy[2];ma2=gl_vy[3];}
 if(mi1>mi2) gl_ux[5]=mi2; 
 else        gl_ux[5]=mi1;
 if(ma1>ma2) gl_ux[4]=ma1; 
 else        gl_ux[4]=ma2;

 // get clut infos in one 32 bit val

 if(TextureMode==2)                                    // no clut here
  {
   GivenClutId=CLUTUSED|(DrawSemiTrans<<30);cx=cy=0;
 
   if(iFrameTexType && Fake15BitTexture()) 
    return gTexName;
  }           
 else 
  {
   cx=((GivenClutId << 4) & 0x3F0);                    // but here
   cy=((GivenClutId >> 6) & CLUTYMASK);
   GivenClutId=(GivenClutId&CLUTMASK)|(DrawSemiTrans<<30)|CLUTUSED;

   // palette check sum.. removed MMX asm, this easy func works as well
    {
     u32 l=0,row;

     u32 * lSRCPtr=(u32 *)(psxVuw+cx+(cy*1024));
     if(TextureMode==1) for(row=1;row<129;row++) l+=((*lSRCPtr++)-1)*row;
     else               for(row=1;row<9;row++)   l+=((*lSRCPtr++)-1)<<row;
     l=(l+HIWORD(l))&0x3fffL;
     GivenClutId|=(l<<16);
    }

  }

 // search cache
 iCache=0;
 OPtr=CheckTextureInSubSCache(TextureMode,GivenClutId,&iCache);

 // cache full? compress and try again
 if(iCache==0xffff)
  {
   CompressTextureSpace();
   OPtr=CheckTextureInSubSCache(TextureMode,GivenClutId,&iCache);
  }

 // found? fine
 usLRUTexPage=iCache;
 if(!OPtr) {return uiStexturePage[iCache];}

 // not found? upload texture and store infos in cache
 gTexName=uiStexturePage[iCache];	
 LoadSubTexFn(GlobalTexturePage,TextureMode,cx,cy);
 uiStexturePage[iCache]=gTexName;
 *OPtr=ubOpaqueDraw;
 return gTexName;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
