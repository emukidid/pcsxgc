/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
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

/* 
* This file contains common definitions and includes for all parts of the 
* emulator core.
*/

#ifndef __PSXCOMMON_H__
#define __PSXCOMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

// XXX: don't care but maybe fix it someday
#if defined(__GNUC__) && __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wformat-truncation"
#pragma GCC diagnostic ignored "-Wformat-overflow"
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
// devkitpro has uint32_t as long, unfortunately
#ifdef _3DS
#pragma GCC diagnostic ignored "-Wformat"
#endif

// System includes
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#ifndef __SWITCH__
#include <sys/types.h>
#endif
#include <assert.h>

// Define types
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef intptr_t sptr;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uintptr_t uptr;

typedef uint8_t boolean;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// Local includes
#include "System.h"

#ifndef _WIN32
#define strnicmp strncasecmp
#endif
#define __inline inline

// Enables NLS/internationalization if active
#ifdef ENABLE_NLS

#include <libintl.h>

#undef _
#define _(String) gettext(String)
#ifdef gettext_noop
#  define N_(String) gettext_noop (String)
#else
#  define N_(String) (String)
#endif

#else

#define _(msgid) msgid
#define N_(msgid) msgid

#endif

extern FILE *emuLog;
extern int Log;

void __Log(char *fmt, ...);

#define CYCLE_MULT_DEFAULT 175

typedef struct {
	char Gpu[MAXPATHLEN];
	char Spu[MAXPATHLEN];
	char Cdr[MAXPATHLEN];
	char Pad1[MAXPATHLEN];
	char Pad2[MAXPATHLEN];
	char Net[MAXPATHLEN];
	char Sio1[MAXPATHLEN];
	char Mcd1[MAXPATHLEN];
	char Mcd2[MAXPATHLEN];
	char Bios[MAXPATHLEN];
	char BiosDir[MAXPATHLEN];
	char PluginsDir[MAXPATHLEN];
	char PatchesDir[MAXPATHLEN];
	boolean Xa;
	boolean Mdec;
	boolean PsxAuto;
	boolean Cdda;
	boolean AsyncCD;
	boolean CHD_Precache; /* loads disk image into memory, works with CHD only. */
	boolean HLE;
	boolean SlowBoot;
	boolean Debug;
	boolean PsxOut;
	boolean UseNet;
	boolean icache_emulation;
	boolean DisableStalls;
	int GpuListWalking;
	int cycle_multiplier; // 100 for 1.0
	int cycle_multiplier_override;
	u8 Cpu; // CPU_DYNAREC or CPU_INTERPRETER
	u8 PsxType; // PSX_TYPE_NTSC or PSX_TYPE_PAL
	struct {
		boolean cdr_read_timing;
		boolean gpu_slow_list_walking;
	} hacks;
#ifdef _WIN32
	char Lang[256];
#endif
} PcsxConfig;

extern PcsxConfig Config;
extern boolean NetOpened;

struct PcsxSaveFuncs {
	void *(*open)(const char *name, const char *mode);
	int   (*read)(void *file, void *buf, u32 len);
	int   (*write)(void *file, const void *buf, u32 len);
	long  (*seek)(void *file, long offs, int whence);
	void  (*close)(void *file);
};
extern struct PcsxSaveFuncs SaveFuncs;

#define gzfreeze(ptr, size) { \
	if (Mode == 1) SaveFuncs.write(f, ptr, size); \
	if (Mode == 0) SaveFuncs.read(f, ptr, size); \
}

#define PSXCLK	33868800	/* 33.8688 MHz */

enum {
	PSX_TYPE_NTSC = 0,
	PSX_TYPE_PAL
}; // PSX Types

enum {
	CPU_DYNAREC = 0,
	CPU_INTERPRETER
}; // CPU Types

enum {
	BIOS_USER_DEFINED,
	BIOS_HLE
};	/* BIOS Types */
int EmuInit();
void EmuReset();
void EmuShutdown();
void EmuUpdate();

#ifdef __cplusplus
}
#endif

// add xjsxjs197 start
#define LOAD_SWAP16p(ptr) ({u16 __ret, *__ptr=(ptr); __asm__ ("lhbrx %0, 0, %1" : "=r" (__ret) : "r" (__ptr)); __ret;})
#define LOAD_SWAP32p(ptr) ({u32 __ret, *__ptr=(ptr); __asm__ ("lwbrx %0, 0, %1" : "=r" (__ret) : "r" (__ptr)); __ret;})
#define STORE_SWAP16p(ptr,val) ({u16 __val=(val), *__ptr=(ptr); __asm__ ("sthbrx %0, 0, %1" : : "r" (__val), "r" (__ptr) : "memory");})
#define STORE_SWAP32p(ptr,val) ({u32 __val=(val), *__ptr=(ptr); __asm__ ("stwbrx %0, 0, %1" : : "r" (__val), "r" (__ptr) : "memory");})
#define STORE_SWAP32p2(ptr,val) ({u32 __val=(val); __asm__ ("stwbrx %0, 0, %1" : : "r" (__val), "r" (ptr) : "memory");})

extern u32 tmpVal;
extern u32 tmpAddr[1];
extern u16 tmpVal16;
extern u16 tmpAddr16[1];
// add xjsxjs197 end

#endif
