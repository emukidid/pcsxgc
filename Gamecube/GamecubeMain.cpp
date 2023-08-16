/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2002  Pcsx Team
 *  Copyright (C) 2009-2010  WiiSX Team
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

#include <libpcsxcore/system.h>

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
#include <aesndlib.h>
#ifdef DEBUGON
# include <debug.h>
#endif
#include <libpcsxcore/psxcommon.h>
#include "wiiSXconfig.h"
#include "menu/MenuContext.h"
extern "C" {
#include <libpcsxcore/lightrec/mem.h>
#include "../dfsound/spu_config.h"
#include "DEBUG.h"
#include "fileBrowser/fileBrowser.h"
#include "fileBrowser/fileBrowser-libfat.h"
#include "fileBrowser/fileBrowser-CARD.h"
#include "fileBrowser/fileBrowser-SMB.h"
#include "gc_input/controller.h"
#ifdef HW_DOL
#include "ARAM.h"
#endif
#include "vm/vm.h"
}

#ifdef WII
#include "MEM2.h"
unsigned int MALLOC_MEM2 = 0;
extern "C" {
#include <di/di.h>
extern u32 __di_check_ahbprot(void);
}
#endif //WII

u32* xfb[2] = { NULL, NULL };	/*** Framebuffers ***/
int whichfb = 0;        /*** Frame buffer toggle ***/
GXRModeObj *vmode;				/*** Graphics Mode Object ***/
#define DEFAULT_FIFO_SIZE ( 256 * 1024 )
BOOL hasLoadedISO = FALSE;
fileBrowser_file isoFile;  //the ISO file
fileBrowser_file cddaFile; //the CDDA file
fileBrowser_file subFile;  //the SUB file
fileBrowser_file *biosFile = NULL;  //BIOS file

#if defined (CPU_LOG) || defined(DMA_LOG) || defined(CDR_LOG) || defined(HW_LOG) || \
	defined(BIOS_LOG) || defined(GTE_LOG) || defined(PAD_LOG)
FILE *emuLog;
#endif

extern "C" PcsxConfig Config;

char dynacore;
char biosDevice;
char LoadCdBios=0;
char frameLimit;
char frameSkip;
extern char audioEnabled;
char volume;
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
signed char autoSaveLoaded = 0;
char screenMode = 0;
char videoMode = 0;
char fileSortMode = 1;
char padAutoAssign;
char padType[2];
char padAssign[2];
char rumbleEnabled;
char loadButtonSlot;
char controllerType;
char numMultitaps;

#define CONFIG_STRING_TYPE 0
#define CONFIG_STRING_SIZE 256
char smbUserName[CONFIG_STRING_SIZE];
char smbPassWord[CONFIG_STRING_SIZE];
char smbShareName[CONFIG_STRING_SIZE];
char smbIpAddr[CONFIG_STRING_SIZE];

extern "C" int stop;

static struct {
	const char* key;
	// For integral options this is a pointer to a char
	// for string values, this is a pointer to a 256-byte string
	// thus, assigning a string value to an integral type will cause overflow
	char* value;
	char  min, max;
} OPTIONS[] =
{ { "Audio", &audioEnabled, AUDIO_DISABLE, AUDIO_ENABLE },
  { "Volume", &volume, VOLUME_LOUDEST, VOLUME_LOW },
  { "FPS", &showFPSonScreen, FPS_HIDE, FPS_SHOW },
//  { "Debug", &printToScreen, DEBUG_HIDE, DEBUG_SHOW },
  { "ScreenMode", &screenMode, SCREENMODE_4x3, SCREENMODE_16x9_PILLARBOX },
  { "VideoMode", &videoMode, VIDEOMODE_AUTO, VIDEOMODE_PROGRESSIVE },
  { "FileSortMode", &fileSortMode, FILESORT_DIRS_MIXED, FILESORT_DIRS_FIRST },
  { "Core", &dynacore, DYNACORE_DYNAREC, DYNACORE_INTERPRETER },
  { "NativeDevice", &nativeSaveDevice, NATIVESAVEDEVICE_SD, NATIVESAVEDEVICE_CARDB },
  { "StatesDevice", &saveStateDevice, SAVESTATEDEVICE_SD, SAVESTATEDEVICE_USB },
  { "AutoSave", &autoSave, AUTOSAVE_DISABLE, AUTOSAVE_ENABLE },
  { "BiosDevice", &biosDevice, BIOSDEVICE_HLE, BIOSDEVICE_USB },
  { "BootThruBios", &LoadCdBios, BOOTTHRUBIOS_NO, BOOTTHRUBIOS_YES },
  { "LimitFrames", &frameLimit, FRAMELIMIT_NONE, FRAMELIMIT_AUTO },
  { "SkipFrames", &frameSkip, FRAMESKIP_DISABLE, FRAMESKIP_ENABLE },
  { "PadAutoAssign", &padAutoAssign, PADAUTOASSIGN_MANUAL, PADAUTOASSIGN_AUTOMATIC },
  { "PadType1", &padType[0], PADTYPE_NONE, PADTYPE_WII },
  { "PadType2", &padType[1], PADTYPE_NONE, PADTYPE_WII },
  { "PadAssign1", &padAssign[0], PADASSIGN_INPUT0, PADASSIGN_INPUT3 },
  { "PadAssign2", &padAssign[1], PADASSIGN_INPUT0, PADASSIGN_INPUT3 },
  { "RumbleEnabled", &rumbleEnabled, RUMBLE_DISABLE, RUMBLE_ENABLE },
  { "LoadButtonSlot", &loadButtonSlot, LOADBUTTON_SLOT0, LOADBUTTON_DEFAULT },
  { "ControllerType", &controllerType, CONTROLLERTYPE_STANDARD, CONTROLLERTYPE_LIGHTGUN },
//  { "NumberMultitaps", &numMultitaps, MULTITAPS_NONE, MULTITAPS_TWO },
  { "smbusername", smbUserName, CONFIG_STRING_TYPE, CONFIG_STRING_TYPE },
  { "smbpassword", smbPassWord, CONFIG_STRING_TYPE, CONFIG_STRING_TYPE },
  { "smbsharename", smbShareName, CONFIG_STRING_TYPE, CONFIG_STRING_TYPE },
  { "smbipaddr", smbIpAddr, CONFIG_STRING_TYPE, CONFIG_STRING_TYPE }
};
void handleConfigPair(char* kv);
void readConfig(FILE* f);
void writeConfig(FILE* f);
int checkBiosExists(int testDevice);

void loadSettings(int argc, char *argv[])
{
	// Default Settings
	audioEnabled     = 1; // Audio
	volume           = VOLUME_MEDIUM;
#ifdef RELEASE
	showFPSonScreen  = 0; // Don't show FPS on Screen
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
	videoMode		 = VIDEOMODE_AUTO;
	fileSortMode	 = FILESORT_DIRS_FIRST;
	padAutoAssign	 = PADAUTOASSIGN_AUTOMATIC;
	padType[0]		 = PADTYPE_NONE;
	padType[1]		 = PADTYPE_NONE;
	padAssign[0]	 = PADASSIGN_INPUT0;
	padAssign[1]	 = PADASSIGN_INPUT1;
	rumbleEnabled	 = RUMBLE_ENABLE;
	loadButtonSlot	 = LOADBUTTON_DEFAULT;
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
	Config.Cdda = 0; //CDDA enabled
	spu_config.iVolume = 1024 - (volume * 192); //Volume="medium" in PEOPSspu
	spu_config.iUseThread = 0;	// Don't enable, broken on GC/Wii
	spu_config.iUseFixedUpdates = 1;
	spu_config.iUseReverb = 0;
	spu_config.iUseInterpolation = 1;
	spu_config.iXAPitch = 0;
	spu_config.iTempo = 0;
	Config.PsxAuto = 1; //Autodetect
	Config.cycle_multiplier = CYCLE_MULT_DEFAULT;
	LoadCdBios = BOOTTHRUBIOS_NO;
	
	strcpy(Config.PluginsDir, "plugins");
	strcpy(Config.Gpu, "builtin_gpu");
	strcpy(Config.Spu, "builtin_spu");
	strcpy(Config.Cdr, "builtin_cdr");
	strcpy(Config.Pad1, "builtin_pad");
	strcpy(Config.Pad2, "builtin_pad2");
	strcpy(Config.Net, "Disabled");

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
			f = fopen( "usb:/wiisx/controlG.cfg", "r" );  //attempt to open file
			if(f) {
				load_configurations(f, &controller_GC);					//read in GC controller mappings
				fclose(f);
			}
#ifdef HW_RVL
			f = fopen( "usb:/wiisx/controlC.cfg", "r" );  //attempt to open file
			if(f) {
				load_configurations(f, &controller_Classic);			//read in Classic controller mappings
				fclose(f);
			}
			f = fopen( "usb:/wiisx/controlN.cfg", "r" );  //attempt to open file
			if(f) {
				load_configurations(f, &controller_WiimoteNunchuk);		//read in WM+NC controller mappings
				fclose(f);
			}
			f = fopen( "usb:/wiisx/controlW.cfg", "r" );  //attempt to open file
			if(f) {
				load_configurations(f, &controller_Wiimote);			//read in Wiimote controller mappings
				fclose(f);
			}
#endif //HW_RVL
		}
	}
	else /*if((argv[0][0]=='s') || (argv[0][0]=='/'))*/
#endif //HW_RVL
	{ //assume SD
		configFile_file = &saveDir_libfat_Default;
		if(configFile_init(configFile_file)) {                //only if device initialized ok
			FILE* f = fopen( "sd:/wiisx/settings.cfg", "r" );  //attempt to open file
			if(f) {        //open ok, read it
				readConfig(f);
				fclose(f);
			}
			f = fopen( "sd:/wiisx/controlG.cfg", "r" );  //attempt to open file
			if(f) {
				load_configurations(f, &controller_GC);					//read in GC controller mappings
				fclose(f);
			}
#ifdef HW_RVL
			f = fopen( "sd:/wiisx/controlC.cfg", "r" );  //attempt to open file
			if(f) {
				load_configurations(f, &controller_Classic);			//read in Classic controller mappings
				fclose(f);
			}
			f = fopen( "sd:/wiisx/controlN.cfg", "r" );  //attempt to open file
			if(f) {
				load_configurations(f, &controller_WiimoteNunchuk);		//read in WM+NC controller mappings
				fclose(f);
			}
			f = fopen( "sd:/wiisx/controlW.cfg", "r" );  //attempt to open file
			if(f) {
				load_configurations(f, &controller_Wiimote);			//read in Wiimote controller mappings
				fclose(f);
			}
#endif //HW_RVL
		}
	}
#ifdef HW_RVL
	// Handle options passed in through arguments
	int i;
	for(i=1; i<argc; ++i){
		handleConfigPair(argv[i]);
	}
#endif

	//Test for Bios file
	if(biosDevice != BIOSDEVICE_HLE) {
		if(checkBiosExists((int)biosDevice) == FILE_BROWSER_ERROR_NO_FILE) {
			biosDevice = BIOSDEVICE_HLE;
		}
		else {
			strcpy(Config.BiosDir, &((biosDevice == BIOSDEVICE_SD) ? &biosDir_libfat_Default : &biosDir_libfat_USB)->name[0]);
			strcpy(Config.Bios, "/SCPH1001.BIN");
		}
	}

	//Synch settings with Config
	Config.Cpu=dynacore;
}

void ScanPADSandReset(u32 dummy) 
{
//	PAD_ScanPads();
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

void video_mode_init(GXRModeObj *videomode, u32 *fb1, u32 *fb2)
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
	  PLUGIN_SLOT_5 };
}

int main(int argc, char *argv[]) 
{
	/* INITIALIZE */
#ifdef HW_RVL
	L2Enhance();
	VM_Init(1024*1024, 256*1024); // whatever for now, we're not really using this for anything other than mmap on Wii.
	DI_UseCache(false);
	if (!__di_check_ahbprot()) {
		s32 preferred = IOS_GetPreferredVersion();
		if (preferred == 58 || preferred == 61)
			IOS_ReloadIOS(preferred);
		else DI_LoadDVDX(true);
	}
	
	DI_Init();    // first
#else
	VM_Init(ARAM_SIZE, MRAM_BACKING);		// Setup Virtual Memory with the entire ARAM
#endif
	
	loadSettings(argc, argv);
	MenuContext *menu = new MenuContext(vmode);
	VIDEO_SetPostRetraceCallback (ScanPADSandReset);

#ifndef WII
	DVD_Init();
#endif

#ifdef DEBUGON
	//DEBUG_Init(GDBSTUB_DEVICE_TCP,GDBSTUB_DEF_TCPPORT); //Default port is 2828
	DEBUG_Init(GDBSTUB_DEVICE_USB, 1);
	_break();
#else
#ifdef PRINTGECKO
	CON_EnableGecko(EXI_CHANNEL_1, TRUE);
#endif
#endif

	control_info_init(); //Perform controller auto assignment at least once at startup.

	// Start up AESND (inited here because its used in SPU and CD)
	AESND_Init();

#ifdef HW_RVL
	// Initialize the network if the user has specified something in their SMB settings
	if(strlen(&smbShareName[0]) && strlen(&smbIpAddr[0])) {
	  init_network_thread();
  }
#endif
	
	while (menu->isRunning()) {}
	
	// Shut down AESND
	AESND_Reset();

	delete menu;

	return 0;
}

// loadISO loads an ISO file as current media to read from.
int loadISOSwap(fileBrowser_file* file) {
  
  // Refresh file pointers
	memset(&isoFile, 0, sizeof(fileBrowser_file));
	memset(&cddaFile, 0, sizeof(fileBrowser_file));
	memset(&subFile, 0, sizeof(fileBrowser_file));
	
	memcpy(&isoFile, file, sizeof(fileBrowser_file) );
	
	CDR_close();
	SetIsoFile(&file->name[0]);
	//might need to insert code here to trigger a lid open/close interrupt
	if(CDR_open() < 0)
		return -1;
	CheckCdrom();
	LoadCdrom();
	return 0;
}


// loadISO loads an ISO, resets the system and loads the save.
int loadISO(fileBrowser_file* file) 
{
	// Refresh file pointers
	memset(&isoFile, 0, sizeof(fileBrowser_file));
	memset(&cddaFile, 0, sizeof(fileBrowser_file));
	memset(&subFile, 0, sizeof(fileBrowser_file));
	
	memcpy(&isoFile, file, sizeof(fileBrowser_file) );
	
	if(hasLoadedISO) {
		SysClose();	
		hasLoadedISO = FALSE;
	}
	SetIsoFile(&file->name[0]);
		
	if(SysInit() < 0)
		return -1;
	hasLoadedISO = TRUE;
	SysReset();
	
	char *tempStr = &file->name[0];
	if((strstr(tempStr,".EXE")!=NULL) || (strstr(tempStr,".exe")!=NULL)) {
		//TODO
		//Load(file);
	}
	else {
		CheckCdrom();
		LoadCdrom();
	}
	
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
		saveFile_init(saveFile_dir);
		/*
		 * TODO: Implement LoadMcd properly using VFS
		LoadMcd(1,saveFile_dir);
		LoadMcd(2,saveFile_dir);
		*/
		saveFile_deinit(saveFile_dir);
		
		switch (nativeSaveDevice)
		{
		case NATIVESAVEDEVICE_SD:
			autoSaveLoaded = NATIVESAVEDEVICE_SD;
			break;
		case NATIVESAVEDEVICE_USB:
			autoSaveLoaded = NATIVESAVEDEVICE_USB;
			break;
		case NATIVESAVEDEVICE_CARDA:
			autoSaveLoaded = NATIVESAVEDEVICE_CARDA;
			break;
		case NATIVESAVEDEVICE_CARDB:
			autoSaveLoaded = NATIVESAVEDEVICE_CARDB;
			break;
		}
	}	
	
	return 0;
}

void setOption(char* key, char* valuePointer){
	bool isString = valuePointer[0] == '"';
	char value = 0;
	
	if(isString) {
		char* p = valuePointer++;
		while(*++p != '"');
		*p = 0;
	} else
		value = atoi(valuePointer);
	
	for(unsigned int i=0; i<sizeof(OPTIONS)/sizeof(OPTIONS[0]); i++){
		if(!strcmp(OPTIONS[i].key, key)){
			if(isString) {
				if(OPTIONS[i].max == CONFIG_STRING_TYPE)
					strncpy(OPTIONS[i].value, valuePointer,
					        CONFIG_STRING_SIZE-1);
			} else if(value >= OPTIONS[i].min && value <= OPTIONS[i].max)
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

	setOption(kv, vs);
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
		if(OPTIONS[i].max == CONFIG_STRING_TYPE)
			fprintf(f, "%s = \"%s\"\n", OPTIONS[i].key, OPTIONS[i].value);
		else
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
	if(LoadPlugins() < 0) {
		SysPrintf("LoadPlugins() failed!\r\n");
	}
	if(OpenPlugins() < 0){
		SysPrintf("LoadPlugins() failed!\r\n");
		return -1;
	}
  
	//Init biosFile pointers and stuff
	if(biosDevice != BIOSDEVICE_HLE) {
		biosFile_dir = (biosDevice == BIOSDEVICE_SD) ? &biosDir_libfat_Default : &biosDir_libfat_USB;
		biosFile_readFile  = fileBrowser_libfat_readFile;
		biosFile_open      = fileBrowser_libfat_open;
		biosFile_init      = fileBrowser_libfat_init;
		biosFile_deinit    = fileBrowser_libfat_deinit;
		if(biosFile) {
    		free(biosFile);
	 	}
		biosFile = (fileBrowser_file*)memalign(32,sizeof(fileBrowser_file));
		memcpy(biosFile,biosFile_dir,sizeof(fileBrowser_file));
		strcat(biosFile->name, "/SCPH1001.BIN");
		biosFile_init(biosFile);  //initialize the bios device (it might not be the same as ISO device)
		Config.HLE = 0;
	} else {
		Config.HLE = 1;
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

void print_gecko(const char *fmt, ...) {
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	//if (Config.PsxOut) printf ("%s", msg);
	DEBUG_print(msg, DBG_USBGECKO);
}

void SysPrintf(const char *fmt, ...) 
{
#if 0
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

int framesdone = 0;
void SysUpdate() 
{
	framesdone++;
}

void SysRunGui() {}
void SysMessage(const char *fmt, ...) {}
void SysCloseLibrary(void *lib) {}
const char *SysLibError() {	return NULL; }

void pl_frame_limit(void)
{
}


/* TODO: Should be populated properly */
int in_type[8] = {
   PSE_PAD_TYPE_NONE, PSE_PAD_TYPE_NONE,
   PSE_PAD_TYPE_NONE, PSE_PAD_TYPE_NONE,
   PSE_PAD_TYPE_NONE, PSE_PAD_TYPE_NONE,
   PSE_PAD_TYPE_NONE, PSE_PAD_TYPE_NONE
};

static s8 psxM_buf[0x220000] __attribute__((aligned(4096)));
static s8 psxR_buf[0x80000] __attribute__((aligned(4096)));

static s8 code_buf [0x400000] __attribute__((aligned(32))); // 4 MiB code buffer for Lightrec
void * code_buffer = code_buf;

int lightrec_init_mmap(void)
{
	psxP = &psxM_buf[0x200000];

	if (lightrec_mmap(psxM_buf, 0x0, 0x200000)
	    || lightrec_mmap(psxM_buf, 0x200000, 0x200000)
	    || lightrec_mmap(psxM_buf, 0x400000, 0x200000)
	    || lightrec_mmap(psxM_buf, 0x600000, 0x200000)) {
		SysMessage(_("Error mapping RAM"));
		return -1;
	}

	psxM = (s8 *) 0x0;

	if (lightrec_mmap(psxR_buf, 0x1fc00000, 0x80000)) {
		SysMessage(_("Error mapping BIOS"));
		return -1;
	}

	psxR = (s8 *) 0x1fc00000;

	if (lightrec_mmap(psxM_buf + 0x210000, 0x1f800000, 0x10000)) {
		SysMessage(_("Error mapping I/O"));
		return -1;
	}

	psxH = (s8 *) 0x1f800000;

	return 0;
}

void lightrec_free_mmap(void)
{
}

} //extern "C"
