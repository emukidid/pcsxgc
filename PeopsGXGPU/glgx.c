#include "glgx.h"

Mtx GXmodelViewIdent;

static scissor gxCurScissor;
static scissor gxCurViewPort;
static GXColor _clearcolor;
static float _cleardepth;

void gxScissor(u32 x,u32 y,u32 width,u32 height) {
	gxCurScissor.x = x;
	gxCurScissor.y = y;
	gxCurScissor.width = width;
	gxCurScissor.height = height;
}

void gxViewport(f32 xOrig,f32 yOrig,f32 wd,f32 ht,f32 nearZ,f32 farZ) {
	GX_SetViewport(xOrig, yOrig, wd, ht, nearZ, farZ);
	gxCurViewPort.x = (u32)xOrig;
	gxCurViewPort.y = (u32)yOrig;
	gxCurViewPort.width = (u32)wd;
	gxCurViewPort.height = (u32)ht;
}

void gxEnable(u32 type) {
	switch (type) {
		case GX_SCISSOR_TEST:
			GX_SetScissor(gxCurScissor.x, gxCurScissor.y, gxCurScissor.width, gxCurScissor.height);
			break;
	}
}

void gxDisable(u32 type) {
	switch (type) {
		case GX_SCISSOR_TEST:
			GX_SetScissor(gxCurViewPort.x, gxCurViewPort.y, gxCurViewPort.width, gxCurViewPort.height);
			break;
	}
}

void gxFlush()
{
	GX_CopyDisp (xfb[whichfb], GX_TRUE);
	GX_DrawDone();
	VIDEO_SetNextFramebuffer(xfb[whichfb]);
	VIDEO_Flush();
	whichfb ^= 1;
}

void gxClearColor(	GLclampf red,
					GLclampf green,
					GLclampf blue,
					GLclampf alpha ) {
					
	_clearcolor.r = red * 0xff;
	_clearcolor.g = green * 0xff;
	_clearcolor.b = blue * 0xff;
	_clearcolor.a = alpha * 0xff;
}

static void draw_axis_align_blanker_quad()
{
	float x1 = gxCurViewPort.x;
	float y1 = gxCurViewPort.y;
	float x2 = gxCurViewPort.width;
	float y2 = gxCurViewPort.height;
	/*Mtx44 GXprojection2D;
	Mtx identity_matrix;

	guMtxIdentity(identity_matrix);
	GX_LoadPosMtxImm(identity_matrix,GX_PNMTX1);
	guOrtho(GXprojection2D, x1, y2, y1, x2, 0, 1.0f);
	GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC); //load current 2D projection matrix
	GX_SetCurrentMtx(GX_PNMTX1);
	*/
	//draw rectangle from ulx,uly to lrx,lry
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

	//disable textures
	GX_SetNumChans (1);
	GX_SetNumTexGens (0);
	GX_SetNumTevStages(1);
	GX_SetNumIndStages(0);
	
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	
	//set blend mode
	GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR); //Fix src alpha
	//GX_SetDstAlpha(GX_DISABLE, 0xFF);
	
	//set cull mode
	GX_SetCullMode (GX_CULL_NONE);
	
	GX_DrawDone();

	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
	// background rectangle
	GX_Position3f32(x1, y1, -_cleardepth);
	GX_Color4u8(_clearcolor.r, _clearcolor.g, _clearcolor.b, _clearcolor.a);
	GX_Position3f32(x2, y1, -_cleardepth);
	GX_Color4u8(_clearcolor.r, _clearcolor.g, _clearcolor.b, _clearcolor.a);
	GX_Position3f32(x2, y2, -_cleardepth);
	GX_Color4u8(_clearcolor.r, _clearcolor.g, _clearcolor.b, _clearcolor.a);
	GX_Position3f32(x1, y2, -_cleardepth);
	GX_Color4u8(_clearcolor.r, _clearcolor.g, _clearcolor.b, _clearcolor.a);
	GX_End();
	
	GX_DrawDone();
}


void gxClear (GLbitfield mask) {

	bool clear_color = false;
	bool clear_depth = false;
	
	if(mask & GL_COLOR_BUFFER_BIT)
	{
		clear_color = true;
	}
	
	if(mask & GL_DEPTH_BUFFER_BIT)
	{
		clear_depth = true;
	}
	
	if(clear_color && clear_depth)
	{		
		// Clear both color and depth
		GX_SetCopyClear(_clearcolor, _cleardepth * GX_MAX_Z24);
		GX_CopyDisp(xfb[whichfb^1], GX_TRUE);
		GX_DrawDone();
		return;
	}
	else if(clear_color)
	{
		// Disable Z-write and enable color and alpha update
		GX_SetZMode(GX_DISABLE, GX_NEVER, GX_FALSE);
		GX_SetColorUpdate(GX_ENABLE);
		GX_SetAlphaUpdate(GX_ENABLE);
		draw_axis_align_blanker_quad();
	}
	else if(clear_depth)
	{
		//to clear only one of buffers then something more clever has to be done (thanks to samson)
		//Disable colour-write, enable zwrite, disable z-test, write a screen-aligned quad at whatever depth you want.
		
		// Disable Z-write and enable color and alpha update
		GX_SetZMode(GX_ENABLE, GX_ALWAYS, GX_TRUE);
		GX_SetColorUpdate(GX_DISABLE);
		GX_SetAlphaUpdate(GX_DISABLE);
		draw_axis_align_blanker_quad();
	}
}

