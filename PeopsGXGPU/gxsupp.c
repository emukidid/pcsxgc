#include <gccore.h>
#include "externals.h"
#include "gxsupp.h"

void PEOPS_GX_PreRetraceCallback(u32 retraceCnt)
{
	if(iDrawnSomething)
	{
		VIDEO_SetNextFramebuffer(xfb[whichfb]);
		VIDEO_Flush();
		whichfb ^= 1;
		iDrawnSomething = false;
	}
}
