#ifndef _GLGX_H_
#define _GLGX_H_

#include <gccore.h>
#include "gl.h"

extern u32* xfb[2];			/*** Framebuffers ***/
extern int whichfb;        	/*** Frame buffer toggle ***/
extern GXRModeObj *vmode;	/*** Graphics Mode Object ***/
extern Mtx GXmodelViewIdent;

typedef struct _scissor {
	u32 x;
	u32 y;
	u32 width;
	u32 height;
} scissor;

enum _gxtypes {
	GX_SCISSOR_TEST=0,
	GX_ALPHA_TEST,
	GX_DEPTH_TEST,
	GX_TEXTURE_2D,
	GX_BLEND_TEST
};

void gxScissor(u32 x,u32 y,u32 width,u32 height);
void gxViewport(f32 xOrig,f32 yOrig,f32 wd,f32 ht,f32 nearZ,f32 farZ);
void gxEnable(u32 type);
void gxDisable(u32 type);
void gxFlush();
void gxClear (GLbitfield mask);
void gxClearColor(	GLclampf red,
					GLclampf green,
					GLclampf blue,
					GLclampf alpha );
#endif
