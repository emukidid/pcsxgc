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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <psemu_plugin_defs.h>

#define SYMS_PER_LIB 22
typedef struct {
	const char* lib;
	int   numSyms;
	struct {
		const char* sym;
		void* pntr;
	} syms[SYMS_PER_LIB];
} PluginTable;
#define NUM_PLUGINS 6

/* GPU */
long GPUopen(unsigned long *disp, char *cap, char *cfg);
long GPUinit(void);
long GPUshutdown(void);
long GPUclose(void);
void GPUwriteStatus(unsigned long);
void GPUwriteData(unsigned long);
void GPUwriteDataMem(unsigned long *, int);
unsigned long GPUreadStatus(void);
unsigned long GPUreadData(void);
void GPUreadDataMem(unsigned long *, int);
long GPUdmaChain(unsigned long *,unsigned long, unsigned long *);
void GPUupdateLace(void);
void GPUdisplayText(char *);
long GPUfreeze(unsigned long,void *);
void GPUrearmedCallbacks(const void **cbs);


/* PAD */
//typedef long (* PADopen)(unsigned long *);
extern long PAD__init(long);
extern long PAD__shutdown(void);
extern long PAD__open(void);
extern long PAD__close(void);
extern long PAD1__readPort1(PadDataS *pad);
extern long PAD2__readPort2(PadDataS *pad);
unsigned char CALLBACK PAD1__poll(unsigned char value, int *more_data);
unsigned char CALLBACK PAD2__poll(unsigned char value, int *more_data);
unsigned char CALLBACK PAD1__startPoll(int pad);
unsigned char CALLBACK PAD2__startPoll(int pad);


/* DFSound Plugin */
int CALLBACK SPUplayCDDAchannel(short *pcm, int nbytes, unsigned int cycle, int is_start);
void CALLBACK SPUplayADPCMchannel(void *xap, unsigned int cycle, int is_start);
void CALLBACK SPUupdate(void);
void CALLBACK SPUasync(unsigned int cycle, unsigned int flags);
long CALLBACK SPUinit(void);
long CALLBACK SPUshutdown(void);
void CALLBACK SPUregisterCallback(void (CALLBACK *callback)(void));
void CALLBACK SPUregisterCDDAVolume(void (CALLBACK *CDDAVcallback)(short, short));
void CALLBACK SPUregisterScheduleCb(void (CALLBACK *callback)(unsigned int));
void CALLBACK SPUwriteDMAMem(unsigned short *pusPSXMem, int iSize, unsigned int cycles);
void CALLBACK SPUreadDMAMem(unsigned short *pusPSXMem, int iSize, unsigned int cycles);
unsigned short CALLBACK SPUreadRegister(unsigned long reg);
void CALLBACK SPUwriteRegister(unsigned long reg, unsigned short val, unsigned int cycles);
long CALLBACK SPUopen(void);
long CALLBACK SPUclose(void);
long CALLBACK SPUfreeze(uint32_t ulFreezeMode, void * pF, uint32_t cycles);

#define EMPTY_PLUGIN \
	{ NULL,      \
	  0,         \
	  { { NULL,  \
	      NULL }, } }

#define PAD1_PLUGIN \
	{ "plugins/builtin_pad",      \
	  7,         \
	  { { "PADinit",  \
	      (void*)PAD__init }, \
	    { "PADshutdown",	\
	      (void*)PAD__shutdown}, \
	    { "PADopen", \
	      (void*)PAD__open}, \
	    { "PADclose", \
	      (void*)PAD__close}, \
	    { "PADreadPort1", \
	      (void*)PAD1__readPort1}, \
	    { "PADstartPoll", \
	      (void*)PAD1__startPoll}, \
	    { "PADpoll", \
	      (void*)PAD1__poll} \
	       } }

#define PAD2_PLUGIN \
	{ "plugins/builtin_pad2",      \
	  7,         \
	  { { "PADinit",  \
	      (void*)PAD__init }, \
	    { "PADshutdown",	\
	      (void*)PAD__shutdown}, \
	    { "PADopen", \
	      (void*)PAD__open}, \
	    { "PADclose", \
	      (void*)PAD__close}, \
	    { "PADreadPort2", \
	      (void*)PAD2__readPort2}, \
	    { "PADstartPoll", \
	      (void*)PAD2__startPoll}, \
	    { "PADpoll", \
	      (void*)PAD2__poll} \
	       } }

#define DFSOUND_PLUGIN \
	{ "plugins/builtin_spu",      \
	  20,         \
	  { { "SPUinit",  \
	      (void*)SPUinit }, \
	    { "SPUshutdown",	\
	      (void*)SPUshutdown}, \
	    { "SPUopen", \
	      (void*)SPUopen}, \
	    { "SPUclose", \
	      (void*)SPUclose}, \
	    { "SPUwriteRegister", \
	      (void*)SPUwriteRegister}, \
	    { "SPUreadRegister", \
	      (void*)SPUreadRegister}, \
	    { "SPUwriteDMAMem", \
	      (void*)SPUwriteDMAMem}, \
	    { "SPUreadDMAMem", \
	      (void*)SPUreadDMAMem}, \
	    { "SPUplayADPCMchannel", \
	      (void*)SPUplayADPCMchannel}, \
	    { "SPUfreeze", \
	      (void*)SPUfreeze}, \
	    { "SPUregisterCallback", \
	      (void*)SPUregisterCallback}, \
	    { "SPUregisterCDDAVolume", \
	      (void*)SPUregisterCDDAVolume}, \
	    { "SPUplayCDDAchannel", \
	      (void*)SPUplayCDDAchannel}, \
		{ "SPUregisterScheduleCb", \
	      (void*)SPUregisterScheduleCb}, \
	    { "SPUasync", \
	      (void*)SPUasync} \
	       } }

#define GPU_PEOPS_PLUGIN \
	{ "plugins/builtin_gpu",      \
	  14,         \
	  { { "GPUinit",  \
	      (void*)GPUinit }, \
	    { "GPUshutdown",	\
	      (void*)GPUshutdown}, \
	    { "GPUopen", \
	      (void*)GPUopen}, \
	    { "GPUclose", \
	      (void*)GPUclose}, \
	    { "GPUwriteStatus", \
	      (void*)GPUwriteStatus}, \
	    { "GPUwriteData", \
	      (void*)GPUwriteData}, \
	    { "GPUwriteDataMem", \
	      (void*)GPUwriteDataMem}, \
	    { "GPUreadStatus", \
	      (void*)GPUreadStatus}, \
	    { "GPUreadData", \
	      (void*)GPUreadData}, \
	    { "GPUreadDataMem", \
	      (void*)GPUreadDataMem}, \
	    { "GPUdmaChain", \
	      (void*)GPUdmaChain}, \
	    { "GPUfreeze", \
	      (void*)GPUfreeze}, \
	    { "GPUupdateLace", \
	      (void*)GPUupdateLace}, \
		{ "GPUrearmedCallbacks", \
	      (void*)GPUrearmedCallbacks} \
	       } }


// Plugin structure
static PluginTable plugins[] = {
	EMPTY_PLUGIN,
	PAD1_PLUGIN,
	PAD2_PLUGIN,
	DFSOUND_PLUGIN,
	GPU_PEOPS_PLUGIN,
	EMPTY_PLUGIN,
};

extern void SysPrintf(const char *fmt, ...);

void *SysLoadLibrary(const char *lib)
{
	int i;
	for(i=0; i<NUM_PLUGINS; i++)
		if((plugins[i].lib != NULL) && (!strcmp(lib, plugins[i].lib)))
			return (void*)i;
	SysPrintf("SysLoadLibrary(%s) couldn't be found!\r\n", lib);
	return NULL;
}

void *SysLoadSym(void *lib, const char *sym)
{
	PluginTable* plugin = plugins + (int)lib;
	int i;
	for(i=0; i<plugin->numSyms; i++)
		if(plugin->syms[i].sym && !strcmp(sym, plugin->syms[i].sym))
			return plugin->syms[i].pntr;
	SysPrintf("SysLoadSym(%s, %s) couldn't be found!\r\n", lib, sym);
	return NULL;
}

void SysCloseLibrary(void *lib)
{
}

const char *SysLibError()
{
	return NULL;
}
