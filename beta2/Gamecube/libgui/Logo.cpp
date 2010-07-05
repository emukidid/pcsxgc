/**
 * WiiSX - Logo.cpp
 * Copyright (C) 2009, 2010 sepp256
 *
 * WiiSX homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/

#include "Logo.h"
#include "GraphicsGX.h"
#include <math.h>
#include "ogc/lwp_watchdog.h"

namespace menu {

#define LOGO_P_X0  0
#define LOGO_P_X1  1
#define LOGO_P_X2  9
#define LOGO_P_X25 10
#define LOGO_P_X3 11
#define LOGO_P_X4 19
#define LOGO_P_Y1 10
#define LOGO_P_Y2 19
#define LOGO_P_Z1  5
#define LOGO_P_Z2 15

// 'P' logo vertex data
s8 P_verts[] ATTRIBUTE_ALIGN (32) =
{ // x y z
   LOGO_P_X1, -LOGO_P_Y2, -LOGO_P_Z2,		// 0 (P, XZ plane, Y=-LOGO_P_Y2)
   LOGO_P_X2, -LOGO_P_Y2, -LOGO_P_Z2,		// 1		5   6 
   LOGO_P_X3, -LOGO_P_Y2, -LOGO_P_Z1,		// 2		  3 4  
   LOGO_P_X2, -LOGO_P_Y2,  LOGO_P_Z1,		// 3		    2    
   LOGO_P_X3, -LOGO_P_Y2,  LOGO_P_Z1,		// 4		0 1   
   LOGO_P_X1, -LOGO_P_Y2,  LOGO_P_Z2,		// 5
   LOGO_P_X3, -LOGO_P_Y2,  LOGO_P_Z2,		// 6

   LOGO_P_X1, -LOGO_P_Y2, -LOGO_P_Z2,		// 7  (S, XY plane, Z=-LOGO_P_Z2)
   LOGO_P_X2, -LOGO_P_Y2, -LOGO_P_Z2,		// 8		21 22
  -LOGO_P_X4, -LOGO_P_Y1, -LOGO_P_Z2,		// 9		15 16 17 18 19 20
  -LOGO_P_X3, -LOGO_P_Y1, -LOGO_P_Z2,		// 10 		
  -LOGO_P_X2, -LOGO_P_Y1, -LOGO_P_Z2,		// 11		 9 10 11 12 13 14
  -LOGO_P_X1, -LOGO_P_Y1, -LOGO_P_Z2,		// 12		             7  8
   LOGO_P_X1, -LOGO_P_Y1, -LOGO_P_Z2,		// 13
   LOGO_P_X2, -LOGO_P_Y1, -LOGO_P_Z2,		// 14
  -LOGO_P_X4,  LOGO_P_Y1, -LOGO_P_Z2,		// 15
  -LOGO_P_X3,  LOGO_P_Y1, -LOGO_P_Z2,		// 16
  -LOGO_P_X2,  LOGO_P_Y1, -LOGO_P_Z2,		// 17
  -LOGO_P_X1,  LOGO_P_Y1, -LOGO_P_Z2,		// 18
   LOGO_P_X1,  LOGO_P_Y1, -LOGO_P_Z2,		// 19
   LOGO_P_X2,  LOGO_P_Y1, -LOGO_P_Z2,		// 20
  -LOGO_P_X4,  LOGO_P_Y2, -LOGO_P_Z2,		// 21
  -LOGO_P_X3,  LOGO_P_Y2, -LOGO_P_Z2,		// 22

  -LOGO_P_X25,-LOGO_P_Y1, -LOGO_P_Z2,		// 23	Yellow band center
  -LOGO_P_X0,  LOGO_P_Y1, -LOGO_P_Z2,		// 24   Blue band center
};

// N64 logo color data
u8 logo_colors[] ATTRIBUTE_ALIGN (32) =
{ // r, g, b, a
	//'P' logo colors
	  8, 147,  48, 255,		// 0 green
	  1,  29, 169, 255,		// 1 blue
	254,  32,  21, 255,		// 2 orange/red
	255, 192,   1, 255,		// 3 yellow/gold
};

#define LOGO_MODE_MAX 1

Logo::Logo()
		: x(0),
		  y(0),
		  size(1),
		  rotateAuto(0),
		  rotateX(0),
		  rotateY(0)
{
	setVisible(false);
	srand ( gettick() );
	logoMode = rand() % LOGO_MODE_MAX;
}

Logo::~Logo()
{
}

void Logo::setLocation(float newX, float newY, float newZ)
{
	x = newX;
	y = newY;
	z = newZ;
}

void Logo::setSize(float newSize)
{
	size = newSize;
}

void Logo::setMode(int mode)
{
	logoMode = mode;
}

void Logo::updateTime(float deltaTime)
{
	//Overload in Component class
	//Add interpolator class & update here?
}

void Logo::drawComponent(Graphics& gfx)
{
	Mtx v, m, mv, tmp;            // view, model, modelview, and perspective matrices
	guVector cam = { 0.0F, 0.0F, 0.0F }, 
		up =    { 0.0F, 1.0F, 0.0F}, 
		look =  { 0.0F, 0.0F,-1.0F},
		axisX = { 1.0F, 0.0F, 0.0F},
		axisXm= {-1.0F, 0.0F, 0.0F},
		axisY = { 0.0F, 1.0F, 0.0F},
		axisYm= { 0.0F,-1.0F, 0.0F},
		axisZ = { 0.0F, 0.0F, 1.0F},
		axisZm= { 0.0F, 0.0F,-1.0F};
	s8 stickX,stickY;
	guVector center;

	guLookAt (v, &cam, &up, &look);
	rotateAuto++;

	u16 pad0 = PAD_ButtonsDown(0);
	if (pad0 & PAD_TRIGGER_Z) logoMode = (logoMode+1) % LOGO_MODE_MAX;

	//libOGC was changed such that sticks are now clamped and don't have to be by us
	stickX = PAD_SubStickX(0);
	stickY = PAD_SubStickY(0);
//	if(stickX > 18 || stickX < -18) rotateX += stickX/32;
//	if(stickY > 18 || stickY < -18) rotateY += stickY/32;
	rotateX += stickX/32;
	rotateY += stickY/32;

	// move the logo out in front of us and rotate it
	guMtxIdentity (m);

	guMtxRotAxisDeg (tmp, &axisX, 115);			//change to isometric view
	guMtxConcat (m, tmp, m);
	guMtxRotAxisDeg (tmp, &axisX, -rotateY);
	guMtxConcat (m, tmp, m);
	guMtxRotAxisDeg (tmp, &axisZ, -29);			//change to isometric view
	guMtxConcat (m, tmp, m);
	guMtxRotAxisDeg (tmp, &axisZ, rotateX);
	guMtxConcat (m, tmp, m);
	guMtxRotAxisDeg (tmp, &axisZ, -rotateAuto);		//slowly rotate logo
	guMtxConcat (m, tmp, m);
	guMtxScale (tmp, 1.5f, 1.0f, 2.0f);
	guMtxConcat (m, tmp, m);

	guMtxTransApply (m, m, x, y, z);
	guMtxConcat (v, m, mv);
	// load the modelview matrix into matrix memory
	GX_LoadPosMtxImm (mv, GX_PNMTX0);

	GX_SetCullMode (GX_CULL_NONE); // show both sides of quads
	GX_SetZMode(GX_ENABLE,GX_GEQUAL,GX_TRUE);

	GX_SetLineWidth(8,GX_TO_ZERO);

	// setup the vertex descriptor
	GX_ClearVtxDesc ();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc (GX_VA_POS, GX_INDEX8);
	GX_SetVtxDesc (GX_VA_CLR0, GX_INDEX8);
	
	// setup the vertex attribute table
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S8, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	
	// set the array stride
	if(logoMode == LOGO_P)		GX_SetArray (GX_VA_POS, P_verts, 3 * sizeof (s8));
	GX_SetArray (GX_VA_CLR0, logo_colors, 4 * sizeof (u8));
	
	GX_SetNumChans (1);
	GX_SetNumTexGens (0);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);

	if (logoMode == LOGO_P)
	{
		// 'P'
		GX_Begin (GX_QUADS, GX_VTXFMT0, 28);  //7 quads, so 28 verts
		drawQuad ( 0,  5,  3,  1, 2); //Side P, red
		drawQuad ( 5,  6,  4,  3, 2);
		drawQuad ( 7, 13, 14,  8, 3); //Side S, yellow
		drawQuad ( 9, 15, 16, 10, 0); //Side S, green
		drawQuad (11, 17, 18, 12, 0);
		drawQuad (13, 19, 20, 14, 0);
		drawQuad (15, 21, 22, 16, 1); //Side S, blue
		GX_End ();

		// setup the vertex descriptor
		GX_ClearVtxDesc ();
		GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
		GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
		GX_SetVtxDesc (GX_VA_CLR0, GX_INDEX8);
		// setup the vertex attribute table
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
		GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

		center = (guVector){(float)P_verts[4*3+0], (float)P_verts[4*3+1], (float)P_verts[4*3+2]};
		drawBand(center, axisZm, axisX, 0.0f, 10.0f, 180.0f, 7, 2);
		center = (guVector){(float)P_verts[23*3+0], (float)P_verts[23*3+1], (float)P_verts[23*3+2]};
		drawBand(center, axisXm, axisYm, 1.0f, 9.0f, 180.0f, 7, 3);
		center = (guVector){(float)P_verts[24*3+0], (float)P_verts[24*3+1], (float)P_verts[24*3+2]};
		drawBand(center, axisX, axisY, 1.0f, 9.0f, 180.0f, 7, 1);
	}
		
	//Reset GX state:
	GX_SetLineWidth(6,GX_TO_ZERO);
	gfx.drawInit();
}

void Logo::drawQuad(u8 v0, u8 v1, u8 v2, u8 v3, u8 c)
{
	// draws a quad from 4 vertex idx and one color idx
	// one 8bit position idx
	GX_Position1x8 (v0);
	// one 8bit color idx
	GX_Color1x8 (c);
	GX_Position1x8 (v1);
	GX_Color1x8 (c);
	GX_Position1x8 (v2);
	GX_Color1x8 (c);
	GX_Position1x8 (v3);
	GX_Color1x8 (c);
}

void Logo::drawLine(u8 v0, u8 v1, u8 c)
{
	// draws a line from 2 vertex idx and one color idx
	// one 8bit position idx
	GX_Position1x8 (v0);
	// one 8bit color idx
	GX_Color1x8 (c);
	GX_Position1x8 (v1);
	GX_Color1x8 (c);
}

#ifndef PI
#define PI 3.14159f
#endif

void Logo::drawBand(guVector center, guVector axis1, guVector axis2, float radius1, float radius2, float thetaMax, int numSegments, u8 c) 
{
	float theta1, theta2, cos_theta1, cos_theta2, sin_theta1, sin_theta2;
	guVector vec0, vec1, vec2, vec3, tmp;

	GX_Begin (GX_QUADS, GX_VTXFMT0, numSegments*4);  //numSegs quads
	for (int i=0; i<numSegments; i++)
	{
		theta1 = thetaMax * 2*PI/360 * i/numSegments;
		theta2 = thetaMax * 2*PI/360 * (i+1)/numSegments;
		cos_theta1 = cos ( theta1 );
		cos_theta2 = cos ( theta2 );
		sin_theta1 = sin ( theta1 );
		sin_theta2 = sin ( theta2 );

		//vec0 = radius1 * (axis1 * cos( theta1 ) + axis2 * sin( theta1 )) ;
		guVecScale(&axis1,&vec0,cos_theta1);
		guVecScale(&axis2,&tmp,sin_theta1);
		guVecAdd(&vec0,&tmp,&vec0);
		guVecScale(&vec0,&vec0,radius1);
		guVecAdd(&vec0,&center,&vec0);
		//vec1 = radius1 * (axis1 * cos( theta2 ) + axis2 * sin( theta2 )) ;
		guVecScale(&axis1,&vec1,cos_theta2);
		guVecScale(&axis2,&tmp,sin_theta2);
		guVecAdd(&vec1,&tmp,&vec1);
		guVecScale(&vec1,&vec1,radius1);
		guVecAdd(&vec1,&center,&vec1);
		//vec2 = radius2 * (axis1 * cos( theta2 ) + axis2 * sin( theta2 )) ;
		guVecScale(&axis1,&vec2,cos_theta2);
		guVecScale(&axis2,&tmp,sin_theta2);
		guVecAdd(&vec2,&tmp,&vec2);
		guVecScale(&vec2,&vec2,radius2);
		guVecAdd(&vec2,&center,&vec2);
		//vec3 = radius2 * (axis1 * cos( theta1 ) + axis2 * sin( theta1 )) ;
		guVecScale(&axis1,&vec3,cos_theta1);
		guVecScale(&axis2,&tmp,sin_theta1);
		guVecAdd(&vec3,&tmp,&vec3);
		guVecScale(&vec3,&vec3,radius2);
		guVecAdd(&vec3,&center,&vec3);

		GX_Position3f32( vec0.x, vec0.y, vec0.z );
		GX_Color1x8 (c);
		GX_Position3f32( vec1.x, vec1.y, vec1.z );
		GX_Color1x8 (c);
		GX_Position3f32( vec2.x, vec2.y, vec2.z );
		GX_Color1x8 (c);
		GX_Position3f32( vec3.x, vec3.y, vec3.z );
		GX_Color1x8 (c);
	}
	GX_End ();
}

} //namespace menu 
