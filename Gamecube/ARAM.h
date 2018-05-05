/**
 * WiiSX - ARAM.h
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 2009 - 2018 emu_kidid
 * 
 * This is the ARAM manager
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/


#ifndef ARAM_H
#define ARAM_H

#ifdef MEM2_H
#error MEM2 and ARAM included in the same code, FAIL!
#endif

#define MB (1024*1024)
#define KB (1024)

#define MRAM_BACKING	(2*MB)			// Use 2MB to page our 16MB

#define ARAM_RESERVED	(64*KB)			// Reserved for DSP/AESND/etc

#define ARAM_VM_BASE	(0x7F000000)	// Map ARAM to here
#define ARAM_START		(ARAM_RESERVED + ARAM_VM_BASE) 
#define ARAM_SIZE		((16*MB) - ARAM_RESERVED)	// ARAM is ~16MB

// We want 128KB for our MEMCARD 1
#define MCD1_SIZE     (128*KB)
#define MCD1_LO       (ARAM_START)
#define MCD1_HI       (MCD1_LO + MCD1_SIZE)

// We want 128KB for our MEMCARD 2
#define MCD2_SIZE     (128*KB)
#define MCD2_LO       (MCD1_HI)
#define MCD2_HI       (MCD2_LO + MCD2_SIZE)

// 256KB for SIO Dongle data
#define SIODONGLE_SIZE (256*KB)
#define SIODONGLE_LO   (MCD2_HI)
#define SIODONGLE_HI   (SIODONGLE_LO + SIODONGLE_SIZE)

#define TEXCACHE_SIZE (10*MB)
#define TEXCACHE_LO   (SIODONGLE_HI)
#define TEXCACHE_HI   (TEXCACHE_LO + TEXCACHE_SIZE)

#endif

