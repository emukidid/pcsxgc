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

#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fat.h>
#include "../PsxCommon.h"
#include "PlugCD.h"
#include "DEBUG.h"
#include "wiiSXconfig.h"
#include "menu/MenuContext.h"
extern "C" {
#include "fileBrowser/fileBrowser.h"
#include "fileBrowser/fileBrowser-libfat.h"
#include "fileBrowser/fileBrowser-DVD.h"
#include "fileBrowser/fileBrowser-CARD.h"
}

#ifdef WII
unsigned int MALLOC_MEM2 = 0;
extern "C" {
#include <di/di.h>
}
#endif //WII

/* function prototypes */
extern "C" {
int SysInit();
void SysReset();
void SysClose();
void SysPrintf(char *fmt, ...);
void *SysLoadLibrary(char *lib);
void *SysLoadSym(void *lib, char *sym);
char *SysLibError();
void SysCloseLibrary(void *lib);
void SysUpdate();
void SysRunGui();
void SysMessage(char *fmt, ...);
}

u32* xfb[2] = { NULL, NULL };	/*** Framebuffers ***/
int whichfb = 0;        /*** Frame buffer toggle ***/
GXRModeObj *vmode;				/*** Graphics Mode Object ***/
#define DEFAULT_FIFO_SIZE ( 256 * 1024 )
extern int controllerType;
BOOL hasLoadedISO = FALSE;
fileBrowser_file *isoFile  = NULL;  //the ISO file
fileBrowser_file *biosFile = NULL;  //BIOS file

#if defined (CPU_LOG) || defined(DMA_LOG) || defined(CDR_LOG) || defined(HW_LOG) || \
	defined(BIOS_LOG) || defined(GTE_LOG) || defined(PAD_LOG)
FILE *emuLog;
#endif

PcsxConfig Config;
char dynacore;
char biosDevice;
char frameLimit;
char frameSkip;
extern char audioEnabled;
char showFPSonScreen;
char printToScreen;
char menuActive;
char saveEnabled;
char creditsScrolling;
char padNeedScan=1;
char wpadNeedScan=1;
char shutdown = 0;
char nativeSaveDevice;
char saveStateDevice;
char autoSave;
char autoSaveLoaded = 0;
char screenMode = 0;
char padAutoAssign;
char padType[4];
char padAssign[4];
char pakMode[4];

int stop = 0;

void ScanPADSandReset(u32 dummy) 
{
	PAD_ScanPads();
	padNeedScan = wpadNeedScan = 1;
	if(!((*(u32*)0xCC003000)>>16))
		stop = 1;
}

#ifdef HW_RVL
void ShutdownWii() 
{
	shutdown = 1;
	stop = 1;
}
#endif

void video_mode_init(GXRModeObj *videomode,unsigned int *fb1, unsigned int *fb2)
{
	vmode = videomode;
	xfb[0] = fb1;
	xfb[1] = fb2;
}

// Plugin structure
extern "C" {
#include "GamecubePlugins.h"
PluginTable plugins[] =
	{ PLUGIN_SLOT_0,
	  PLUGIN_SLOT_1,
	  PLUGIN_SLOT_2,
	  PLUGIN_SLOT_3,
	  PLUGIN_SLOT_4,
	  PLUGIN_SLOT_5,
	  PLUGIN_SLOT_6,
	  PLUGIN_SLOT_7 };
}

long LoadCdBios=0;

int main(int argc, char *argv[]) 
{
	/* INITIALIZE */
#ifdef HW_RVL
	//DI_Close();
	DI_Init();    // first
#endif

	MenuContext *menu = new MenuContext(vmode);
	VIDEO_SetPostRetraceCallback (ScanPADSandReset);
#ifndef WII
	DVD_Init();
#endif

#ifdef DEBUGON
	//DEBUG_Init(GDBSTUB_DEVICE_TCP,GDBSTUB_DEF_TCPPORT); //Default port is 2828
	DEBUG_Init(GDBSTUB_DEVICE_USB, 1);
	_break();
#endif

	// Default Settings
	audioEnabled     = 1; // Audio
#ifdef RELEASE
	showFPSonScreen  = 0; // Show FPS on Screen
#else
	showFPSonScreen  = 1; // Show FPS on Screen
#endif
	printToScreen    = 1; // Show DEBUG text on screen
	printToSD        = 0; // Disable SD logging
	frameLimit		 = 1; // Auto limit FPS
	frameSkip		 = 0; // Disable frame skipping
	iUseDither		 = 1; // Default dithering
	saveEnabled      = 0; // Don't save game
	nativeSaveDevice = 0; // SD
	saveStateDevice	 = 0; // SD
	autoSave         = 1; // Auto Save Game
	creditsScrolling = 0; // Normal menu for now
	dynacore         = 0; // Dynarec
	screenMode		 = 0; // Stretch FB horizontally
/*	padAutoAssign	 = PADAUTOASSIGN_AUTOMATIC;
	padType[0]		 = PADTYPE_NONE;
	padType[1]		 = PADTYPE_NONE;
	padType[2]		 = PADTYPE_NONE;
	padType[3]		 = PADTYPE_NONE;
	padAssign[0]	 = PADASSIGN_INPUT0;
	padAssign[1]	 = PADASSIGN_INPUT1;
	padAssign[2]	 = PADASSIGN_INPUT2;
	padAssign[3]	 = PADASSIGN_INPUT3;
	pakMode[0]		 = PAKMODE_MEMPAK; // memPak plugged into controller 1
	pakMode[1]		 = PAKMODE_MEMPAK;
	pakMode[2]		 = PAKMODE_MEMPAK;
	pakMode[3]		 = PAKMODE_MEMPAK;*/
#ifdef GLN64_GX
// glN64 specific  settings
// 	glN64_useFrameBufferTextures = 0; // Disable FrameBuffer textures
//	glN64_use2xSaiTextures = 0;	// Disable 2xSai textures
//	renderCpuFramebuffer = 0; // Disable CPU Framebuffer Rendering
#endif //GLN64_GX
	menuActive = 1;

	//PCSX-specific defaults
	memset(&Config, 0, sizeof(PcsxConfig));
	controllerType=0;	//Standard controller
	Config.Cpu=dynacore;		//Dynarec core
	strcpy(Config.Net,"Disabled");
	Config.PsxOut = 1;
	Config.HLE = 1;
	Config.Xa = 0;  //XA enabled
	Config.Cdda = 1;
	iVolume=3; //Volume="medium" in PEOPSspu
	Config.PsxAuto = 1; //Autodetect
	LoadCdBios=0;
  
#if 0
	//config stuff
	fileBrowser_file* configFile_file;
	int (*configFile_init)(fileBrowser_file*) = fileBrowser_libfat_init;
#ifdef HW_RVL
	if(argv[0][0] == 'u') {  //assume USB
		configFile_file = &saveDir_libfat_USB;
		if(configFile_init(configFile_file)) {                //only if device initialized ok
			FILE* f = fopen( "usb:/wiisx/settings.cfg", "r" );  //attempt to open file
			if(f) {        //open ok, read it
				readConfig(f);
				fclose(f);
			}
		}
	}
	else /*if((argv[0][0]=='s') || (argv[0][0]=='/'))*/
#endif
	{ //assume SD
		configFile_file = &saveDir_libfat_Default;
		if(configFile_init(configFile_file)) {                //only if device initialized ok
			FILE* f = fopen( "sd:/wiisx/settings.cfg", "r" );  //attempt to open file
			if(f) {        //open ok, read it
				readConfig(f);
				fclose(f);
			}
		}
	}
#ifdef HW_RVL
	// Handle options passed in through arguments
	int i;
	for(i=1; i<argc; ++i){
		handleConfigPair(argv[i]);
	}
#endif
#endif //0

	while (menu->isRunning()) {}

	delete menu;

	return 0;
}

int loadISO(fileBrowser_file* file) 
{
	if(isoFile) free(isoFile);
	isoFile = (fileBrowser_file*) memalign(32,sizeof(fileBrowser_file));
	memcpy( isoFile, file, sizeof(fileBrowser_file) );

	if(!hasLoadedISO) {
  	
  	//Init biosFile. TODO: perform this in menu
  	//do the same as above but for the bios(s)
  	if(1) { //SD
  		biosFile_dir = &biosDir_libfat_Default;
  		biosFile_readFile  = fileBrowser_libfat_readFile;
  		biosFile_init      = fileBrowser_libfat_init;
  		biosFile_deinit    = fileBrowser_libfat_deinit;
  		//biosFile_init(saveFile_dir);
  	} //fixme: code for the rest of the devices
	

  	biosFile = (fileBrowser_file*)memalign(32,sizeof(fileBrowser_file)); //also hardcoded for SD.
  	memcpy(biosFile,&biosDir_libfat_Default,sizeof(fileBrowser_file));
  	strcat(biosFile->name, "/SCPH1001.BIN");          // Use actual BIOS
  	//biosFile_init(biosFile);  //initialize this device
	  SysInit();        //Call me early to avoid fragmentation
  }
	if(hasLoadedISO) {
  	SysClose();	
  	SysInit();        //Call me early to avoid fragmentation
	}
	hasLoadedISO = 1;
	CheckCdrom();
	SysReset();
	LoadCdrom();
	
	if(autoSave==AUTOSAVE_ENABLE) {
    switch (nativeSaveDevice)
    {
    	case NATIVESAVEDEVICE_SD:
    	case NATIVESAVEDEVICE_USB:
    		// Adjust saveFile pointers
    		saveFile_dir = (nativeSaveDevice==NATIVESAVEDEVICE_SD) ? &saveDir_libfat_Default:&saveDir_libfat_USB;
    		saveFile_readFile  = fileBrowser_libfat_readFile;
    		saveFile_writeFile = fileBrowser_libfat_writeFile;
    		saveFile_init      = fileBrowser_libfat_init;
    		saveFile_deinit    = fileBrowser_libfat_deinit;
    		break;
    	case NATIVESAVEDEVICE_CARDA:
    	case NATIVESAVEDEVICE_CARDB:
    		// Adjust saveFile pointers
    		saveFile_dir       = (nativeSaveDevice==NATIVESAVEDEVICE_CARDA) ? &saveDir_CARD_SlotA:&saveDir_CARD_SlotB;
    		saveFile_readFile  = fileBrowser_CARD_readFile;
    		saveFile_writeFile = fileBrowser_CARD_writeFile;
    		saveFile_init      = fileBrowser_CARD_init;
    		saveFile_deinit    = fileBrowser_CARD_deinit;
    		break;
    }
    // Try loading everything
  	int result = 0;
  	saveFile_init(saveFile_dir);
  	result += LoadMcd(1,saveFile_dir);
  	result += LoadMcd(2,saveFile_dir);
  	saveFile_deinit(saveFile_dir);

  	switch (nativeSaveDevice)
  	{
  		case NATIVESAVEDEVICE_SD:
  			if (result) autoSaveLoaded = NATIVESAVEDEVICE_SD;
  			break;
  		case NATIVESAVEDEVICE_USB:
  			if (result) autoSaveLoaded = NATIVESAVEDEVICE_USB;
  			break;
  		case NATIVESAVEDEVICE_CARDA:
  			if (result) autoSaveLoaded = NATIVESAVEDEVICE_CARDA;
  			break;
  		case NATIVESAVEDEVICE_CARDB:
  			if (result) autoSaveLoaded = NATIVESAVEDEVICE_CARDB;
  			break;
  	}
  }	
	
	return 0;
}

#if 0
//int loadISO(fileBrowser_file* iso)
int loadISO(void)
{
	// Configure pcsx //
	memset(&Config, 0, sizeof(PcsxConfig));

	u16 butns=0;
	printf("Select Controller Type:\n(A) Standard : (B) Analog\n");
	do{butns = PAD_ButtonsDown(0);}while(!((butns & PAD_BUTTON_A) || (butns & PAD_BUTTON_B)));
	if(butns & PAD_BUTTON_A)  controllerType=0;
	else  controllerType=1;
	printf("%s selected\n",controllerType ? "Analog":"Standard");

	do{butns = PAD_ButtonsDown(0);}while(((butns & PAD_BUTTON_A) || (butns & PAD_BUTTON_B)));

	printf("Select Core Type:\n(X) Dynarec : (Y) Interpreter\n");
	do{butns = PAD_ButtonsDown(0);}while(!((butns & PAD_BUTTON_X) || (butns & PAD_BUTTON_Y)));
	if(butns & PAD_BUTTON_X)  Config.Cpu=0;
	else  Config.Cpu=1;
	printf("%s selected\n",Config.Cpu ? "Interpreter":"Dynarec");

	do{butns = PAD_ButtonsDown(0);}while(((butns & PAD_BUTTON_X) || (butns & PAD_BUTTON_Y)));

	printf("Press A\n");
	while(!(PAD_ButtonsDown(0) & PAD_BUTTON_A));
	while((PAD_ButtonsDown(0) & PAD_BUTTON_A));


	strcpy(Config.Bios, "SCPH1001.BIN"); // Use actual BIOS
	strcpy(Config.BiosDir, "/PSXISOS/");
	strcpy(Config.Net,"Disabled");
	strcpy(Config.Mcd1,"/PSXISOS/Memcard1.mcd");
	strcpy(Config.Mcd2,"/PSXISOS/Memcard2.mcd");
	Config.PsxOut = 0; //Disable SysPrintf to console
	Config.HLE = 1;
	Config.Xa = 0;  //XA enabled
	Config.Cdda = 1;
	Config.PsxAuto = 1; //Autodetect
	if (SysInit() == -1)
	{
		printf("SysInit() Error!\n");
		while(1);
	}
	OpenPlugins();

	SysReset();
	CheckCdrom();
	LoadCdrom();

	return 0;

}
#endif

extern "C" {
//System Functions
void go(void)
{
	Config.PsxOut = 0;
	stop = 0;
	psxCpu->Execute();
}

int SysInit() 
{
#if defined (CPU_LOG) || defined(DMA_LOG) || defined(CDR_LOG) || defined(HW_LOG) || \
	defined(BIOS_LOG) || defined(GTE_LOG) || defined(PAD_LOG)
	emuLog = fopen("/PSXISOS/emu.log", "w");
#endif

	psxInit();
	LoadPlugins();
	OpenPlugins();
	return 0;
}

void SysReset() 
{
	psxReset();
}

void SysClose() 
{
	psxShutdown();
	ClosePlugins();
	ReleasePlugins();
#if defined (CPU_LOG) || defined(DMA_LOG) || defined(CDR_LOG) || defined(HW_LOG) || \
	defined(BIOS_LOG) || defined(GTE_LOG) || defined(PAD_LOG)
	if (emuLog != NULL) fclose(emuLog);
#endif
}

void SysPrintf(char *fmt, ...) 
{
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	if (Config.PsxOut) printf ("%s", msg);
#if defined (CPU_LOG) || defined(DMA_LOG) || defined(CDR_LOG) || defined(HW_LOG) || \
	defined(BIOS_LOG) || defined(GTE_LOG) || defined(PAD_LOG)
	fprintf(emuLog, "%s", msg);
#endif
}

void *SysLoadLibrary(char *lib) 
{
	int i;
	for(i=0; i<NUM_PLUGINS; i++)
		if((plugins[i].lib != NULL) && (!strcmp(lib, plugins[i].lib)))
			return (void*)i;
	return NULL;
}

void *SysLoadSym(void *lib, char *sym) 
{
	PluginTable* plugin = plugins + (int)lib;
	int i;
	for(i=0; i<plugin->numSyms; i++)
		if(plugin->syms[i].sym && !strcmp(sym, plugin->syms[i].sym))
			return plugin->syms[i].pntr;
	return NULL;
}

char *SysLibError() 
{
	return NULL;
}

void SysCloseLibrary(void *lib) 
{
//	dlclose(lib);
}

int framesdone = 0;
void SysUpdate() 
{
	framesdone++;
//	PADhandleKey(PAD1_keypressed());
//	PADhandleKey(PAD2_keypressed());
}

void SysRunGui() 
{
//	RunGui();
}

void SysMessage(char *fmt, ...) 
{

}

} //extern "C"
