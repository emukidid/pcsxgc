/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Unai                                               *
*   Copyright (C) 2016 Senquack (dansilsby <AT> gmail <DOT> com)          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
***************************************************************************/

#include <stddef.h>
#include "../plugins.h"
#include "../psxcommon.h"
//#include "port.h"
#include "gpu_unai.h"

#define VIDEO_WIDTH 320

#ifdef TIME_IN_MSEC
#define TPS 1000
#else
#define TPS 1000000
#endif

#define IS_PAL (gpu_unai.GPU_GP1&(0x08<<17))

//senquack - Original 512KB of guard space seems not to be enough, as Xenogears
// accesses outside this range and crashes in town intro fight sequence.
// Increased to 2MB total (double PSX VRAM) and Xenogears no longer
// crashes, but some textures are still messed up. Also note that alignment min
// is 16 bytes, needed for pixel-skipping rendering/blitting in high horiz res.
// Extra 4KB is for guard room at beginning.
// TODO: Determine cause of out-of-bounds write/reads. <-- Note: this is largely
//  solved by adoption of PCSX Rearmed's 'gpulib' in gpulib_if.cpp, which
//  replaces this file (gpu.cpp)
//u16   GPU_FrameBuffer[(FRAME_BUFFER_SIZE+512*1024)/2] __attribute__((aligned(32)));
static u16 GPU_FrameBuffer[(FRAME_BUFFER_SIZE*2 + 4096)/2] __attribute__((aligned(32)));

///////////////////////////////////////////////////////////////////////////////
// GPU fixed point math
#include "gpu_fixedpoint.h"

///////////////////////////////////////////////////////////////////////////////
// Inner loop driver instantiation file
#include "gpu_inner.h"

///////////////////////////////////////////////////////////////////////////////
// GPU internal image drawing functions
#include "gpu_raster_image.h"

///////////////////////////////////////////////////////////////////////////////
// GPU internal line drawing functions
#include "gpu_raster_line.h"

///////////////////////////////////////////////////////////////////////////////
// GPU internal polygon drawing functions
#include "gpu_raster_polygon.h"

///////////////////////////////////////////////////////////////////////////////
// GPU internal sprite drawing functions
#include "gpu_raster_sprite.h"

///////////////////////////////////////////////////////////////////////////////
// GPU command buffer execution/store
#include "gpu_command.h"

#ifdef __GX__
#include <gccore.h>
#include <malloc.h>
#include <time.h>
#include "../Gamecube/libgui/IPLFontC.h"
#include "../Gamecube/DEBUG.h"
#include "../Gamecube/wiiSXconfig.h"
#include "ogc/lwp_watchdog.h"
static unsigned short	GXtexture[1024*512] __attribute__((aligned(32)));
unsigned short *SCREEN;
extern u32* xfb[2];	/*** Framebuffers ***/
extern int whichfb;        /*** Frame buffer toggle ***/
extern time_t tStart;
extern char text[DEBUG_TEXT_HEIGHT][DEBUG_TEXT_WIDTH]; /*** DEBUG textbuffer ***/
extern char menuActive;
extern char screenMode;

long UNAI_GPUopen(void **unused)
{
	SCREEN = &GXtexture[0];
  return 0;
}

long UNAI_GPUclose(void)
{
  return 0;
}

void video_flip(void)
{
	static int oldwidth=0;
	static int oldheight=0;
	static GXTexObj GXtexobj;

	int x = gpu_unai.DisplayArea[0];
	int y = gpu_unai.DisplayArea[1];
	int width = gpu_unai.DisplayArea[2];
	int height = gpu_unai.DisplayArea[3];

	printf("x %i y %i, width %i height %i\r\n", x, y, width, height);
	if((width == 0) || (height == 0))
		return;

	if ((oldwidth != width) || (oldheight != height))
	{ //adjust texture conversion
		oldwidth = width;
		oldheight = height;
		//memset(GXtexture,0,512*1024);
		GX_InitTexObj(&GXtexobj, GXtexture, width, height, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
	}
/*
	if (gpu.status & PSX_GPU_STATUS_RGB24) {
		// TODO this is a cop-out and is really doing RGB888->RGB555 in a separate pass, fixme.
		vf64 *wgPipePtr = GX_RedirectWriteGatherPipe(GXtexture);
		f64 *src1 = (f64 *) buffer;
		f64 *src2 = (f64 *) (buffer + pitch);
		f64 *src3 = (f64 *) (buffer + (pitch * 2));
		f64 *src4 = (f64 *) (buffer + (pitch * 3));
		int rowpitch = (pitch >> 3) * 4  - (width >> 2);
		for (h = 0; h < height; h += 4)
		{
			for (w = 0; w < (width >> 2); w++)
			{
				*wgPipePtr = *src1++;
				*wgPipePtr = *src2++;
				*wgPipePtr = *src3++;
				*wgPipePtr = *src4++;
			}

		  src1 += rowpitch;
		  src2 += rowpitch;
		  src3 += rowpitch;
		  src4 += rowpitch;
		}
	}
	else {
		vu32 *wgPipePtr = GX_RedirectWriteGatherPipe(GXtexture);
		u32 *src1 = (u32 *) buffer;
		u32 *src2 = (u32 *) (buffer + pitch);
		u32 *src3 = (u32 *) (buffer + (pitch * 2));
		u32 *src4 = (u32 *) (buffer + (pitch * 3));
		int rowpitch = (pitch >> 2) * 4  - (width >> 1);
		u32 alphaMask = 0x00800080;
		for (h = 0; h < height; h += 4)
		{
			for (w = 0; w < (width >> 2); w++)
			{
				// Convert from BGR555 LE data to BGR BE, we OR the first bit with "1" to send RGB5A3 mode into RGB555 on the GP (GC/Wii)
				__asm__ (
					"lwz 3, 0(%1)\n"
					"lwz 4, 4(%1)\n"
					"lwz 5, 0(%2)\n"
					"lwz 6, 4(%2)\n"
					
					"rlwinm 3, 3, 16, 0, 31\n"
					"rlwinm 4, 4, 16, 0, 31\n"
					"rlwinm 5, 5, 16, 0, 31\n"
					"rlwinm 6, 6, 16, 0, 31\n"
					
					"or 3, 3, %5\n"
					"or 4, 4, %5\n"
					"or 5, 5, %5\n"
					"or 6, 6, %5\n"				
					
					"stwbrx 3, 0, %0\n" 
					"stwbrx 4, 0, %0\n" 
					"stwbrx 5, 0, %0\n" 	
					"stwbrx 6, 0, %0\n" 
					
					"lwz 3, 0(%3)\n"
					"lwz 4, 4(%3)\n"
					"lwz 5, 0(%4)\n"
					"lwz 6, 4(%4)\n"
					
					"rlwinm 3, 3, 16, 0, 31\n"
					"rlwinm 4, 4, 16, 0, 31\n"
					"rlwinm 5, 5, 16, 0, 31\n"
					"rlwinm 6, 6, 16, 0, 31\n"
					
					"or 3, 3, %5\n"
					"or 4, 4, %5\n"
					"or 5, 5, %5\n"
					"or 6, 6, %5\n"
					
					"stwbrx 3, 0, %0\n" 
					"stwbrx 4, 0, %0\n" 					
					"stwbrx 5, 0, %0\n" 
					"stwbrx 6, 0, %0" 					
					: : "r" (wgPipePtr), "r" (src1), "r" (src2), "r" (src3), "r" (src4), "r" (alphaMask) : "memory", "r3", "r4", "r5", "r6");
					//         %0             %1            %2          %3          %4           %5     
				src1+=2;
				src2+=2;
				src3+=2;
				src4+=2;
			}

		  src1 += rowpitch;
		  src2 += rowpitch;
		  src3 += rowpitch;
		  src4 += rowpitch;
		}
	}
	GX_RestoreWriteGatherPipe();
*/
	GX_LoadTexObj(&GXtexobj, GX_TEXMAP0);

	Mtx44	GXprojIdent;
	Mtx		GXmodelIdent;
	guMtxIdentity(GXprojIdent);
	guMtxIdentity(GXmodelIdent);
	GXprojIdent[2][2] = 0.5;
	GXprojIdent[2][3] = -0.5;
	GX_LoadProjectionMtx(GXprojIdent, GX_ORTHOGRAPHIC);
	GX_LoadPosMtxImm(GXmodelIdent,GX_PNMTX0);

	GX_SetCullMode(GX_CULL_NONE);
	GX_SetZMode(GX_ENABLE,GX_ALWAYS,GX_TRUE);

	GX_InvalidateTexAll();
	GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);


	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_IDENTITY);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	GX_SetNumTexGens(1);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	float xcoord = 1.0;
	float ycoord = 1.0;
	if(screenMode == SCREENMODE_16x9_PILLARBOX) xcoord = 640.0/848.0;

	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
	  GX_Position2f32(-xcoord, ycoord);
	  GX_TexCoord2f32( 0.0, 0.0);
	  GX_Position2f32( xcoord, ycoord);
	  GX_TexCoord2f32( 1.0, 0.0);
	  GX_Position2f32( xcoord,-ycoord);
	  GX_TexCoord2f32( 1.0, 1.0);
	  GX_Position2f32(-xcoord,-ycoord);
	  GX_TexCoord2f32( 0.0, 1.0);
	GX_End();

	//Write menu/debug text on screen
	GXColor fontColor = {150,255,150,255};
	IplFont_drawInit(fontColor);
//	if(showFPSonScreen)
//		IplFont_drawString(10,35,frate, 1.0, false);

	int i = 0;
	DEBUG_update();
	for (i=0;i<DEBUG_TEXT_HEIGHT;i++)
		IplFont_drawString(10,(10*i+60),text[i], 0.5, false);
		
   //reset swap table from GUI/DEBUG
	GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_BLUE, GX_CH_GREEN, GX_CH_RED ,GX_CH_ALPHA);
	GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);

	GX_DrawDone();

	whichfb ^= 1;
	GX_CopyDisp(xfb[whichfb], GX_TRUE);
	GX_DrawDone();
//	printf("Prv.Rng.x0,x1,y0 = %d, %d, %d, Prv.Mode.y = %d,DispPos.x,y = %d, %d, RGB24 = %x\n",PreviousPSXDisplay.Range.x0,PreviousPSXDisplay.Range.x1,PreviousPSXDisplay.Range.y0,PreviousPSXDisplay.DisplayMode.y,PSXDisplay.DisplayPosition.x,PSXDisplay.DisplayPosition.y,PSXDisplay.RGB24);
	VIDEO_SetNextFramebuffer(xfb[whichfb]);
	VIDEO_Flush();
}

void video_clear(void)
{
	if (menuActive) return;

	whichfb ^= 1;
	GX_CopyDisp(xfb[whichfb], GX_TRUE);
	GX_DrawDone();
	VIDEO_SetNextFramebuffer(xfb[whichfb]);
	VIDEO_Flush();	
}

u32 get_ticks(void) {
	return gettick();
}
#endif


///////////////////////////////////////////////////////////////////////////////
static void gpuReset(void)
{
	memset((void*)&gpu_unai, 0, sizeof(gpu_unai));
	gpu_unai.vram = (u16*)GPU_FrameBuffer + (4096/2); //4kb guard room in front
	gpu_unai.GPU_GP1 = 0x14802000;
	gpu_unai.DrawingArea[2] = 256;
	gpu_unai.DrawingArea[3] = 240;
	gpu_unai.DisplayArea[2] = 256;
	gpu_unai.DisplayArea[3] = 240;
	gpu_unai.DisplayArea[5] = 240;
	gpu_unai.TextureWindow[0] = 0;
	gpu_unai.TextureWindow[1] = 0;
	gpu_unai.TextureWindow[2] = 255;
	gpu_unai.TextureWindow[3] = 255;
	//senquack - new vars must be updated whenever texture window is changed:
	//           (used for polygon-drawing in gpu_inner.h, gpu_raster_polygon.h)
	const u32 fb = FIXED_BITS;  // # of fractional fixed-pt bits of u4/v4
	gpu_unai.u_msk = (((u32)gpu_unai.TextureWindow[2]) << fb) | ((1 << fb) - 1);
	gpu_unai.v_msk = (((u32)gpu_unai.TextureWindow[3]) << fb) | ((1 << fb) - 1);

	// Configuration options
	gpu_unai.config = gpu_unai_config_ext;
	gpu_unai.ilace_mask = gpu_unai.config.ilace_force;
	gpu_unai.frameskip.skipCount = gpu_unai.config.frameskip_count;

	SetupLightLUT();
	SetupDitheringConstants();
}

///////////////////////////////////////////////////////////////////////////////
#ifndef __GX__
long GPU_init(void)
#else
long UNAI_GPUinit(void)
#endif
{
	gpuReset();

#ifdef GPU_UNAI_USE_INT_DIV_MULTINV
	// s_invTable
	for(unsigned int i=1;i<=(1<<TABLE_BITS);++i)
	{
		s_invTable[i-1]=0x7fffffff/i;
	}
#endif

	gpu_unai.fb_dirty = true;
	gpu_unai.dma.last_dma = NULL;
	return (0);
}

///////////////////////////////////////////////////////////////////////////////
#ifndef __GX__
long GPU_shutdown(void)
#else
long UNAI_GPUshutdown(void)
#endif
{
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
#ifndef __GX__
long GPU_freeze(u32 bWrite, GPUFreeze_t* p2)
#else
long UNAI_GPUfreeze(u32 bWrite, GPUFreeze_t* p2)
#endif
{
	if (!p2) return (0);
	if (p2->ulFreezeVersion != 1) return (0);

	if (bWrite)
	{
		p2->ulStatus = gpu_unai.GPU_GP1;
		memset(p2->ulControl, 0, sizeof(p2->ulControl));
		// save resolution and registers for P.E.Op.S. compatibility
		p2->ulControl[3] = (3 << 24) | ((gpu_unai.GPU_GP1 >> 23) & 1);
		p2->ulControl[4] = (4 << 24) | ((gpu_unai.GPU_GP1 >> 29) & 3);
		p2->ulControl[5] = (5 << 24) | (gpu_unai.DisplayArea[0] | (gpu_unai.DisplayArea[1] << 10));
		p2->ulControl[6] = (6 << 24) | (2560 << 12);
		p2->ulControl[7] = (7 << 24) | (gpu_unai.DisplayArea[4] | (gpu_unai.DisplayArea[5] << 10));
		p2->ulControl[8] = (8 << 24) | ((gpu_unai.GPU_GP1 >> 17) & 0x3f) | ((gpu_unai.GPU_GP1 >> 10) & 0x40);
		memcpy((void*)p2->psxVRam, (void*)gpu_unai.vram, FRAME_BUFFER_SIZE);
		return (1);
	}
	else
	{
		extern void UNAI_GPUwriteStatus(u32 data);
		gpu_unai.GPU_GP1 = p2->ulStatus;
		memcpy((void*)gpu_unai.vram, (void*)p2->psxVRam, FRAME_BUFFER_SIZE);
		UNAI_GPUwriteStatus((5 << 24) | p2->ulControl[5]);
		UNAI_GPUwriteStatus((7 << 24) | p2->ulControl[7]);
		UNAI_GPUwriteStatus((8 << 24) | p2->ulControl[8]);
		gpuSetTexture(gpu_unai.GPU_GP1);
		return (1);
	}
	return (0);
}

///////////////////////////////////////////////////////////////////////////////
//  GPU DMA communication

///////////////////////////////////////////////////////////////////////////////
u8 PacketSize[256] =
{
	0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		0-15
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		16-31
	3, 3, 3, 3, 6, 6, 6, 6, 4, 4, 4, 4, 8, 8, 8, 8,	//		32-47
	5, 5, 5, 5, 8, 8, 8, 8, 7, 7, 7, 7, 11, 11, 11, 11,	//	48-63
	2, 2, 2, 2, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3,	//		64-79
	3, 3, 3, 3, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4,	//		80-95
	2, 2, 2, 2, 3, 3, 3, 3, 1, 1, 1, 1, 2, 2, 2, 2,	//		96-111
	1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2,	//		112-127
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		128-
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		144
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		160
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0	//
};

///////////////////////////////////////////////////////////////////////////////
INLINE void gpuSendPacket()
{
	gpuSendPacketFunction(gpu_unai.PacketBuffer.U4[0]>>24);
}

///////////////////////////////////////////////////////////////////////////////
INLINE void gpuCheckPacket(u32 uData)
{
	if (gpu_unai.PacketCount)
	{
		gpu_unai.PacketBuffer.U4[gpu_unai.PacketIndex++] = uData;
		--gpu_unai.PacketCount;
	}
	else
	{
		gpu_unai.PacketBuffer.U4[0] = uData;
		gpu_unai.PacketCount = PacketSize[uData >> 24];
		gpu_unai.PacketIndex = 1;
	}
	if (!gpu_unai.PacketCount) gpuSendPacket();
}

///////////////////////////////////////////////////////////////////////////////
#ifndef __GX__
void GPU_writeDataMem(u32* dmaAddress, int dmaCount)
#else
void UNAI_GPUwriteDataMem(u32* dmaAddress, int dmaCount)
#endif
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"GPU_writeDataMem(%d)\n",dmaCount);
	#endif
	u32 data;
	const u16 *VIDEO_END = (u16*)gpu_unai.vram+(FRAME_BUFFER_SIZE/2)-1;
	gpu_unai.GPU_GP1 &= ~0x14000000;

	while (dmaCount) 
	{
		if (gpu_unai.dma.FrameToWrite)
		{
			while (dmaCount)
			{
				dmaCount--;
				data = *dmaAddress++;
				if ((&gpu_unai.dma.pvram[gpu_unai.dma.px])>(VIDEO_END)) gpu_unai.dma.pvram-=512*1024;
				gpu_unai.dma.pvram[gpu_unai.dma.px] = data;
				if (++gpu_unai.dma.px >= gpu_unai.dma.x_end)
				{
					gpu_unai.dma.px = 0;
					gpu_unai.dma.pvram += 1024;
					if (++gpu_unai.dma.py >= gpu_unai.dma.y_end)
					{
						gpu_unai.dma.FrameToWrite = false;
						gpu_unai.GPU_GP1 &= ~0x08000000;
						gpu_unai.fb_dirty = true;
						break;
					}
				}
				if ((&gpu_unai.dma.pvram[gpu_unai.dma.px])>(VIDEO_END)) gpu_unai.dma.pvram-=512*1024;
				gpu_unai.dma.pvram[gpu_unai.dma.px] = data>>16;
				if (++gpu_unai.dma.px >= gpu_unai.dma.x_end)
				{
					gpu_unai.dma.px = 0;
					gpu_unai.dma.pvram += 1024;
					if (++gpu_unai.dma.py >= gpu_unai.dma.y_end)
					{
						gpu_unai.dma.FrameToWrite = false;
						gpu_unai.GPU_GP1 &= ~0x08000000;
						gpu_unai.fb_dirty = true;
						break;
					}
				}
			}
		}
		else
		{
			data = *dmaAddress++;
			dmaCount--;
			gpuCheckPacket(data);
		}
	}

	gpu_unai.GPU_GP1 = (gpu_unai.GPU_GP1 | 0x14000000) & ~0x60000000;
}

#ifndef __GX__
long GPU_dmaChain(u32 *rambase, u32 start_addr)
#else
long UNAI_GPUdmaChain(u32 *rambase, u32 start_addr)
#endif
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"GPU_dmaChain(0x%x)\n",start_addr);
	#endif

	u32 addr, *list;
	u32 len, count;
	long dma_words = 0;

	if (gpu_unai.dma.last_dma) *gpu_unai.dma.last_dma |= 0x800000;
	
	gpu_unai.GPU_GP1 &= ~0x14000000;
	
	addr = start_addr & 0xffffff;
	for (count = 0; addr != 0xffffff; count++)
	{
		list = rambase + (addr & 0x1fffff) / 4;
		len = list[0] >> 24;
		addr = list[0] & 0xffffff;

		dma_words += 1 + len;

		// add loop detection marker
		list[0] |= 0x800000;

		if (len) GPU_writeDataMem(list + 1, len);

		if (addr & 0x800000)
		{
			#ifdef ENABLE_GPU_LOG_SUPPORT
				fprintf(stdout,"GPU_dmaChain(LOOP)\n");
			#endif
			break;
		}
	}

	// remove loop detection markers
	addr = start_addr & 0x1fffff;
	while (count-- > 0)
	{
		list = rambase + addr / 4;
		addr = list[0] & 0x1fffff;
		list[0] &= ~0x800000;
	}
	
	if (gpu_unai.dma.last_dma) *gpu_unai.dma.last_dma &= ~0x800000;
	gpu_unai.dma.last_dma = rambase + (start_addr & 0x1fffff) / 4;

	gpu_unai.GPU_GP1 = (gpu_unai.GPU_GP1 | 0x14000000) & ~0x60000000;

	return dma_words;
}

///////////////////////////////////////////////////////////////////////////////
#ifndef __GX__
void GPU_writeData(u32 data)
#else
void UNAI_GPUwriteData(u32 data)
#endif
{
	const u16 *VIDEO_END = (u16*)gpu_unai.vram+(FRAME_BUFFER_SIZE/2)-1;
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"GPU_writeData()\n");
	#endif
	gpu_unai.GPU_GP1 &= ~0x14000000;

	if (gpu_unai.dma.FrameToWrite)
	{
		if ((&gpu_unai.dma.pvram[gpu_unai.dma.px])>(VIDEO_END)) gpu_unai.dma.pvram-=512*1024;
		gpu_unai.dma.pvram[gpu_unai.dma.px]=(u16)data;
		if (++gpu_unai.dma.px >= gpu_unai.dma.x_end)
		{
			gpu_unai.dma.px = 0;
			gpu_unai.dma.pvram += 1024;
			if (++gpu_unai.dma.py >= gpu_unai.dma.y_end)
			{
				gpu_unai.dma.FrameToWrite = false;
				gpu_unai.GPU_GP1 &= ~0x08000000;
				gpu_unai.fb_dirty = true;
			}
		}
		if (gpu_unai.dma.FrameToWrite)
		{
			if ((&gpu_unai.dma.pvram[gpu_unai.dma.px])>(VIDEO_END)) gpu_unai.dma.pvram-=512*1024;
			gpu_unai.dma.pvram[gpu_unai.dma.px]=data>>16;
			if (++gpu_unai.dma.px >= gpu_unai.dma.x_end)
			{
				gpu_unai.dma.px = 0;
				gpu_unai.dma.pvram += 1024;
				if (++gpu_unai.dma.py >= gpu_unai.dma.y_end)
				{
					gpu_unai.dma.FrameToWrite = false;
					gpu_unai.GPU_GP1 &= ~0x08000000;
					gpu_unai.fb_dirty = true;
				}
			}
		}
	}
	else
	{
		gpuCheckPacket(data);
	}
	gpu_unai.GPU_GP1 |= 0x14000000;
}


///////////////////////////////////////////////////////////////////////////////
#ifndef __GX__
void GPU_readDataMem(u32* dmaAddress, int dmaCount)
#else
void UNAI_GPUreadDataMem(u32* dmaAddress, int dmaCount)
#endif
{
	const u16 *VIDEO_END = (u16*)gpu_unai.vram+(FRAME_BUFFER_SIZE/2)-1;
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"GPU_readDataMem(%d)\n",dmaCount);
	#endif
	if(!gpu_unai.dma.FrameToRead) return;

	gpu_unai.GPU_GP1 &= ~0x14000000;
	do 
	{
		if ((&gpu_unai.dma.pvram[gpu_unai.dma.px])>(VIDEO_END)) gpu_unai.dma.pvram-=512*1024;
		// lower 16 bit
		//senquack - 64-bit fix (from notaz)
		//u32 data = (unsigned long)gpu_unai.dma.pvram[gpu_unai.dma.px];
		u32 data = (u32)gpu_unai.dma.pvram[gpu_unai.dma.px];

		if (++gpu_unai.dma.px >= gpu_unai.dma.x_end)
		{
			gpu_unai.dma.px = 0;
			gpu_unai.dma.pvram += 1024;
		}

		if ((&gpu_unai.dma.pvram[gpu_unai.dma.px])>(VIDEO_END)) gpu_unai.dma.pvram-=512*1024;
		// higher 16 bit (always, even if it's an odd width)
		//senquack - 64-bit fix (from notaz)
		//data |= (unsigned long)(gpu_unai.dma.pvram[gpu_unai.dma.px])<<16;
		data |= (u32)(gpu_unai.dma.pvram[gpu_unai.dma.px])<<16;
		
		*dmaAddress++ = data;

		if (++gpu_unai.dma.px >= gpu_unai.dma.x_end)
		{
			gpu_unai.dma.px = 0;
			gpu_unai.dma.pvram += 1024;
			if (++gpu_unai.dma.py >= gpu_unai.dma.y_end)
			{
				gpu_unai.dma.FrameToRead = false;
				gpu_unai.GPU_GP1 &= ~0x08000000;
				break;
			}
		}
	} while (--dmaCount);

	gpu_unai.GPU_GP1 = (gpu_unai.GPU_GP1 | 0x14000000) & ~0x60000000;
}



///////////////////////////////////////////////////////////////////////////////
#ifndef __GX__
u32 GPU_readData(void)
#else
u32 UNAI_GPUreadData(void)
#endif
{
	const u16 *VIDEO_END = (u16*)gpu_unai.vram+(FRAME_BUFFER_SIZE/2)-1;
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"GPU_readData()\n");
	#endif
	gpu_unai.GPU_GP1 &= ~0x14000000;
	if (gpu_unai.dma.FrameToRead)
	{
		if ((&gpu_unai.dma.pvram[gpu_unai.dma.px])>(VIDEO_END)) gpu_unai.dma.pvram-=512*1024;
		gpu_unai.GPU_GP0 = gpu_unai.dma.pvram[gpu_unai.dma.px];
		if (++gpu_unai.dma.px >= gpu_unai.dma.x_end)
		{
			gpu_unai.dma.px = 0;
			gpu_unai.dma.pvram += 1024;
			if (++gpu_unai.dma.py >= gpu_unai.dma.y_end)
			{
				gpu_unai.dma.FrameToRead = false;
				gpu_unai.GPU_GP1 &= ~0x08000000;
			}
		}
		if ((&gpu_unai.dma.pvram[gpu_unai.dma.px])>(VIDEO_END)) gpu_unai.dma.pvram-=512*1024;
		gpu_unai.GPU_GP0 |= gpu_unai.dma.pvram[gpu_unai.dma.px]<<16;
		if (++gpu_unai.dma.px >= gpu_unai.dma.x_end)
		{
			gpu_unai.dma.px = 0;
			gpu_unai.dma.pvram += 1024;
			if (++gpu_unai.dma.py >= gpu_unai.dma.y_end)
			{
				gpu_unai.dma.FrameToRead = false;
				gpu_unai.GPU_GP1 &= ~0x08000000;
			}
		}

	}
	gpu_unai.GPU_GP1 |= 0x14000000;

	return (gpu_unai.GPU_GP0);
}

///////////////////////////////////////////////////////////////////////////////
#ifndef __GX__
u32 GPU_readStatus(void)
#else
u32 UNAI_GPUreadStatus(void)
#endif
{
	return gpu_unai.GPU_GP1;
}

INLINE void GPU_NoSkip(void)
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"GPU_NoSkip()\n");
	#endif
	gpu_unai.frameskip.wasSkip = gpu_unai.frameskip.isSkip;
	if (gpu_unai.frameskip.isSkip)
	{
		gpu_unai.frameskip.isSkip = false;
		gpu_unai.frameskip.skipGPU = false;
	}
	else
	{
		gpu_unai.frameskip.isSkip = gpu_unai.frameskip.skipFrame;
		gpu_unai.frameskip.skipGPU = gpu_unai.frameskip.skipFrame;
	}
}

///////////////////////////////////////////////////////////////////////////////
#ifndef __GX__
void GPU_writeStatus(u32 data)
#else
void UNAI_GPUwriteStatus(u32 data)
#endif
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"GPU_writeStatus(%d,%d)\n",data>>24,data & 0xff);
	#endif
	switch (data >> 24) {
	case 0x00:
		gpuReset();
		break;
	case 0x01:
		gpu_unai.GPU_GP1 &= ~0x08000000;
		gpu_unai.PacketCount = 0;
		gpu_unai.dma.FrameToRead = gpu_unai.dma.FrameToWrite = false;
		break;
	case 0x02:
		gpu_unai.GPU_GP1 &= ~0x08000000;
		gpu_unai.PacketCount = 0;
		gpu_unai.dma.FrameToRead = gpu_unai.dma.FrameToWrite = false;
		break;
	case 0x03:
		gpu_unai.GPU_GP1 = (gpu_unai.GPU_GP1 & ~0x00800000) | ((data & 1) << 23);
		break;
	case 0x04:
		if (data == 0x04000000)	gpu_unai.PacketCount = 0;
		gpu_unai.GPU_GP1 = (gpu_unai.GPU_GP1 & ~0x60000000) | ((data & 3) << 29);
		break;
	case 0x05:
		// Start of Display Area in VRAM
		gpu_unai.DisplayArea[0] = data & 0x3ff;         // X (0..1023)
		gpu_unai.DisplayArea[1] = (data >> 10) & 0x1ff; // Y (0..511)
		GPU_NoSkip();
		break;
	case 0x06:
		// GP1(06h) - Horizontal Display range (on Screen)
		// 0-11   X1 (260h+0)       ;12bit       ;\counted in 53.222400MHz units,
		// 12-23  X2 (260h+320*8)   ;12bit       ;/relative to HSYNC

		// senquack - gpu_unai completely ignores GP1(0x06) command and
		// lacks even a place in DisplayArea[] array to store the values.
		// It seems to have been concerned only with vertical display range
		// and centering top/bottom. I will not add support here, and
		// focus instead on the gpulib version (gpulib_if.cpp) which uses
		// gpulib for its PS1->host framebuffer blitting.
		break;
	case 0x07:
		// GP1(07h) - Vertical Display range (on Screen)
		// 0-9   Y1 (NTSC=88h-(224/2), (PAL=A3h-(264/2))  ;\scanline numbers on screen,
		// 10-19 Y2 (NTSC=88h+(224/2), (PAL=A3h+(264/2))  ;/relative to VSYNC
		// 20-23 Not used (zero)
		{
			u32 v1=data & 0x000003FF; //(short)(data & 0x3ff);
			u32 v2=(data & 0x000FFC00) >> 10; //(short)((data>>10) & 0x3ff);
			if ((gpu_unai.DisplayArea[4]!=v1)||(gpu_unai.DisplayArea[5]!=v2))
			{
				gpu_unai.DisplayArea[4] = v1;
				gpu_unai.DisplayArea[5] = v2;
				#ifdef ENABLE_GPU_LOG_SUPPORT
					fprintf(stdout,"video_clear(CHANGE_Y)\n");
				#endif
				video_clear();
			}
		}
		break;
	case 0x08:
		{
			static const u32 HorizontalResolution[8] = { 256, 368, 320, 384, 512, 512, 640, 640 };
			static const u32 VerticalResolution[4] = { 240, 480, 256, 480 };
			gpu_unai.GPU_GP1 = (gpu_unai.GPU_GP1 & ~0x007F0000) | ((data & 0x3F) << 17) | ((data & 0x40) << 10);
			#ifdef ENABLE_GPU_LOG_SUPPORT
				fprintf(stdout,"GPU_writeStatus(RES=%dx%d,BITS=%d,PAL=%d)\n",HorizontalResolution[(gpu_unai.GPU_GP1 >> 16) & 7],
						VerticalResolution[(gpu_unai.GPU_GP1 >> 19) & 3],(gpu_unai.GPU_GP1&0x00200000?24:15),(IS_PAL?1:0));
			#endif
			// Video mode change
			u32 new_width = HorizontalResolution[(gpu_unai.GPU_GP1 >> 16) & 7];
			u32 new_height = VerticalResolution[(gpu_unai.GPU_GP1 >> 19) & 3];

			if (gpu_unai.DisplayArea[2] != new_width || gpu_unai.DisplayArea[3] != new_height)
			{
				// Update width
				gpu_unai.DisplayArea[2] = new_width;

				if (PixelSkipEnabled()) {
					// Set blit_mask for high horizontal resolutions. This allows skipping
					//  rendering pixels that would never get displayed on low-resolution
					//  platforms that use simple pixel-dropping scaler.
					switch (gpu_unai.DisplayArea[2])
					{
						case 512: gpu_unai.blit_mask = 0xa4; break; // GPU_BlitWWSWWSWS
						case 640: gpu_unai.blit_mask = 0xaa; break; // GPU_BlitWS
						default:  gpu_unai.blit_mask = 0;    break;
					}
				} else {
					gpu_unai.blit_mask = 0;
				}

				// Update height
				gpu_unai.DisplayArea[3] = new_height;

				if (LineSkipEnabled()) {
					// Set rendering line-skip (only render every other line in high-res
					//  480 vertical mode, or, optionally, force it for all video modes)

					if (gpu_unai.DisplayArea[3] == 480) {
						if (gpu_unai.config.ilace_force) {
							gpu_unai.ilace_mask = 3; // Only need 1/4 of lines
						} else {
							gpu_unai.ilace_mask = 1; // Only need 1/2 of lines
						}
					} else {
						// Vert resolution changed from 480 to lower one
						gpu_unai.ilace_mask = gpu_unai.config.ilace_force;
					}
				} else {
					gpu_unai.ilace_mask = 0;
				}

				#ifdef ENABLE_GPU_LOG_SUPPORT
					fprintf(stdout,"video_clear(CHANGE_RES)\n");
				#endif
				video_clear();
			}

		}
		break;
	case 0x10:
		switch (data & 0xff) {
			case 2: gpu_unai.GPU_GP0 = gpu_unai.tex_window; break;
			case 3: gpu_unai.GPU_GP0 = (gpu_unai.DrawingArea[1] << 10) | gpu_unai.DrawingArea[0]; break;
			case 4: gpu_unai.GPU_GP0 = ((gpu_unai.DrawingArea[3]-1) << 10) | (gpu_unai.DrawingArea[2]-1); break;
			case 5: case 6:	gpu_unai.GPU_GP0 = (((u32)gpu_unai.DrawingOffset[1] & 0x7ff) << 11) | ((u32)gpu_unai.DrawingOffset[0] & 0x7ff); break;
			case 7: gpu_unai.GPU_GP0 = 2; break;
			case 8: case 15: gpu_unai.GPU_GP0 = 0xBFC03720; break;
		}
		break;
	}
}

// Blitting functions
#include "gpu_blit.h"

static void gpuVideoOutput(void)
{
	int h0, x0, y0, w0, h1;

	x0 = gpu_unai.DisplayArea[0];
	y0 = gpu_unai.DisplayArea[1];

	w0 = gpu_unai.DisplayArea[2];
	h0 = gpu_unai.DisplayArea[3];  // video mode

	h1 = gpu_unai.DisplayArea[5] - gpu_unai.DisplayArea[4]; // display needed
	if (h0 == 480) h1 = Min2(h1*2,480);

	bool isRGB24 = (gpu_unai.GPU_GP1 & 0x00200000 ? true : false);
	u16* dst16 = SCREEN;
	u16* src16 = (u16*)gpu_unai.vram;

	// PS1 fb read wraps around (fixes black screen in 'Tobal no. 1')
	unsigned int src16_offs_msk = 1024*512-1;
	unsigned int src16_offs = (x0 + y0*1024) & src16_offs_msk;

	//  Height centering
	int sizeShift = 1;
	if (h0 == 256) {
		h0 = 240;
	} else if (h0 == 480) {
		sizeShift = 2;
	}
	if (h1 > h0) {
		src16_offs = (src16_offs + (((h1-h0) / 2) * 1024)) & src16_offs_msk;
		h1 = h0;
	} else if (h1<h0) {
		dst16 += ((h0-h1) >> sizeShift) * VIDEO_WIDTH;
	}


	/* Main blitter */
	int incY = (h0==480) ? 2 : 1;
	h0=(h0==480 ? 2048 : 1024);

	{
		const int li=gpu_unai.ilace_mask;
		bool pi = ProgressiveInterlaceEnabled();
		bool pif = gpu_unai.prog_ilace_flag;
		switch ( w0 )
		{
			case 256:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if (( 0 == (y0&li) ) && ((!pi) || (pif=!pif)))
						GPU_BlitWWDWW(src16 + src16_offs, dst16, isRGB24);
					dst16 += VIDEO_WIDTH;
					src16_offs = (src16_offs + h0) & src16_offs_msk;
				}
				break;
			case 368:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if (( 0 == (y0&li) ) && ((!pi) || (pif=!pif)))
						GPU_BlitWWWWWWWWS(src16 + src16_offs, dst16, isRGB24, 4);
					dst16 += VIDEO_WIDTH;
					src16_offs = (src16_offs + h0) & src16_offs_msk;
				}
				break;
			case 320:
				// Ensure 32-bit alignment for GPU_BlitWW() blitter:
				src16_offs &= ~1;
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if (( 0 == (y0&li) ) && ((!pi) || (pif=!pif)))
						GPU_BlitWW(src16 + src16_offs, dst16, isRGB24);
					dst16 += VIDEO_WIDTH;
					src16_offs = (src16_offs + h0) & src16_offs_msk;
				}
				break;
			case 384:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if (( 0 == (y0&li) ) && ((!pi) || (pif=!pif)))
						GPU_BlitWWWWWS(src16 + src16_offs, dst16, isRGB24);
					dst16 += VIDEO_WIDTH;
					src16_offs = (src16_offs + h0) & src16_offs_msk;
				}
				break;
			case 512:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if (( 0 == (y0&li) ) && ((!pi) || (pif=!pif)))
						GPU_BlitWWSWWSWS(src16 + src16_offs, dst16, isRGB24);
					dst16 += VIDEO_WIDTH;
					src16_offs = (src16_offs + h0) & src16_offs_msk;
				}
				break;
			case 640:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if (( 0 == (y0&li) ) && ((!pi) || (pif=!pif)))
						GPU_BlitWS(src16 + src16_offs, dst16, isRGB24);
					dst16 += VIDEO_WIDTH;
					src16_offs = (src16_offs + h0) & src16_offs_msk;
				}
				break;
		}
		gpu_unai.prog_ilace_flag = !gpu_unai.prog_ilace_flag;
	}
	video_flip();
}

// Update frames-skip each second>>3 (8 times per second)
#define GPU_FRAMESKIP_UPDATE 3

static void GPU_frameskip (bool show)
{
	u32 now=get_ticks(); // current frame

	// Update frameskip
	if (gpu_unai.frameskip.skipCount==0) gpu_unai.frameskip.skipFrame=false; // frameskip off
	else if (gpu_unai.frameskip.skipCount==7) { if (show) gpu_unai.frameskip.skipFrame=!gpu_unai.frameskip.skipFrame; } // frameskip medium
	else if (gpu_unai.frameskip.skipCount==8) gpu_unai.frameskip.skipFrame=true; // frameskip maximum
	else
	{
		static u32 spd=100; // speed %
		static u32 frames=0; // frames counter
		static u32 prev=now; // previous fps calculation
		frames++;
		if ((now-prev)>=(TPS>>GPU_FRAMESKIP_UPDATE))
		{
			if (IS_PAL) spd=(frames<<1);
			else spd=((frames*1001)/600);
			spd<<=GPU_FRAMESKIP_UPDATE;
			frames=0;
			prev=now;
		}
		switch(gpu_unai.frameskip.skipCount)
		{
			case 1: if (spd<50) gpu_unai.frameskip.skipFrame=true; else gpu_unai.frameskip.skipFrame=false; break; // frameskip on (spd<50%)
			case 2: if (spd<60) gpu_unai.frameskip.skipFrame=true; else gpu_unai.frameskip.skipFrame=false; break; // frameskip on (spd<60%)
			case 3: if (spd<70) gpu_unai.frameskip.skipFrame=true; else gpu_unai.frameskip.skipFrame=false; break; // frameskip on (spd<70%)
			case 4: if (spd<80) gpu_unai.frameskip.skipFrame=true; else gpu_unai.frameskip.skipFrame=false; break; // frameskip on (spd<80%)
			case 5: if (spd<90) gpu_unai.frameskip.skipFrame=true; else gpu_unai.frameskip.skipFrame=false; break; // frameskip on (spd<90%)
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
#ifndef __GX__
void GPU_updateLace(void)
#else
void UNAI_GPUupdateLace(void)
#endif
{
	// Interlace bit toggle
	gpu_unai.GPU_GP1 ^= 0x80000000;

	// Update display?
	if ((gpu_unai.fb_dirty) && (!gpu_unai.frameskip.wasSkip) && (!(gpu_unai.GPU_GP1&0x00800000)))
	{
		// Display updated
		gpuVideoOutput();
		GPU_frameskip(true);
		#ifdef ENABLE_GPU_LOG_SUPPORT
			fprintf(stdout,"GPU_updateLace(UPDATE)\n");
		#endif
	} else {
		GPU_frameskip(false);
		#ifdef ENABLE_GPU_LOG_SUPPORT
			fprintf(stdout,"GPU_updateLace(SKIP)\n");
		#endif
	}

	if ((!gpu_unai.frameskip.skipCount) && (gpu_unai.DisplayArea[3] == 480)) gpu_unai.frameskip.skipGPU=true; // Tekken 3 hack

	gpu_unai.fb_dirty=false;
	gpu_unai.dma.last_dma = NULL;
}

#ifndef __GX__
// Allows frontend to signal plugin to redraw screen after returning to emu
void GPU_requestScreenRedraw()
{
	gpu_unai.fb_dirty = true;
}

void GPU_getScreenInfo(GPUScreenInfo_t *sinfo)
{
	bool depth24 = (gpu_unai.GPU_GP1 & 0x00200000 ? true : false);
	int16_t hres = (uint16_t)gpu_unai.DisplayArea[2];
	int16_t vres = (uint16_t)gpu_unai.DisplayArea[3];
	int16_t w = hres; // Original gpu_unai doesn't support width < 100%
	int16_t h = gpu_unai.DisplayArea[5] - gpu_unai.DisplayArea[4];
	if (vres == 480)
		h *= 2;
	if (h <= 0 || h > vres)
		h = vres;

	sinfo->vram    = (uint8_t*)gpu_unai.vram;
	sinfo->x       = (uint16_t)gpu_unai.DisplayArea[0];
	sinfo->y       = (uint16_t)gpu_unai.DisplayArea[1];
	sinfo->w       = w;
	sinfo->h       = h;
	sinfo->hres    = hres;
	sinfo->vres    = vres;
	sinfo->depth24 = depth24;
	sinfo->pal     = IS_PAL;
}
#endif
