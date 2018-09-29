/***************************************************************************
                          texture.h  -  description
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

#ifndef _GPU_TEXTURE_H_
#define _GPU_TEXTURE_H_

#define TEXTUREPAGESIZE 256*256

// "texture window" cache entry
typedef struct textureWndCacheEntryTag
{
 u32  ClutID;
 short          pageid;
 short          textureMode;
 short          Opaque;
 short          used;
 EXLong         pos;
 GLuint         texname;
 GXTexObj    	GXtexObj;
 u16		    *GXtexture;
 u8				GXtexfmt;
 u32 			GXrealWidth,GXrealHeight;
} textureWndCacheEntry;

// "standard texture" cache entry (12 byte per entry, as small as possible... we need lots of them)

typedef struct textureSubCacheEntryTagS 
{
 u32   ClutID;
 EXLong          pos;
 unsigned char   posTX;
 unsigned char   posTY;
 unsigned char   cTexID;
 unsigned char   Opaque;
} textureSubCacheEntryS;

void           InitializeTextureStore();
void           CleanupTextureStore();
textureWndCacheEntry*         LoadTextureWnd(s32 pageid,s32 TextureMode,u32 GivenClutId);
textureWndCacheEntry*         LoadTextureMovie(void);
void           InvalidateTextureArea(s32 imageX0,s32 imageY0,s32 imageX1,s32 imageY1);
void           InvalidateTextureAreaEx(void);
void           LoadTexturePage(int pageid, int mode, short cx, short cy);
void           ResetTextureArea(BOOL bDelTex);
textureWndCacheEntry*         SelectSubTextureS(s32 TextureMode, u32 GivenClutId);
void           CheckTextureMemory(void);


void           LoadSubTexturePage(int pageid, int mode, short cx, short cy);
void           LoadSubTexturePageSort(int pageid, int mode, short cx, short cy);
void           LoadPackedSubTexturePage(int pageid, int mode, short cx, short cy);
void           LoadPackedSubTexturePageSort(int pageid, int mode, short cx, short cy);
u32  XP8RGBA(u32 BGR);
u32  XP8RGBAEx(u32 BGR);
u32  XP8RGBA_0(u32 BGR);
u32  XP8RGBAEx_0(u32 BGR);
u32  XP8BGRA_0(u32 BGR);
u32  XP8BGRAEx_0(u32 BGR);
u32  XP8RGBA_1(u32 BGR);
u32  XP8RGBAEx_1(u32 BGR);
u32  XP8BGRA_1(u32 BGR);
u32  XP8BGRAEx_1(u32 BGR);
u32  P8RGBA(u32 BGR);
u32  P8BGRA(u32 BGR);
u32  CP8RGBA_0(u32 BGR);
u32  CP8RGBAEx_0(u32 BGR);
u32  CP8BGRA_0(u32 BGR);
u32  CP8BGRAEx_0(u32 BGR);
u32  CP8RGBA(u32 BGR);
u32  CP8RGBAEx(u32 BGR);
unsigned short XP5RGBA (unsigned short BGR);
unsigned short XP5RGBA_0 (unsigned short BGR);
unsigned short XP5RGBA_1 (unsigned short BGR);
unsigned short P5RGBA (unsigned short BGR);
unsigned short CP5RGBA_0 (unsigned short BGR);
unsigned short XP4RGBA (unsigned short BGR);
unsigned short XP4RGBA_0 (unsigned short BGR);
unsigned short XP4RGBA_1 (unsigned short BGR);
unsigned short P4RGBA (unsigned short BGR);
unsigned short CP4RGBA_0 (unsigned short BGR);

#endif // _TEXTURE_H_
