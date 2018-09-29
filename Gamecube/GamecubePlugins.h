/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2002  Pcsx Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef GAMECUBE_PLUGINS_H
#define GAMECUBE_PLUGINS_H

#ifndef NULL
#define NULL ((void*)0)
#endif
#include "../Decode_XA.h"
#include "../PSEmu_Plugin_Defs.h"
#include "../plugins.h"

#define SYMS_PER_LIB 32
typedef struct {
	const char* lib;
	int   numSyms;
	struct {
		const char* sym;
		void* pntr;
	} syms[SYMS_PER_LIB];
} PluginTable;
#define NUM_PLUGINS 8

/* SPU NULL */
//typedef s32 (* SPUopen)(void);
void NULL_SPUwriteRegister(u32 reg, unsigned short val);
unsigned short NULL_SPUreadRegister(u32 reg);
unsigned short NULL_SPUreadDMA(void);
void NULL_SPUwriteDMA(unsigned short val);
void NULL_SPUwriteDMAMem(unsigned short * pusPSXMem,int iSize);
void NULL_SPUreadDMAMem(unsigned short * pusPSXMem,int iSize);
void NULL_SPUplayADPCMchannel(xa_decode_t *xap);
s32 NULL_SPUinit(void);
s32 NULL_SPUopen(void);
void NULL_SPUsetConfigFile(char * pCfg);
s32 NULL_SPUclose(void);
s32 NULL_SPUshutdown(void);
s32 NULL_SPUtest(void);
void NULL_SPUregisterCallback(void (*callback)(void));
void NULL_SPUregisterCDDAVolume(void (*CDDAVcallback)(unsigned short,unsigned short));
char * NULL_SPUgetLibInfos(void);
void NULL_SPUabout(void);
s32 NULL_SPUfreeze(u32 ulFreezeMode,SPUFreeze_t *);

/* SPU PEOPS 1.9 */
//dma.c
unsigned short CALLBACK PEOPS_SPUreadDMA(void);
void CALLBACK PEOPS_SPUreadDMAMem(unsigned short * pusPSXMem,int iSize);
void CALLBACK PEOPS_SPUwriteDMA(unsigned short val);
void CALLBACK PEOPS_SPUwriteDMAMem(unsigned short * pusPSXMem,int iSize);
//PEOPSspu.c
void CALLBACK PEOPS_SPUasync(u32 cycle);
void CALLBACK PEOPS_SPUupdate(void);
void CALLBACK PEOPS_SPUplayADPCMchannel(xa_decode_t *xap);
s32 CALLBACK PEOPS_SPUinit(void);
s32 PEOPS_SPUopen(void);
void PEOPS_SPUsetConfigFile(char * pCfg);
s32 CALLBACK PEOPS_SPUclose(void);
s32 CALLBACK PEOPS_SPUshutdown(void);
s32 CALLBACK PEOPS_SPUtest(void);
s32 CALLBACK PEOPS_SPUconfigure(void);
void CALLBACK PEOPS_SPUabout(void);
void CALLBACK PEOPS_SPUregisterCallback(void (CALLBACK *callback)(void));
void CALLBACK PEOPS_SPUregisterCDDAVolume(void (CALLBACK *CDDAVcallback)(unsigned short,unsigned short));
//registers.c
void CALLBACK PEOPS_SPUwriteRegister(u32 reg, unsigned short val);
unsigned short CALLBACK PEOPS_SPUreadRegister(u32 reg);
//freeze.c
s32 CALLBACK PEOPS_SPUfreeze(u32 ulFreezeMode,SPUFreeze_t * pF);

/* franspu */
//spu_registers.cpp
void FRAN_SPU_writeRegister(u32 reg, unsigned short val);
unsigned short FRAN_SPU_readRegister(u32 reg);
//spu_dma.cpp
unsigned short FRAN_SPU_readDMA(void);
void FRAN_SPU_readDMAMem(unsigned short * pusPSXMem,int iSize);
void FRAN_SPU_writeDMA(unsigned short val);
void FRAN_SPU_writeDMAMem(unsigned short * pusPSXMem,int iSize);
//spu.cpp
void FRAN_SPU_async(u32 cycle);
void FRAN_SPU_playADPCMchannel(xa_decode_t *xap);
s32 FRAN_SPU_init(void);
s32 FRAN_SPU_open(void);
s32 FRAN_SPU_close(void);
s32 FRAN_SPU_shutdown(void);
s32 FRAN_SPU_freeze(u32 ulFreezeMode,SPUFreeze_t * pF);
void FRAN_SPU_setConfigFile(char *cfgfile);
void FRAN_SPU_About();
void FRAN_SPU_test();
void FRAN_SPU_registerCallback(void (*callback)(void));
void FRAN_SPU_registerCDDAVolume(void (*CDDAVcallback)(unsigned short,unsigned short));

/* CDR */
s32 CDR__open(void);
s32 CDR__init(void);
s32 CDR__shutdown(void);
s32 CDR__open(void);
s32 CDR__close(void);
s32 CDR__getTN(unsigned char *);
s32 CDR__getTD(unsigned char , unsigned char *);
s32 CDR__readTrack(unsigned char *);
s32 CDR__play(unsigned char *sector);
s32 CDR__stop(void);
s32 CDR__getStatus(struct CdrStat *stat);
unsigned char *CDR__getBuffer(void);
unsigned char *CDR__getBufferSub(void);

/* NULL GPU */
//typedef s32 (* GPUopen)(u32 *, char *, char *);
s32 GPU__open(void);  
s32 GPU__init(void);
s32 GPU__shutdown(void);
s32 GPU__close(void);
void GPU__writeStatus(u32);
void GPU__writeData(u32);
u32 GPU__readStatus(void);
u32 GPU__readData(void);
s32 GPU__dmaChain(u32 *,u32);
void GPU__updateLace(void);

/* PEOPS GPU */
s32 PEOPS_GPUopen(u32 *, char *, char *); 
s32 PEOPS_GPUinit(void);
s32 PEOPS_GPUshutdown(void);
s32 PEOPS_GPUclose(void);
void PEOPS_GPUwriteStatus(u32);
void PEOPS_GPUwriteData(u32);
void PEOPS_GPUwriteDataMem(u32 *, int);
u32 PEOPS_GPUreadStatus(void);
u32 PEOPS_GPUreadData(void);
void PEOPS_GPUreadDataMem(u32 *, int);
s32 PEOPS_GPUdmaChain(u32 *,u32);
void PEOPS_GPUupdateLace(void);
void PEOPS_GPUdisplayText(char *);
s32 PEOPS_GPUfreeze(u32,GPUFreeze_t *);

/* PAD */
//typedef s32 (* PADopen)(u32 *);
extern s32 PAD__init(s32);
extern s32 PAD__shutdown(void);	

/* WiiSX PAD Plugin */
extern s32 PAD__open(void);
extern s32 PAD__close(void);
unsigned char PAD__startPoll (int pad);
unsigned char PAD__poll (const unsigned char value);
s32 PAD__readPort1(PadDataS*);
s32 PAD__readPort2(PadDataS*);

/* SSSPSX PAD Plugin */
s32 SSS_PADopen (void *p);
s32 SSS_PADclose (void);
unsigned char SSS_PADstartPoll (int pad);
unsigned char SSS_PADpoll (const unsigned char value);
s32 SSS_PADreadPort1 (PadDataS* pads);
s32 SSS_PADreadPort2 (PadDataS* pads);

/* CDR ISO Plugin */
s32 CALLBACK ISOinit(void);
s32 CALLBACK ISOshutdown(void);
s32 CALLBACK ISOopen(void);
s32 CALLBACK ISOclose(void);
s32 CALLBACK ISOgetTN(unsigned char *buffer);
s32 CALLBACK ISOgetTD(unsigned char track, unsigned char *buffer);
s32 CALLBACK ISOreadTrack(unsigned char *time);
unsigned char * CALLBACK ISOgetBuffer(void);
s32 CALLBACK ISOplay(unsigned char *time);
s32 CALLBACK ISOstop(void);
unsigned char* CALLBACK ISOgetBufferSub(void);
s32 CALLBACK ISOgetStatus(struct CdrStat *stat);

#define EMPTY_PLUGIN \
	{ NULL,      \
	  0,         \
	  { { NULL,  \
	      NULL }, } }
	      
#define PAD1_PLUGIN \
	{ "PAD1",      \
	  7,         \
	  { { "PADinit",  \
	      (void*)PAD__init }, \
	    { "PADshutdown",	\
	      (void*)PAD__shutdown}, \
	    { "PADopen", \
	      (void*)PAD__open}, \
	    { "PADclose", \
	      (void*)PAD__close}, \
	    { "PADpoll", \
	      (void*)PAD__poll}, \
	    { "PADstartPoll", \
	      (void*)PAD__startPoll}, \
	    { "PADreadPort1", \
	      (void*)PAD__readPort1} \
	       } }
	    
#define PAD2_PLUGIN \
	{ "PAD2",      \
	  7,         \
	  { { "PADinit",  \
	      (void*)PAD__init }, \
	    { "PADshutdown",	\
	      (void*)PAD__shutdown}, \
	    { "PADopen", \
	      (void*)PAD__open}, \
	    { "PADclose", \
	      (void*)PAD__close}, \
	    { "PADpoll", \
	      (void*)PAD__poll}, \
	    { "PADstartPoll", \
	      (void*)PAD__startPoll}, \
	    { "PADreadPort2", \
	      (void*)PAD__readPort2} \
	       } }

#define SSS_PAD1_PLUGIN \
	{ "PAD1",      \
	  7,         \
	  { { "PADinit",  \
	      (void*)PAD__init }, \
	    { "PADshutdown",	\
	      (void*)PAD__shutdown}, \
	    { "PADopen", \
	      (void*)SSS_PADopen}, \
	    { "PADclose", \
	      (void*)SSS_PADclose}, \
	    { "PADpoll", \
	      (void*)SSS_PADpoll}, \
	    { "PADstartPoll", \
	      (void*)SSS_PADstartPoll}, \
	    { "PADreadPort1", \
	      (void*)SSS_PADreadPort1} \
	       } }
	    
#define SSS_PAD2_PLUGIN \
	{ "PAD2",      \
	  7,         \
	  { { "PADinit",  \
	      (void*)PAD__init }, \
	    { "PADshutdown",	\
	      (void*)PAD__shutdown}, \
	    { "PADopen", \
	      (void*)SSS_PADopen}, \
	    { "PADclose", \
	      (void*)SSS_PADclose}, \
	    { "PADpoll", \
	      (void*)SSS_PADpoll}, \
	    { "PADstartPoll", \
	      (void*)SSS_PADstartPoll}, \
	    { "PADreadPort2", \
	      (void*)SSS_PADreadPort2} \
	       } }

#define MOOBY28_CDR_PLUGIN \
	{ "CDR",      \
	  12,         \
	  { { "CDRinit",  \
	      (void*)Mooby2CDRinit }, \
	    { "CDRshutdown",	\
	      (void*)Mooby2CDRshutdown}, \
	    { "CDRopen", \
	      (void*)Mooby2CDRopen}, \
	    { "CDRclose", \
	      (void*)Mooby2CDRclose}, \
	    { "CDRgetTN", \
	      (void*)Mooby2CDRgetTN}, \
	    { "CDRgetTD", \
	      (void*)Mooby2CDRgetTD}, \
	    { "CDRreadTrack", \
	      (void*)Mooby2CDRreadTrack}, \
	    { "CDRgetBuffer", \
	      (void*)Mooby2CDRgetBuffer}, \
	    { "CDRplay", \
	      (void*)Mooby2CDRplay}, \
	    { "CDRstop", \
	      (void*)Mooby2CDRstop}, \
	    { "CDRgetStatus", \
	      (void*)Mooby2CDRgetStatus}, \
	    { "CDRgetBufferSub", \
	      (void*)Mooby2CDRgetBufferSub} \
	       } }
	       
#define CDRISO_PLUGIN \
	{ "CDR",      \
	  12,         \
	  { { "CDRinit",  \
	      (void*)ISOinit }, \
	    { "CDRshutdown",	\
	      (void*)ISOshutdown}, \
	    { "CDRopen", \
	      (void*)ISOopen}, \
	    { "CDRclose", \
	      (void*)ISOclose}, \
	    { "CDRgetTN", \
	      (void*)ISOgetTN}, \
	    { "CDRgetTD", \
	      (void*)ISOgetTD}, \
	    { "CDRreadTrack", \
	      (void*)ISOreadTrack}, \
	    { "CDRgetBuffer", \
	      (void*)ISOgetBuffer}, \
	    { "CDRplay", \
	      (void*)ISOplay}, \
	    { "CDRstop", \
	      (void*)ISOstop}, \
	    { "CDRgetStatus", \
	      (void*)ISOgetStatus}, \
	    { "CDRgetBufferSub", \
	      (void*)ISOgetBufferSub} \
	       } }
	       
#define CDR_PLUGIN \
	{ "CDR",      \
	  12,         \
	  { { "CDRinit",  \
	      (void*)CDR__init }, \
	    { "CDRshutdown",	\
	      (void*)CDR__shutdown}, \
	    { "CDRopen", \
	      (void*)CDR__open}, \
	    { "CDRclose", \
	      (void*)CDR__close}, \
	    { "CDRgetTN", \
	      (void*)CDR__getTN}, \
	    { "CDRgetTD", \
	      (void*)CDR__getTD}, \
	    { "CDRreadTrack", \
	      (void*)CDR__readTrack}, \
	    { "CDRgetBuffer", \
	      (void*)CDR__getBuffer}, \
	    { "CDRplay", \
	      (void*)CDR__play}, \
	    { "CDRstop", \
	      (void*)CDR__stop}, \
	    { "CDRgetStatus", \
	      (void*)CDR__getStatus}, \
	    { "CDRgetBufferSub", \
	      (void*)CDR__getBufferSub} \
	       } }

#define SPU_NULL_PLUGIN \
	{ "SPU",      \
	  17,         \
	  { { "SPUinit",  \
	      (void*)NULL_SPUinit }, \
	    { "SPUshutdown",	\
	      (void*)NULL_SPUshutdown}, \
	    { "SPUopen", \
	      (void*)NULL_SPUopen}, \
	    { "SPUclose", \
	      (void*)NULL_SPUclose}, \
	    { "SPUconfigure", \
	      (void*)NULL_SPUsetConfigFile}, \
	    { "SPUabout", \
	      (void*)NULL_SPUabout}, \
	    { "SPUtest", \
	      (void*)NULL_SPUtest}, \
	    { "SPUwriteRegister", \
	      (void*)NULL_SPUwriteRegister}, \
	    { "SPUreadRegister", \
	      (void*)NULL_SPUreadRegister}, \
	    { "SPUwriteDMA", \
	      (void*)NULL_SPUwriteDMA}, \
	    { "SPUreadDMA", \
	      (void*)NULL_SPUreadDMA}, \
	    { "SPUwriteDMAMem", \
	      (void*)NULL_SPUwriteDMAMem}, \
	    { "SPUreadDMAMem", \
	      (void*)NULL_SPUreadDMAMem}, \
	    { "SPUplayADPCMchannel", \
	      (void*)NULL_SPUplayADPCMchannel}, \
	    { "SPUfreeze", \
	      (void*)NULL_SPUfreeze}, \
	    { "SPUregisterCallback", \
	      (void*)NULL_SPUregisterCallback}, \
	    { "SPUregisterCDDAVolume", \
	      (void*)NULL_SPUregisterCDDAVolume} \
	       } }

#define SPU_PEOPS_PLUGIN \
	{ "SPU",      \
	  18,         \
	  { { "SPUinit",  \
	      (void*)PEOPS_SPUinit }, \
	    { "SPUshutdown",	\
	      (void*)PEOPS_SPUshutdown}, \
	    { "SPUopen", \
	      (void*)PEOPS_SPUopen}, \
	    { "SPUclose", \
	      (void*)PEOPS_SPUclose}, \
	    { "SPUconfigure", \
	      (void*)PEOPS_SPUsetConfigFile}, \
	    { "SPUabout", \
	      (void*)PEOPS_SPUabout}, \
	    { "SPUtest", \
	      (void*)PEOPS_SPUtest}, \
	    { "SPUwriteRegister", \
	      (void*)PEOPS_SPUwriteRegister}, \
	    { "SPUreadRegister", \
	      (void*)PEOPS_SPUreadRegister}, \
	    { "SPUwriteDMA", \
	      (void*)PEOPS_SPUwriteDMA}, \
	    { "SPUreadDMA", \
	      (void*)PEOPS_SPUreadDMA}, \
	    { "SPUwriteDMAMem", \
	      (void*)PEOPS_SPUwriteDMAMem}, \
	    { "SPUreadDMAMem", \
	      (void*)PEOPS_SPUreadDMAMem}, \
	    { "SPUplayADPCMchannel", \
	      (void*)PEOPS_SPUplayADPCMchannel}, \
	    { "SPUfreeze", \
	      (void*)PEOPS_SPUfreeze}, \
	    { "SPUregisterCallback", \
	      (void*)PEOPS_SPUregisterCallback}, \
	    { "SPUregisterCDDAVolume", \
	      (void*)PEOPS_SPUregisterCDDAVolume}, \
	    { "SPUasync", \
	      (void*)PEOPS_SPUasync} \
	       } }
      
#define FRANSPU_PLUGIN \
	{ "SPU",      \
	  18,         \
	  { { "SPUinit",  \
	      (void*)FRAN_SPU_init }, \
	    { "SPUshutdown",	\
	      (void*)FRAN_SPU_shutdown}, \
	    { "SPUopen", \
	      (void*)FRAN_SPU_open}, \
	    { "SPUclose", \
	      (void*)FRAN_SPU_close}, \
	    { "SPUconfigure", \
	      (void*)FRAN_SPU_setConfigFile}, \
	    { "SPUabout", \
	      (void*)FRAN_SPU_About}, \
	    { "SPUtest", \
	      (void*)FRAN_SPU_test}, \
	    { "SPUwriteRegister", \
	      (void*)FRAN_SPU_writeRegister}, \
	    { "SPUreadRegister", \
	      (void*)FRAN_SPU_readRegister}, \
	    { "SPUwriteDMA", \
	      (void*)FRAN_SPU_writeDMA}, \
	    { "SPUreadDMA", \
	      (void*)FRAN_SPU_readDMA}, \
	    { "SPUwriteDMAMem", \
	      (void*)FRAN_SPU_writeDMAMem}, \
	    { "SPUreadDMAMem", \
	      (void*)FRAN_SPU_readDMAMem}, \
	    { "SPUplayADPCMchannel", \
	      (void*)FRAN_SPU_playADPCMchannel}, \
	    { "SPUfreeze", \
	      (void*)FRAN_SPU_freeze}, \
	    { "SPUregisterCallback", \
	      (void*)FRAN_SPU_registerCallback}, \
	    { "SPUregisterCDDAVolume", \
	      (void*)FRAN_SPU_registerCDDAVolume}, \
	    { "SPUasync", \
	      (void*)FRAN_SPU_async} \
	       } }
	       
#define GPU_NULL_PLUGIN \
	{ "GPU",      \
	  10,         \
	  { { "GPUinit",  \
	      (void*)GPU__init }, \
	    { "GPUshutdown",	\
	      (void*)GPU__shutdown}, \
	    { "GPUopen", \
	      (void*)GPU__open}, \
	    { "GPUclose", \
	      (void*)GPU__close}, \
	    { "GPUwriteStatus", \
	      (void*)GPU__writeStatus}, \
	    { "GPUwriteData", \
	      (void*)GPU__writeData}, \
	    { "GPUreadStatus", \
	      (void*)GPU__readStatus}, \
	    { "GPUreadData", \
	      (void*)GPU__readData}, \
	    { "GPUdmaChain", \
	      (void*)GPU__dmaChain}, \
	    { "GPUupdateLace", \
	      (void*)GPU__updateLace} \
	       } }

#define GPU_PEOPS_PLUGIN \
	{ "GPU",      \
	  14,         \
	  { { "GPUinit",  \
	      (void*)PEOPS_GPUinit }, \
	    { "GPUshutdown",	\
	      (void*)PEOPS_GPUshutdown}, \
	    { "GPUopen", \
	      (void*)PEOPS_GPUopen}, \
	    { "GPUclose", \
	      (void*)PEOPS_GPUclose}, \
	    { "GPUwriteStatus", \
	      (void*)PEOPS_GPUwriteStatus}, \
	    { "GPUwriteData", \
	      (void*)PEOPS_GPUwriteData}, \
	    { "GPUwriteDataMem", \
	      (void*)PEOPS_GPUwriteDataMem}, \
	    { "GPUreadStatus", \
	      (void*)PEOPS_GPUreadStatus}, \
	    { "GPUreadData", \
	      (void*)PEOPS_GPUreadData}, \
	    { "GPUreadDataMem", \
	      (void*)PEOPS_GPUreadDataMem}, \
	    { "GPUdmaChain", \
	      (void*)PEOPS_GPUdmaChain}, \
	    { "GPUdisplayText", \
	      (void*)PEOPS_GPUdisplayText}, \
	    { "GPUfreeze", \
	      (void*)PEOPS_GPUfreeze}, \
	    { "GPUupdateLace", \
	      (void*)PEOPS_GPUupdateLace} \
	       } }

#define PLUGIN_SLOT_0 EMPTY_PLUGIN
//#define PLUGIN_SLOT_1 PAD1_PLUGIN
#define PLUGIN_SLOT_1 SSS_PAD1_PLUGIN
//#define PLUGIN_SLOT_2 PAD2_PLUGIN
#define PLUGIN_SLOT_2 SSS_PAD2_PLUGIN
//#define PLUGIN_SLOT_3 CDR_PLUGIN
#define PLUGIN_SLOT_3 CDRISO_PLUGIN
//#define PLUGIN_SLOT_4 SPU_NULL_PLUGIN
//#define PLUGIN_SLOT_4 SPU_PEOPS_PLUGIN
#define PLUGIN_SLOT_4 FRANSPU_PLUGIN
//#define PLUGIN_SLOT_5 GPU_NULL_PLUGIN
#define PLUGIN_SLOT_5 GPU_PEOPS_PLUGIN
#define PLUGIN_SLOT_6 EMPTY_PLUGIN
#define PLUGIN_SLOT_7 EMPTY_PLUGIN



#endif

