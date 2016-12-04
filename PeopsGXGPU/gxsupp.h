#ifndef _GXSUPP_H
#define _GXSUPP_H

#include <gccore.h>

extern u32* xfb[2];	/*** Framebuffers ***/
extern int whichfb;        /*** Frame buffer toggle ***/
extern GXRModeObj *vmode;				/*** Graphics Mode Object ***/

extern void PEOPS_GX_PreRetraceCallback(u32 retraceCnt);
#endif // _GPU_INTERNALS_H
