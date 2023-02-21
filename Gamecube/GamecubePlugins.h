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

#define SYMS_PER_LIB 20
typedef struct {
	const char* lib;
	int   numSyms;
	struct {
		const char* sym;
		void* pntr;
	} syms[SYMS_PER_LIB];
} PluginTable;
#define NUM_PLUGINS 5

/* SPU NULL */
//typedef long (* SPUopen)(void);
void NULL_SPUwriteRegister(unsigned long reg, unsigned short val);
unsigned short NULL_SPUreadRegister(unsigned long reg);
unsigned short NULL_SPUreadDMA(void);
void NULL_SPUwriteDMA(unsigned short val);
void NULL_SPUwriteDMAMem(unsigned short * pusPSXMem,int iSize);
void NULL_SPUreadDMAMem(unsigned short * pusPSXMem,int iSize);
void NULL_SPUplayADPCMchannel(xa_decode_t *xap);
long NULL_SPUinit(void);
long NULL_SPUopen(void);
void NULL_SPUsetConfigFile(char * pCfg);
long NULL_SPUclose(void);
long NULL_SPUshutdown(void);
long NULL_SPUtest(void);
void NULL_SPUregisterCallback(void (*callback)(void));
void NULL_SPUregisterCDDAVolume(void (*CDDAVcallback)(unsigned short,unsigned short));
char * NULL_SPUgetLibInfos(void);
void NULL_SPUabout(void);
long NULL_SPUfreeze(unsigned long ulFreezeMode,SPUFreeze_t *);


/* franspu */
//spu_registers.cpp
void FRAN_SPU_writeRegister(unsigned long reg, unsigned short val);
unsigned short FRAN_SPU_readRegister(unsigned long reg);
//spu_dma.cpp
unsigned short FRAN_SPU_readDMA(void);
void FRAN_SPU_readDMAMem(unsigned short * pusPSXMem,int iSize);
void FRAN_SPU_writeDMA(unsigned short val);
void FRAN_SPU_writeDMAMem(unsigned short * pusPSXMem,int iSize);
//spu.cpp
void FRAN_SPU_async(unsigned long cycle);
void FRAN_SPU_playADPCMchannel(xa_decode_t *xap);
long FRAN_SPU_init(void);
s32 FRAN_SPU_open(void);
long FRAN_SPU_close(void);
long FRAN_SPU_shutdown(void);
long FRAN_SPU_freeze(unsigned long ulFreezeMode,SPUFreeze_t * pF);
void FRAN_SPU_setConfigFile(char *cfgfile);
void FRAN_SPU_About();
void FRAN_SPU_test();
void FRAN_SPU_registerCallback(void (*callback)(void));
void FRAN_SPU_registerCDDAVolume(void (*CDDAVcallback)(unsigned short,unsigned short));


/* PEOPS GPU */
long PEOPS_GPUopen(unsigned long *, char *, char *); 
long PEOPS_GPUinit(void);
long PEOPS_GPUshutdown(void);
long PEOPS_GPUclose(void);
void PEOPS_GPUwriteStatus(unsigned long);
void PEOPS_GPUwriteData(unsigned long);
void PEOPS_GPUwriteDataMem(unsigned long *, int);
unsigned long PEOPS_GPUreadStatus(void);
unsigned long PEOPS_GPUreadData(void);
void PEOPS_GPUreadDataMem(unsigned long *, int);
long PEOPS_GPUdmaChain(unsigned long *,unsigned long);
void PEOPS_GPUupdateLace(void);
void PEOPS_GPUdisplayText(char *);
long PEOPS_GPUfreeze(unsigned long,GPUFreeze_t *);


/* PAD */
//typedef long (* PADopen)(unsigned long *);
extern long PAD__init(long);
extern long PAD__shutdown(void);	

/* WiiSX PAD Plugin */
extern long PAD__open(void);
extern long PAD__close(void);
unsigned char PAD__startPoll (int pad);
unsigned char PAD__poll (const unsigned char value);
long PAD__readPort1(PadDataS*);
long PAD__readPort2(PadDataS*);

/* SSSPSX PAD Plugin */
long SSS_PADopen (void *p);
long SSS_PADclose (void);
unsigned char SSS_PADstartPoll (int pad);
unsigned char SSS_PADpoll (const unsigned char value);
long SSS_PADreadPort1 (PadDataS* pads);
long SSS_PADreadPort2 (PadDataS* pads);

#define EMPTY_PLUGIN \
	{ NULL,      \
	  0,         \
	  { { NULL,  \
	      NULL }, } }

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

#define PLUGIN_SLOT_0 SSS_PAD1_PLUGIN
#define PLUGIN_SLOT_1 SSS_PAD2_PLUGIN
#define PLUGIN_SLOT_2 FRANSPU_PLUGIN
#define PLUGIN_SLOT_3 GPU_PEOPS_PLUGIN
#define PLUGIN_SLOT_4 EMPTY_PLUGIN

#endif

