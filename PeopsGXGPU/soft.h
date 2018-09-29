/***************************************************************************
                         soft.h  -  description
                             -------------------
    begin                : Sun Oct 28 2001
    copyright            : (C) 2001 by Pete Bernert
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
// 2001/10/28 - Pete  
// - generic cleanup for the Peops release
//
//*************************************************************************// 

#ifndef _GPU_SOFT_H_
#define _GPU_SOFT_H_

// internally used defines

#define RED(x) ((x>>24) & 0xff)
#define BLUE(x) ((x>>8) & 0xff)
#define GREEN(x) ((x>>16) & 0xff)
#define COLOR(x) SWAP32(x & 0xffffff)

///////////////////////////////////////////////////////////////////////

void offsetPSXLine(void);
void offsetPSX2(void);
void offsetPSX3(void);
void offsetPSX4(void);

void FillSoftwareAreaTrans(short x0,short y0,short x1,short y1,unsigned short col);
void FillSoftwareArea(short x0,short y0,short x1,short y1,unsigned short col);
void drawPoly3G(s32 rgb1, s32 rgb2, s32 rgb3);
void drawPoly4G(s32 rgb1, s32 rgb2, s32 rgb3, s32 rgb4);
void drawPoly3F(s32 rgb);
void drawPoly4F(s32 rgb);
void drawPoly4FT(unsigned char * baseAddr);
void drawPoly4GT(unsigned char * baseAddr);
void drawPoly3FT(unsigned char * baseAddr);
void drawPoly3GT(unsigned char * baseAddr);
void DrawSoftwareSprite(unsigned char * baseAddr,short w,short h,s32 tx,s32 ty);
void DrawSoftwareSpriteTWin(unsigned char * baseAddr,s32 w,s32 h);
void DrawSoftwareSpriteMirror(unsigned char * baseAddr,s32 w,s32 h);

#endif // _GPU_SOFT_H_
