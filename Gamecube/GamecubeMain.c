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
#include "PsxCommon.h"
#include "PlugCD.h"
#include "DEBUG.h"
#include "fileBrowser/fileBrowser.h"
#include "fileBrowser/fileBrowser-libfat.h"
#include "fileBrowser/fileBrowser-DVD.h"

/* function prototypes */
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

u32* xfb[2] = { NULL, NULL };	/*** Framebuffers ***/
int whichfb = 0;        /*** Frame buffer toggle ***/
GXRModeObj *vmode;				/*** Graphics Mode Object ***/
#define DEFAULT_FIFO_SIZE ( 256 * 1024 )
extern int controllerType;
unsigned int MALLOC_MEM2 = 0;
int stop = 0;
static int hasLoadedISO = 0;
fileBrowser_file *isoFile  = NULL;  //the ISO file
fileBrowser_file *memCardA = NULL;  //Slot 1 memory card
fileBrowser_file *memCardB = NULL;  //Slot 2 memory card
fileBrowser_file *biosFile = NULL;  //BIOS file
char wpadNeedScan = 1, padNeedScan = 1;

void ScanPADSandReset() {
  PAD_ScanPads(); //get rid of me when the menu has proper reliance on the NeedScan variables
  if(!((*(u32*)0xCC003000)>>16))
    stop=1;  //exit will be called
}

void loadISO() {
  if(hasLoadedISO) {
    //free stuff here
  }
  if (SysInit() == -1)
	{
		printf("SysInit() Error!\n");
		while(1);
	}
	OpenPlugins();

	SysReset();

  SysPrintf("CheckCdrom\r\n");
	CheckCdrom();
	LoadCdrom();
  hasLoadedISO = 1;

  SysPrintf("Execute\r\n");
}

static void Initialise (void){
  VIDEO_Init();
  PAD_Init();

#ifdef HW_RVL
  CONF_Init();
#endif

  whichfb = 0;        /*** Frame buffer toggle ***/
  vmode = VIDEO_GetPreferredMode(NULL);
#ifdef HW_RVL
  if(VIDEO_HaveComponentCable() && CONF_GetProgressiveScan())
    	vmode = &TVNtsc480Prog;
#else
  if(VIDEO_HaveComponentCable())
	  vmode = &TVNtsc480Prog;
#endif
  VIDEO_Configure (vmode);

  xfb[0] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode)); //assume PAL largest
  xfb[1] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));	//fixme for progressive?
  console_init (xfb[0], 20, 64, vmode->fbWidth, vmode->xfbHeight, vmode->fbWidth * 2);
  VIDEO_ClearFrameBuffer (vmode, xfb[0], COLOR_BLACK);
  VIDEO_ClearFrameBuffer (vmode, xfb[1], COLOR_BLACK);
  VIDEO_SetNextFramebuffer (xfb[0]);
  VIDEO_SetPreRetraceCallback (ScanPADSandReset);
  VIDEO_SetBlack (0);
  VIDEO_Flush ();
  VIDEO_WaitVSync ();        /*** Wait for VBL ***/
  if (vmode->viTVMode & VI_NON_INTERLACE)
    VIDEO_WaitVSync ();

  // setup the fifo and then init GX
  void *gp_fifo = NULL;
  gp_fifo = MEM_K0_TO_K1 (memalign (32, DEFAULT_FIFO_SIZE));
  memset (gp_fifo, 0, DEFAULT_FIFO_SIZE);

  GX_Init (gp_fifo, DEFAULT_FIFO_SIZE);

  // clears the bg to color and clears the z buffer
  GX_SetCopyClear ((GXColor){0,0,0,255}, 0x00000000);
  // init viewport
  GX_SetViewport (0, 0, vmode->fbWidth, vmode->efbHeight, 0, 1);
  // Set the correct y scaling for efb->xfb copy operation
  GX_SetDispCopyYScale ((f32) vmode->xfbHeight / (f32) vmode->efbHeight);
  GX_SetDispCopyDst (vmode->fbWidth, vmode->xfbHeight);
  GX_SetCullMode (GX_CULL_NONE);
  GX_CopyDisp (xfb[0], GX_TRUE); // This clears the efb
//  GX_CopyDisp (xfb[0], GX_TRUE); // This clears the xfb

  printf("\n\nInitialize - whichfb = %d; xfb = %x, %x\n",(u32)whichfb,(u32)xfb[0],(u32)xfb[1]);

}

// Plugin structure
#include "GamecubePlugins.h"
PluginTable plugins[] =
	{ PLUGIN_SLOT_0,
	  PLUGIN_SLOT_1,
	  PLUGIN_SLOT_2,
	  PLUGIN_SLOT_3,
	  PLUGIN_SLOT_4,
	  PLUGIN_SLOT_5,
	  PLUGIN_SLOT_6,
	  PLUGIN_SLOT_7
};


/* draw background */
void draw_splash(void)
{
   printf("Splash Screen");
}

long LoadCdBios;

int main(int argc, char *argv[]) {

	Initialise();
	//fatInitDefault();
  draw_splash();

  /* Configure pcsx */
	memset(&Config, 0, sizeof(PcsxConfig));

  printf("\n\nWiiSX\n\n");

  //move from this point on to the menu configuration page
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


	strcpy(Config.Net,"Disabled");
	Config.PsxOut = 1;
	Config.HLE = 1;
	Config.Xa = 0;  //XA enabled
	Config.Cdda = 1;
	Config.PsxAuto = 1; //Autodetect
  //end of "move to menu" section

  //call menu

  //setup based on iso load device
  if(1) { //SD
    // Deinit any existing romFile state
  	if(isoFile_deinit) isoFile_deinit( isoFile_topLevel );
  	// Change all the romFile pointers
  	isoFile_topLevel = &topLevel_libfat_Default;
  	isoFile_readDir  = fileBrowser_libfat_readDir;
  	isoFile_readFile = fileBrowser_libfatROM_readFile;
  	isoFile_seekFile = fileBrowser_libfat_seekFile;
  	isoFile_init     = fileBrowser_libfat_init;
  	isoFile_deinit   = fileBrowser_libfatROM_deinit;
  	// Make sure the romFile system is ready before we browse the filesystem
  	isoFile_deinit( isoFile_topLevel );
  	isoFile_init( isoFile_topLevel );
	}
	if(0) { //DVD
	  // Deinit any existing romFile state
  	if(isoFile_deinit) isoFile_deinit( isoFile_topLevel );
  	// Change all the romFile pointers
  	isoFile_topLevel = &topLevel_DVD;
  	isoFile_readDir  = fileBrowser_DVD_readDir;
  	isoFile_readFile = fileBrowser_DVD_readFile;
  	isoFile_seekFile = fileBrowser_DVD_seekFile;
  	isoFile_init     = fileBrowser_DVD_init;
  	isoFile_deinit   = fileBrowser_DVD_deinit;
  	// Make sure the romFile system is ready before we browse the filesystem
  	isoFile_init( isoFile_topLevel );
  }
  if(0) { //USB
#ifdef WII
  	// Deinit any existing romFile state
  	if(isoFile_deinit) isoFile_deinit( isoFile_topLevel );
  	// Change all the romFile pointers
  	isoFile_topLevel = &topLevel_libfat_USB;
  	isoFile_readDir  = fileBrowser_libfat_readDir;
  	isoFile_readFile = fileBrowser_libfatROM_readFile;
  	isoFile_seekFile = fileBrowser_libfat_seekFile;
  	isoFile_init     = fileBrowser_libfat_init;
  	isoFile_deinit   = fileBrowser_libfatROM_deinit;
  	// Make sure the romFile system is ready before we browse the filesystem
  	isoFile_deinit( isoFile_topLevel );
  	isoFile_init( isoFile_topLevel );
#endif
}
  //do the same as above but for memory card slots
  if(1) { //SD
   	saveFile_dir = &saveDir_libfat_Default;
  	saveFile_readFile  = fileBrowser_libfat_readFile;
  	saveFile_writeFile = fileBrowser_libfat_writeFile;
  	saveFile_init      = fileBrowser_libfat_init;
  	saveFile_deinit    = fileBrowser_libfat_deinit;
  	saveFile_init(saveFile_dir);
  } //fixme: code for the rest of the devices
  
  //do the same as above but for the bios(s)
  if(1) { //SD
   	biosFile_dir = &biosDir_libfat_Default;
  	biosFile_readFile  = fileBrowser_libfat_readFile;
  	biosFile_init      = fileBrowser_libfat_init;
  	biosFile_deinit    = fileBrowser_libfat_deinit;
  	biosFile_init(saveFile_dir);
  } //fixme: code for the rest of the devices
  
  biosFile = (fileBrowser_file*)memalign(32,sizeof(fileBrowser_file)); //also hardcoded for SD.
  memcpy(biosFile,&biosDir_libfat_Default,sizeof(fileBrowser_file));
  strcat(biosFile->name, "/SCPH1001.BIN");          // Use actual BIOS
  biosFile_init(biosFile);  //initialize this device
  
  //hardcoded for SD .. do this properly too in the menu
  memCardA = (fileBrowser_file*)memalign(32,sizeof(fileBrowser_file));
  memCardB = (fileBrowser_file*)memalign(32,sizeof(fileBrowser_file));
  memcpy(memCardA,&saveDir_libfat_Default,sizeof(fileBrowser_file));  
  strcat(memCardA->name,"/memcard1.mcd");
  memcpy(memCardB,&saveDir_libfat_Default,sizeof(fileBrowser_file));
  strcat(memCardB->name,"/memcard2.mcd");
  
  printf("Devices are alive .. Press A\n");
  while(!(PAD_ButtonsDown(0) & PAD_BUTTON_A));
  while((PAD_ButtonsDown(0) & PAD_BUTTON_A));
  
  //call a proper menu fileBrowser, not this garbage i'm doing here :p
  isoFile = textFileBrowser(isoFile_topLevel);  //the = isn't really needed here, but yeah.
  if(!isoFile) {
    printf("Failed to load ISO returning to hbc in 5 sec\n");
    sleep(5);
    exit(0);
  }

  //load ISO
  loadISO();          //equivalent of loadROM in wii64
  //Config.PsxOut = 0;  //disables all printfs
	psxCpu->Execute();  //equivalent to go(); in wii64

	return 0;
}

int SysInit() {
#if defined (CPU_LOG) || defined(DMA_LOG) || defined(CDR_LOG) || defined(HW_LOG) || \
	defined(BIOS_LOG) || defined(GTE_LOG) || defined(PAD_LOG)
	emuLog = fopen("/PSXISOS/emu.log", "w");
#endif

    SysPrintf("start SysInit()\r\n");

    SysPrintf("psxInit()\r\n");
	psxInit();

    SysPrintf("LoadPlugins()\r\n");
	if(LoadPlugins()==-1)
		SysPrintf("ErrorLoadingPlugins()\r\n");
    SysPrintf("LoadMcds()\r\n");
	LoadMcds(memCardA, memCardB);

	SysPrintf("end SysInit()\r\n");
	return 0;
}

void SysReset() {
    SysPrintf("start SysReset()\r\n");
	psxReset();
	SysPrintf("end SysReset()\r\n");
}

void SysClose() {
	psxShutdown();
	ReleasePlugins();

	if (emuLog != NULL) fclose(emuLog);
}

void SysPrintf(char *fmt, ...) {
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

void *SysLoadLibrary(char *lib) {
	int i;
	for(i=0; i<NUM_PLUGINS; i++)
		if((plugins[i].lib != NULL) && (!strcmp(lib, plugins[i].lib)))
			return (void*)i;
	return NULL;
}

void *SysLoadSym(void *lib, char *sym) {
	PluginTable* plugin = plugins + (int)lib;
	int i;
	for(i=0; i<plugin->numSyms; i++)
		if(plugin->syms[i].sym && !strcmp(sym, plugin->syms[i].sym))
			return plugin->syms[i].pntr;
	return NULL;
}

char *SysLibError() {
	return NULL;
}

void SysCloseLibrary(void *lib) {
//	dlclose(lib);
}

int framesdone = 0;
void SysUpdate() {
  wpadNeedScan = 1; //move me later ?
  padNeedScan = 1;  //move me later ?
	framesdone++;
	
//	PADhandleKey(PAD1_keypressed());
//	PADhandleKey(PAD2_keypressed());
}

void SysRunGui() {
//	RunGui();
}

void SysMessage(char *fmt, ...) {

}
