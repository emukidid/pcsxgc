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
#include "gc_input/controller.h"
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
char padType[2];
char padAssign[2];
char controllerType;
char numMultitaps;

int stop = 0;

static struct {
	char* key;
	char* value; // Not a string, but a char pointer
	char  min, max;
} OPTIONS[] =
{ { "Audio", &audioEnabled, AUDIO_DISABLE, AUDIO_ENABLE },
  { "FPS", &showFPSonScreen, FPS_HIDE, FPS_SHOW },
//  { "Debug", &printToScreen, DEBUG_HIDE, DEBUG_SHOW },
  { "ScreenMode", &screenMode, SCREENMODE_4x3, SCREENMODE_16x9 },
  { "Core", &dynacore, DYNACORE_DYNAREC, DYNACORE_INTERPRETER },
  { "NativeDevice", &nativeSaveDevice, NATIVESAVEDEVICE_SD, NATIVESAVEDEVICE_CARDB },
  { "StatesDevice", &saveStateDevice, SAVESTATEDEVICE_SD, SAVESTATEDEVICE_USB },
  { "AutoSave", &autoSave, AUTOSAVE_DISABLE, AUTOSAVE_ENABLE },
  { "BiosDevice", &biosDevice, BIOSDEVICE_HLE, BIOSDEVICE_USB },
  { "LimitFrames", &frameLimit, FRAMELIMIT_NONE, FRAMELIMIT_AUTO },
  { "SkipFrames", &frameSkip, FRAMESKIP_DISABLE, FRAMESKIP_ENABLE },
  { "PadAutoAssign", &padAutoAssign, PADAUTOASSIGN_MANUAL, PADAUTOASSIGN_AUTOMATIC },
  { "PadType1", &padType[0], PADTYPE_NONE, PADTYPE_WII },
  { "PadType2", &padType[1], PADTYPE_NONE, PADTYPE_WII },
  { "PadAssign1", &padAssign[0], PADASSIGN_INPUT0, PADASSIGN_INPUT3 },
  { "PadAssign2", &padAssign[1], PADASSIGN_INPUT0, PADASSIGN_INPUT3 },
  { "ControllerType", &controllerType, CONTROLLERTYPE_STANDARD, CONTROLLERTYPE_LIGHTGUN },
  { "NumberMultitaps", &numMultitaps, MULTITAPS_NONE, MULTITAPS_TWO },
};
void handleConfigPair(char* kv);
void readConfig(FILE* f);
void writeConfig(FILE* f);

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
	padAutoAssign	 = PADAUTOASSIGN_AUTOMATIC;
	padType[0]		 = PADTYPE_NONE;
	padType[1]		 = PADTYPE_NONE;
	padAssign[0]	 = PADASSIGN_INPUT0;
	padAssign[1]	 = PADASSIGN_INPUT1;
	controllerType	 = CONTROLLERTYPE_STANDARD;
	numMultitaps	 = MULTITAPS_NONE;
	menuActive = 1;

	//PCSX-specific defaults
	memset(&Config, 0, sizeof(PcsxConfig));
	Config.Cpu=dynacore;		//Dynarec core
	strcpy(Config.Net,"Disabled");
	Config.PsxOut = 1;
	Config.HLE = 1;
	Config.Xa = 0;  //XA enabled
	Config.Cdda = 1;
	iVolume=3; //Volume="medium" in PEOPSspu
	Config.PsxAuto = 1; //Autodetect
	LoadCdBios=0;
  
	update_controller_assignment(); //Perform controller auto assignment at least once at startup.

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

	//Synch settings with Config
	Config.Cpu=dynacore;

	while (menu->isRunning()) {}

	delete menu;

	return 0;
}

int loadISO(fileBrowser_file* file) 
{
	if(isoFile) free(isoFile);
	isoFile = (fileBrowser_file*) memalign(32,sizeof(fileBrowser_file));
	memcpy( isoFile, file, sizeof(fileBrowser_file) );

	if(hasLoadedISO) {
  	SysClose();	
	}
	SysInit();
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

void setOption(char* key, char value){
	for(unsigned int i=0; i<sizeof(OPTIONS)/sizeof(OPTIONS[0]); ++i){
		if(!strcmp(OPTIONS[i].key, key)){
			if(value >= OPTIONS[i].min && value <= OPTIONS[i].max)
				*OPTIONS[i].value = value;
			break;
		}
	}
}

void handleConfigPair(char* kv){
	char* vs = kv;
	while(*vs != ' ' && *vs != '\t' && *vs != ':' && *vs != '=')
			++vs;
	*(vs++) = 0;
	while(*vs == ' ' || *vs == '\t' || *vs == ':' || *vs == '=')
			++vs;

	setOption(kv, atoi(vs));
}

void readConfig(FILE* f){
	char line[256];
	while(fgets(line, 256, f)){
		if(line[0] == '#') continue;
		handleConfigPair(line);
	}
}

void writeConfig(FILE* f){
	for(unsigned int i=0; i<sizeof(OPTIONS)/sizeof(OPTIONS[0]); ++i){
		fprintf(f, "%s = %d\n", OPTIONS[i].key, *OPTIONS[i].value);
	}
}

extern "C" {
//System Functions
void go(void) {
	Config.PsxOut = 0;
	stop = 0;
	psxCpu->Execute();
}

int SysInit() {
#if defined (CPU_LOG) || defined(DMA_LOG) || defined(CDR_LOG) || defined(HW_LOG) || \
	defined(BIOS_LOG) || defined(GTE_LOG) || defined(PAD_LOG)
	emuLog = fopen("/PSXISOS/emu.log", "w");
#endif
  Config.Cpu = dynacore;  //cpu may have changed  
  psxInit();
  LoadPlugins();
  OpenPlugins();
  
  //Init biosFile pointers and stuff
  if(biosDevice != BIOSDEVICE_HLE) {
   	biosFile_dir = (biosDevice == BIOSDEVICE_SD) ? &biosDir_libfat_Default : &biosDir_libfat_USB;
  	biosFile_readFile  = fileBrowser_libfat_readFile;
  	biosFile_init      = fileBrowser_libfat_init;
  	biosFile_deinit    = fileBrowser_libfat_deinit;
	 	if(biosFile) {
    	free(biosFile);
	 	}
  	biosFile = (fileBrowser_file*)memalign(32,sizeof(fileBrowser_file));
    memcpy(biosFile,&biosDir_libfat_Default,sizeof(fileBrowser_file));
    strcat(biosFile->name, "/SCPH1001.BIN");
    biosFile_init(biosFile);  //initialize the bios device (it might not be the same as ISO device)
    Config.HLE = BIOS_USER_DEFINED;
  } else {
    Config.HLE = BIOS_HLE;
  }

	return 0;
}

void SysReset() {
	psxReset();
}

void SysStartCPU() {
  Config.PsxOut = 0;
	stop = 0;
  psxCpu->Execute();
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
#ifdef PRINTGECKO
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	//if (Config.PsxOut) printf ("%s", msg);
	DEBUG_print(msg, DBG_USBGECKO);
#if defined (CPU_LOG) || defined(DMA_LOG) || defined(CDR_LOG) || defined(HW_LOG) || \
	defined(BIOS_LOG) || defined(GTE_LOG) || defined(PAD_LOG)
	fprintf(emuLog, "%s", msg);
#endif
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

int framesdone = 0;
void SysUpdate() 
{
	framesdone++;
}

void SysRunGui() {}
void SysMessage(char *fmt, ...) {}
void SysCloseLibrary(void *lib) {}
char *SysLibError() {	return NULL; }

} //extern "C"
