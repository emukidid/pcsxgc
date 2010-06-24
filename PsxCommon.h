/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
 *   schultz.ryan@gmail.com, http://rschultz.ath.cx/code.php               *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/*
* This file contains common definitions and includes for all parts of the
* emulator core.
*/

#ifndef __PSXCOMMON_H__
#define __PSXCOMMON_H__

//#include "config.h"

/* System includes */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <zlib.h>
//#include <glib.h>

/* Define types */
/*typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef intptr_t sptr;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uintptr_t uptr;
*/
/* Local includes */
#include "System.h"
#include "CoreDebug.h"

/* Ryan TODO WTF is this? */
#if defined (__LINUX__) || defined (__MACOSX__) || defined(HW_RVL) || defined(HW_DOL)
#define strnicmp strncasecmp
#endif
#define __inline inline

/* Enables NLS/internationalization if active */
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

extern int Log;
void __Log(char *fmt, ...);

typedef struct {
	char Gpu[256];
	char Spu[256];
	char Cdr[256];
	char Pad1[256];
	char Pad2[256];
	char Net[256];
	char Mcd1[256];
	char Mcd2[256];
	char Bios[256];
	char BiosDir[MAXPATHLEN];
	char PluginsDir[MAXPATHLEN];
	long Xa;
	long Sio;
	long Mdec;
	long PsxAuto;
	long PsxType;		/* NTSC or PAL */
	long Cdda;
	long HLE;
	long Cpu;
	long Dbg;
	long PsxOut;
	long SpuIrq;
	long RCntFix;
	long UseNet;
	long VSyncWA;
} PcsxConfig;

extern PcsxConfig Config;

extern char LoadCdBios;
extern int StatesC;
extern int cdOpenCase;
extern int NetOpened;

#define gzfreeze(ptr, size) \
	if (Mode == 1) gzwrite(f, ptr, size); \
	if (Mode == 0) gzread(f, ptr, size);

#define gzfreezel(ptr) gzfreeze(ptr, sizeof(ptr))

//#define BIAS	4
#define BIAS	2
#define PSXCLK	33868800	/* 33.8688 Mhz */

enum {
	BIOS_USER_DEFINED,
	BIOS_HLE
};	/* BIOS Types */

enum {
	PSX_TYPE_NTSC,
	PSX_TYPE_PAL
};	/* PSX Type */


#endif /* __PSXCOMMON_H__ */
