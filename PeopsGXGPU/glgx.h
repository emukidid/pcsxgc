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

typedef struct _alphacomp {
	u8 func;
	u8 ref;
	u8 enabled;
} alphacomp;

typedef struct _blendfunc {
	u8 srcfact;
	u8 destfact;
	u8 enabled;
} blendfunc;

enum _gxtypes {
	GX_SCISSOR_TEST=0,
	GX_ALPHA_TEST,
	GX_DEPTH_TEST,
	GX_TEXTURE_2D
};

void gxScissor(u32 x,u32 y,u32 width,u32 height);
void gxViewport(f32 xOrig,f32 yOrig,f32 wd,f32 ht,f32 nearZ,f32 farZ);
void gxAlphaFunc(u8 func, u8 ref);
void gxBlendFunc(u8 srcfact, u8 destfact);
void gxEnable(u32 type);
void gxDisable(u32 type);
void gxFlush();
void gxSwapBuffers();
void gxClear (GLbitfield mask);
void gxClearColor(	GLclampf red,
					GLclampf green,
					GLclampf blue,
					GLclampf alpha );
#endif
