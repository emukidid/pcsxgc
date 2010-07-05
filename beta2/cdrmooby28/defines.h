/************************************************************************

Copyright mooby 2002

CDRMooby2 defines.h
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/

#ifdef WINDOWS
#pragma warning(disable:4786)
#endif

#ifndef __DEFINES_H
#define __DEFINES_H

#if defined _WINDOWS || defined __CYGWIN32__
#include <windows.h>
#else
#define CALLBACK
#endif

#include <gccore.h>

// PSEMU DEFINES

// header version
#define _PPDK_HEADER_VERSION		1

#define PLUGIN_VERSION				1

// plugin type returned by PSEgetLibType (types can be merged if plugin is multi type!)
#define PSE_LT_CDR					1

// every function in DLL if completed sucessfully should return this value
#define PSE_ERR_SUCCESS				0
// undefined error but fatal one, that kills all functionality
#define PSE_ERR_FATAL				-1

// XXX_Init return values
// Those return values apply to all libraries

// initialization went OK
#define PSE_INIT_ERR_SUCCESS		0

// this driver is not configured
#define PSE_INIT_ERR_NOTCONFIGURED	-2

// this driver can not operate properly on this hardware or hardware is not detected
#define PSE_INIT_ERR_NOHARDWARE		-3


/*         CDR PlugIn          */

//	CDR_Test return values

// sucess, everything configured, and went OK.
#define PSE_CDR_ERR_SUCCESS 0

// ERRORS
#define PSE_CDR_ERR -40
// this driver is not configured
#define PSE_CDR_ERR_NOTCONFIGURED	PSE_CDR_ERR - 0
// if this driver is unable to read data from medium
#define PSE_CDR_ERR_NOREAD			PSE_CDR_ERR - 1

// WARNINGS
#define PSE_CDR_WARN 40
// if this driver emulates lame mode ie. can read only 2048 tracks and sector header is emulated
// this might happen to CDROMS that do not support RAW mode reading - surelly it will kill many games
#define PSE_CDR_WARN_LAMECD			PSE_CDR_WARN + 0






//  FPSE DEFINES

// Return values (should be applied to the entire code).
#define FPSE_OK         0
#define FPSE_ERR        -1
#define FPSE_WARN       1

typedef signed char INT8;
typedef short int   INT16;

typedef unsigned char      UINT8;
typedef unsigned short int UINT16;

#if !defined _WINDOWS && !defined __CYGWIN32__
typedef long int    INT32;
typedef unsigned long int  UINT32;
#endif

#define	INT64 long long

#define FPSE_CDROM  5


// New MDEC from GPU plugin.
typedef struct {
    int     (*MDEC0_Read)();
    int     (*MDEC0_Write)();
    int     (*MDEC1_Read)();
    int     (*MDEC1_Write)();
    int     (*MDEC0_DmaExec)();
    int     (*MDEC1_DmaExec)();
} MDEC_Export;

#if defined _WINDOWS || defined __CYGWIN32__
// Main Struct for initialization
typedef struct {
    UINT8        *SystemRam;   // Pointer to the PSX system ram
    UINT32        Flags;       // Flags to plugins
    UINT32       *IrqPulsePtr; // Pointer to interrupt pending reg
    MDEC_Export   MDecAltern;  // Use another MDEC engine
    int         (*ReadCfg)();  // Read an item from INI
    int         (*WriteCfg)(); // Write an item to INI
    void        (*FlushRec)(); // Tell where the RAM is changed
    HWND          HWnd;        // Window handle
    HINSTANCE     HInstance;
} FPSEWin32;

#endif

// cdr stat struct
struct CdrStat
{
 unsigned long Type;
 unsigned long Status;
 unsigned char Time[3]; // current playing time
};

// Main Struct for initialization
typedef struct {
    UINT8        *SystemRam;   // Pointer to the PSX system ram
    UINT32        Flags;       // Flags to plugins
    UINT32       *IrqPulsePtr; // Pointer to interrupt pending reg
    MDEC_Export   MDecAltern;  // Use another MDEC engine
    int         (*ReadCfg)();  // Read an item from INI
    int         (*WriteCfg)(); // Write an item to INI
    void        (*FlushRec)(); // Tell where the RAM is changed
} FPSElinux;

// Info about a plugin
typedef struct {
    UINT8   PlType;             // Plugin type: GPU, SPU or Controllers
    UINT8   VerLo;              // Version High
    UINT8   VerHi;              // Version Low
    UINT8   TestResult;         // Returns if it'll work or not
    char    Author[64];         // Name of the author
    char    Name[64];           // Name of plugin
    char    Description[1024];  // Description to put in the edit box
} FPSEAbout;



/* PS2 defines */

#if 0
#if defined(_WINDOWS)

typedef __int8  s8;
typedef __int16 s16;
typedef __int32 s32;
typedef __int64 s64;

typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

#else

typedef char s8;
typedef short s16;
typedef long s32;
typedef long long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;

#endif
#endif

typedef struct { // NOT bcd coded
	u8 minute;
	u8 second;
	u8 frame;
} cdvdLoc;

typedef struct {
	u8 strack;
	u8 etrack;
} cdvdTN;

#define PS2E_LT_CDVD 0x8


#endif  //__DEFINES_H
