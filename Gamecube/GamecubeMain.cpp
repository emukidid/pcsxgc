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

#ifdef __SWITCH__
#include <switch.h>
#else
#include <gccore.h>
#include <fat.h>
#include <aesndlib.h>
#endif
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#ifdef DEBUGON
# include <debug.h>
#endif
#include "../PsxCommon.h"
#include "wiiSXconfig.h"

#ifdef __SWITCH__
extern "C" {
#include "DEBUG.h"
#include "fileBrowser/fileBrowser.h"
#include "fileBrowser/fileBrowser-libfat.h"
#include "gc_input/controller.h"
void go(void);
}
#else
#include "menu/MenuContext.h"
extern "C" {
#include "DEBUG.h"
#include "fileBrowser/fileBrowser.h"
#include "fileBrowser/fileBrowser-libfat.h"
#include "fileBrowser/fileBrowser-CARD.h"
#include "fileBrowser/fileBrowser-SMB.h"
#include "gc_input/controller.h"
}
#endif

#ifdef WII
unsigned int MALLOC_MEM2 = 0;
extern "C" {
#include <di/di.h>
extern u32 __di_check_ahbprot(void);
}
#endif //WII

//-----------------------------------------------------------------------------
// nxlink support
//-----------------------------------------------------------------------------

#ifndef ENABLE_NXLINK
#define TRACE(fmt,...) ((void)0)
#else
#include <unistd.h>
#define TRACE(fmt,...) printf("%s: " fmt "\n", __PRETTY_FUNCTION__, ## __VA_ARGS__)

static int s_nxlinkSock = -1;

static void initNxLink()
{
    if (R_FAILED(socketInitializeDefault()))
        return;

    s_nxlinkSock = nxlinkStdio();
    if (s_nxlinkSock >= 0)
        TRACE("printf output now goes to nxlink server");
    else
        socketExit();
}

static void deinitNxLink()
{
    if (s_nxlinkSock >= 0)
    {
        close(s_nxlinkSock);
        socketExit();
        s_nxlinkSock = -1;
    }
}

extern "C" void userAppInit()
{
    initNxLink();
}

extern "C" void userAppExit()
{
    deinitNxLink();
}

#endif

/* function prototypes */
extern "C" {
int SysInit();
void SysReset();
void SysClose();
void SysPrintf(const char *fmt, ...);
void *SysLoadLibrary(const char *lib);
void *SysLoadSym(void *lib, const char *sym);
const char *SysLibError();
void SysCloseLibrary(void *lib);
void SysUpdate();
void SysRunGui();
void SysMessage(const char *fmt, ...);
}

#ifdef __SWITCH__
fileBrowser_file isoFile;  //the ISO file
fileBrowser_file cddaFile; //the CDDA file
fileBrowser_file subFile;  //the SUB file
fileBrowser_file *biosFile = NULL;  //BIOS file
#else
u32* xfb[2] = { NULL, NULL };	/*** Framebuffers ***/
int whichfb = 0;        /*** Frame buffer toggle ***/
GXRModeObj *vmode;				/*** Graphics Mode Object ***/
#define DEFAULT_FIFO_SIZE ( 256 * 1024 )
fileBrowser_file isoFile;  //the ISO file
fileBrowser_file cddaFile; //the CDDA file
fileBrowser_file subFile;  //the SUB file
fileBrowser_file *biosFile = NULL;  //BIOS file
#endif
bool hasLoadedISO = FALSE;

#if defined (CPU_LOG) || defined(DMA_LOG) || defined(CDR_LOG) || defined(HW_LOG) || \
	defined(BIOS_LOG) || defined(GTE_LOG) || defined(PAD_LOG)
FILE *emuLog;
#endif

PcsxConfig Config;
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
#ifdef HW_RVL
char shutdown = 0;
#endif
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

int stop = 0;

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
#ifdef __SWITCH__
  { "Core", &dynacore, DYNACORE_INTERPRETER, DYNACORE_INTERPRETER },
  { "NativeDevice", &nativeSaveDevice, NATIVESAVEDEVICE_SWITCH, NATIVESAVEDEVICE_SWITCH },
  { "StatesDevice", &saveStateDevice, SAVESTATEDEVICE_SWITCH, SAVESTATEDEVICE_SWITCH },
  { "BiosDevice", &biosDevice, BIOSDEVICE_HLE, BIOSDEVICE_SWITCH },
#else
  { "Core", &dynacore, DYNACORE_DYNAREC, DYNACORE_INTERPRETER },
  { "NativeDevice", &nativeSaveDevice, NATIVESAVEDEVICE_SD, NATIVESAVEDEVICE_CARDB },
  { "StatesDevice", &saveStateDevice, SAVESTATEDEVICE_SD, SAVESTATEDEVICE_USB },
  { "BiosDevice", &biosDevice, BIOSDEVICE_HLE, BIOSDEVICE_USB },
#endif
  { "AutoSave", &autoSave, AUTOSAVE_DISABLE, AUTOSAVE_ENABLE },
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
  { "ControllerType", &controllerType, CONTROLLERTYPE_STANDARD, CONTROLLERTYPE_ANALOG },
//  { "NumberMultitaps", &numMultitaps, MULTITAPS_NONE, MULTITAPS_TWO },
  { "smbusername", smbUserName, CONFIG_STRING_TYPE, CONFIG_STRING_TYPE },
  { "smbpassword", smbPassWord, CONFIG_STRING_TYPE, CONFIG_STRING_TYPE },
  { "smbsharename", smbShareName, CONFIG_STRING_TYPE, CONFIG_STRING_TYPE },
  { "smbipaddr", smbIpAddr, CONFIG_STRING_TYPE, CONFIG_STRING_TYPE }
};
void handleConfigPair(char* kv);
void readConfig(FILE* f);
void writeConfig(FILE* f);
#ifndef __SWITCH__
int checkBiosExists(int testDevice);
#else
int checkBiosExists(int testDevice) 
{
	fileBrowser_file testFile;
	memset(&testFile, 0, sizeof(fileBrowser_file));
	
	biosFile_dir = &biosDir_SWITCH_Default;
	sprintf(&testFile.name[0], "%s/SCPH1001.BIN", &biosFile_dir->name[0]);
	biosFile_readFile  = fileBrowser_libfat_readFile;
	biosFile_open      = fileBrowser_libfat_open;
	biosFile_init      = fileBrowser_libfat_init;
	biosFile_deinit    = fileBrowser_libfat_deinit;
	biosFile_init(&testFile);  //initialize the bios device (it might not be the same as ISO device)
	return biosFile_open(&testFile);
}
#endif


void loadSettings(int argc, char *argv[])
{
	// Default Settings
	audioEnabled     = 1; // Audio
	volume           = VOLUME_MEDIUM;
#ifdef RELEASE
	showFPSonScreen  = 1; // Don't show FPS on Screen
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
	dynacore         = 1; // Interpreter
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
	//menuActive = 1;

	//PCSX-specific defaults
	memset(&Config, 0, sizeof(PcsxConfig));
#ifdef __SWITCH__
	Config.Cpu=CPU_INTERPRETER;
#else
	Config.Cpu=dynacore;		//Dynarec core
#endif
	strcpy(Config.Net,"Disabled");
	Config.PsxOut = 1;
	Config.HLE = 1;
	Config.Xa = 0;  //XA enabled
	Config.Cdda = 1; //CDDA disabled
	iVolume = volume; //Volume="medium" in PEOPSspu
	Config.PsxAuto = 1; //Autodetect
	LoadCdBios = BOOTTHRUBIOS_NO;

#ifdef __SWITCH__
#include <switch.h>
#else
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
#endif

	//Test for Bios file
	if(checkBiosExists((int)biosDevice) == FILE_BROWSER_ERROR_NO_FILE)
		biosDevice = BIOSDEVICE_HLE;
	else
		biosDevice = BIOSDEVICE_SWITCH;
	//Synch settings with Config
#ifdef __SWITCH__
	Config.Cpu=CPU_INTERPRETER;
#else
	Config.Cpu=dynacore;
#endif
	iVolume = volume;
}

#ifdef __SWITCH__
#include <switch.h>
#else
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

void video_mode_init(GXRModeObj *videomode,unsigned int *fb1, unsigned int *fb2)
{
	vmode = videomode;
	xfb[0] = fb1;
	xfb[1] = fb2;
}
#endif

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
		Load(file);
	}
	else {
		CheckCdrom();
		LoadCdrom();
	}

#ifdef __SWITCH__
	nativeSaveDevice = NATIVESAVEDEVICE_SWITCH;
#endif
	if(autoSave==AUTOSAVE_ENABLE) {
		switch (nativeSaveDevice)
		{
#ifdef __SWITCH__
		case NATIVESAVEDEVICE_SWITCH:
			// Adjust saveFile pointers
			saveFile_dir = &saveDir_SWITCH_Default;
			saveFile_readFile  = fileBrowser_libfat_readFile;
			saveFile_writeFile = fileBrowser_libfat_writeFile;
			saveFile_init      = fileBrowser_libfat_init;
			saveFile_deinit    = fileBrowser_libfat_deinit;
			break;
#else
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
#endif
		}
		// Try loading everything
		int result = 0;
		saveFile_init(saveFile_dir);
		result += LoadMcd(1,saveFile_dir);
		result += LoadMcd(2,saveFile_dir);
		saveFile_deinit(saveFile_dir);
		
		switch (nativeSaveDevice)
		{
#ifdef __SWITCH__
		case NATIVESAVEDEVICE_SWITCH:
#endif
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

static char* filenameFromAbsPath(char* absPath)
{
	char* filename = absPath;
	while( *filename ) ++filename;
	while( filename != absPath && (*(filename-1) != '\\' && *(filename-1) != '/'))
		--filename;
	return filename;
}

#ifdef __SWITCH__
#include <EGL/egl.h>    // EGL library
#include <EGL/eglext.h> // EGL extensions
#include <glad/glad.h>  // glad library (OpenGL loader)

// GLM headers
#define GLM_FORCE_PURE
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
//-----------------------------------------------------------------------------
// EGL initialization
//-----------------------------------------------------------------------------

EGLDisplay s_display;
EGLContext s_context;
EGLSurface s_surface;

static GLuint s_program;
static GLuint s_vao, s_vbo;
GLuint s_tex;

static const char* const vertexShaderSource = R"text(
    #version 150

	in vec4 position;
	in vec2 texture_coord;
	out vec2 texture_coord_from_vshader;

	void main() {
		gl_Position = position;
		texture_coord_from_vshader = texture_coord;
	}
)text";

static const char* const fragmentShaderSource = R"text(
    #version 150

	in vec2 texture_coord_from_vshader;
	out vec4 out_color;

	uniform sampler2D texture_sampler;

	void main() {
		out_color = texture(texture_sampler, texture_coord_from_vshader);
	}
)text";

static GLuint createAndCompileShader(GLenum type, const char* source)
{
    GLint success;
    GLchar msg[512];

    GLuint handle = glCreateShader(type);
    if (!handle)
    {
        TRACE("%u: cannot create shader", type);
        return 0;
    }
    glShaderSource(handle, 1, &source, nullptr);
    glCompileShader(handle);
    glGetShaderiv(handle, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(handle, sizeof(msg), nullptr, msg);
        TRACE("%u: %s\n", type, msg);
        glDeleteShader(handle);
        return 0;
    }

    return handle;
}

static u64 s_startTicks;

static void sceneInit()
{
   // Use a Vertex Array Object
	glGenVertexArrays(1, &s_vao);
	glBindVertexArray(s_vao);

	float xcoord = 640.0/1280.0;
	float ycoord = 480.0/720.0;
	GLfloat vertices_position[8] = {
		-xcoord, -ycoord,
		xcoord, -ycoord,
		xcoord, ycoord,
		-xcoord, ycoord,		
	};

	GLfloat texture_coord[8] = {
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		0.0, 0.0,
	};

	GLuint indices[6] = {
		0, 1, 2,
		2, 3, 0
	};

	// Create a Vector Buffer Object that will store the vertices on video memory
	glGenBuffers(1, &s_vbo);

	// Allocate space for vertex positions and texture coordinates
	glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_position) + sizeof(texture_coord), NULL, GL_STATIC_DRAW);

	// Transfer the vertex positions:
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices_position), vertices_position);

	// Transfer the texture coordinates:
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices_position), sizeof(texture_coord), texture_coord);

	// Create an Element Array Buffer that will store the indices array:
	GLuint eab;
	glGenBuffers(1, &eab);

	// Transfer the data from indices to eab
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eab);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Create a texture
	GLuint s_tex;
	glGenTextures(1, &s_tex);

	// Specify that we work with a 2D texture
    glActiveTexture(GL_TEXTURE0); // activate the texture unit first before binding texture
	glBindTexture(GL_TEXTURE_2D, s_tex);

	// Load and compile the vertex and fragment shaders
	GLuint vertexShader = createAndCompileShader(GL_VERTEX_SHADER, vertexShaderSource);
	GLuint fragmentShader = createAndCompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

	// Attach the above shader to a program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	// Flag the shaders for deletion
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Link and use the program
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	// Get the location of the attributes that enters in the vertex shader
	GLint position_attribute = glGetAttribLocation(shaderProgram, "position");

	// Specify how the data for position can be accessed
	glVertexAttribPointer(position_attribute, 2, GL_FLOAT, GL_FALSE, 0, 0);

	// Enable the attribute
	glEnableVertexAttribArray(position_attribute);

	// Texture coord attribute
	GLint texture_coord_attribute = glGetAttribLocation(shaderProgram, "texture_coord");
	glVertexAttribPointer(texture_coord_attribute, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid *)sizeof(vertices_position));
	glEnableVertexAttribArray(texture_coord_attribute);
	
}

static bool initEgl()
{
    // Connect to the EGL default display
    s_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!s_display)
    {
        TRACE("Could not connect to display! error: %d", eglGetError());
        goto _fail0;
    }

    // Initialize the EGL display connection
    eglInitialize(s_display, nullptr, nullptr);

    // Select OpenGL (Core) as the desired graphics API
    if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE)
    {
        TRACE("Could not set API! error: %d", eglGetError());
        goto _fail1;
    }

    // Get an appropriate EGL framebuffer configuration
    EGLConfig config;
    EGLint numConfigs;
    static const EGLint framebufferAttributeList[] =
    {
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_NONE
    };
    eglChooseConfig(s_display, framebufferAttributeList, &config, 1, &numConfigs);
    if (numConfigs == 0)
    {
        TRACE("No config found! error: %d", eglGetError());
        goto _fail1;
    }

    // Create an EGL window surface
    s_surface = eglCreateWindowSurface(s_display, config, (char*)"", nullptr);
    if (!s_surface)
    {
        TRACE("Surface creation failed! error: %d", eglGetError());
        goto _fail1;
    }

    // Create an EGL rendering context
    static const EGLint contextAttributeList[] =
    {
        EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
        EGL_CONTEXT_MAJOR_VERSION_KHR, 4,
        EGL_CONTEXT_MINOR_VERSION_KHR, 3,
        EGL_NONE
    };
    s_context = eglCreateContext(s_display, config, EGL_NO_CONTEXT, contextAttributeList);
    if (!s_context)
    {
        TRACE("Context creation failed! error: %d", eglGetError());
        goto _fail2;
    }

    // Connect the context to the surface
    eglMakeCurrent(s_display, s_surface, s_surface, s_context);
    return true;

_fail2:
    eglDestroySurface(s_display, s_surface);
    s_surface = nullptr;
_fail1:
    eglTerminate(s_display);
    s_display = nullptr;
_fail0:
    return false;
}

static void deinitEgl()
{
    if (s_display)
    {
        if (s_context)
        {
            eglDestroyContext(s_display, s_context);
            s_context = nullptr;
        }
        if (s_surface)
        {
            eglDestroySurface(s_display, s_surface);
            s_surface = nullptr;
        }
        eglTerminate(s_display);
        s_display = nullptr;
    }
}

extern "C" {
void sceneRender()
{
    glClear(GL_COLOR_BUFFER_BIT);

	glBindVertexArray(s_vao);
	glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	// Swap front and back buffers
	eglSwapBuffers(s_display, s_surface);
}
};
#endif 


int main(int argc, char *argv[]) 
{
#ifdef __SWITCH__

    // Initialize EGL
    if (!initEgl())
        return EXIT_FAILURE;

    // Load OpenGL routines using glad
    gladLoadGL();
	sceneInit();
	hidInitialize();
	
#endif
	/* INITIALIZE */
#ifdef HW_RVL
	DI_UseCache(false);
	if (!__di_check_ahbprot()) {
		s32 preferred = IOS_GetPreferredVersion();
		if (preferred == 58 || preferred == 61)
			IOS_ReloadIOS(preferred);
		else DI_LoadDVDX(true);
	}
	
	DI_Init();    // first
#endif

	
	loadSettings(argc, argv);
#ifdef __SWITCH__
	control_info_init(); 
#else
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
#endif

#ifdef __SWITCH__
	// Change all the romFile pointers
	isoFile_topLevel = &topLevel_SWITCH_Default;
	isoFile_readDir  = fileBrowser_libfat_readDir;
	isoFile_open     = fileBrowser_libfat_open;
	isoFile_readFile = fileBrowser_libfatROM_readFile;
	isoFile_seekFile = fileBrowser_libfat_seekFile;
	isoFile_init     = fileBrowser_libfat_init;
	isoFile_deinit   = fileBrowser_libfatROM_deinit;
	// Make sure the romFile system is ready before we browse the filesystem
	isoFile_init( isoFile_topLevel );
	
	fileBrowser_file hardcodedIso =
	{ "/switch/wiisx/isos/game.bin", // file name
	  0, // sector
	  0, // offset
	  747435024, // size
	  0
	 };
	fileBrowser_file *dircontents = NULL;
	isoFile_readDir(isoFile_topLevel, &dircontents);
	
	int ret = loadISO( &hardcodedIso );
	
	if(!ret){	// If the read succeeded.
		SysPrintf("Loaded: %s\n", filenameFromAbsPath(hardcodedIso.name));

		char RomInfo[512] = "";
		char buffer [50];
		sprintf(buffer,"\nCD-ROM Label: %s\n",CdromLabel);
		strcat(RomInfo,buffer);
		sprintf(buffer,"CD-ROM ID: %s\n", CdromId);
		strcat(RomInfo,buffer);
		sprintf(buffer,"ISO Size: %d Mb\n",isoFile.size/1024/1024);
		strcat(RomInfo,buffer);
		sprintf(buffer,"Country: %s\n",(!Config.PsxType) ? "NTSC":"PAL");
		strcat(RomInfo,buffer);
		sprintf(buffer,"BIOS: %s\n",(Config.HLE==BIOS_USER_DEFINED) ? "USER DEFINED":"HLE");
		strcat(RomInfo,buffer);
		
		SysPrintf(RomInfo);
		go();
	}
	else		// If not.
		SysPrintf("Load %s Failed\n", hardcodedIso.name);

#endif
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
#ifdef __SWITCH__
	Config.Cpu=CPU_INTERPRETER;
#else
	Config.Cpu=dynacore;
#endif 
	psxInit();
	LoadPlugins();
	if(OpenPlugins() < 0)
		return -1;

	//Init biosFile pointers and stuff
	if(biosDevice != BIOSDEVICE_HLE) {
		SysPrintf("Loading BIOS\n");
#ifdef __SWITCH__
		biosFile_dir = &biosDir_SWITCH_Default;
#else
		biosFile_dir = (biosDevice == BIOSDEVICE_SD) ? &biosDir_libfat_Default : &biosDir_libfat_USB;
#endif
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

void SysPrintf(const char *fmt, ...) 
{

	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	//if (Config.PsxOut) printf ("%s", msg);
#ifdef __SWITCH__
	printf(msg);
#else
	DEBUG_print(msg, DBG_USBGECKO);
#endif
#if defined (CPU_LOG) || defined(DMA_LOG) || defined(CDR_LOG) || defined(HW_LOG) || \
	defined(BIOS_LOG) || defined(GTE_LOG) || defined(PAD_LOG)
	fprintf(emuLog, "%s", msg);
#endif

}

void *SysLoadLibrary(const char *lib) 
{
	s32 i;
	for(i=0; i<NUM_PLUGINS; i++)
		if((plugins[i].lib != NULL) && (!strcmp(lib, plugins[i].lib)))
			return (void*)i;
	return NULL;
}

void *SysLoadSym(void *lib, const char *sym) 
{
	PluginTable* plugin = plugins + (long)lib;
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
#ifdef PROFILE
	refresh_stat();
#endif
}

void SysRunGui() {}
void SysMessage(const char *fmt, ...) {}
void SysCloseLibrary(void *lib) {}
const char *SysLibError() {	return NULL; }

} //extern "C"
