#include <gccore.h>
#include "externals.h"
#include "gxsupp.h"

Mtx GXmodelViewIdent;

void PEOPS_GX_Flush()
{
	GX_CopyDisp (xfb[whichfb], GX_TRUE);
	GX_DrawDone();
	VIDEO_SetNextFramebuffer(xfb[whichfb]);
	VIDEO_Flush();
	whichfb ^= 1;
}
