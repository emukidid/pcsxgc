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
* Specficies which logs should be activated.
* Ryan TODO: These should ALL be definable with configure flags.
*/

#ifndef __CORE_DEBUG_H__
#define __CORE_DEBUG_H__

#include <switch.h>
#include <stdint.h>

extern char *disRNameCP0[];

char* disR3000AF(u32 code, u32 pc);

#if defined (CPU_LOG) || defined(DMA_LOG) || defined(CDR_LOG) || defined(HW_LOG) || \
	defined(BIOS_LOG) || defined(GTE_LOG) || defined(PAD_LOG)
extern FILE *emuLog;
#endif

//#define GTE_DUMP

#ifdef GTE_DUMP
FILE *gteLog;
#endif

//#define LOG_STDOUT

//#define PAD_LOG  printf
//#define GTE_LOG  printf
//#define CDR_LOG  printf("%8.8lx %8.8lx: ", psxRegs.pc, psxRegs.cycle); printf
//
//#define PSXHW_LOG   printf("%8.8lx %8.8lx: ", psxRegs.pc, psxRegs.cycle); printf
//#define PSXBIOS_LOG printf("%8.8lx %8.8lx: ", psxRegs.pc, psxRegs.cycle); printf
//#define PSXDMA_LOG  printf
//#define PSXMEM_LOG  printf("%8.8lx %8.8lx: ", psxRegs.pc, psxRegs.cycle); printf
//#define PSXCPU_LOG  printf

//#define CDRCMD_DEBUG

#if defined (PSXCPU_LOG) || defined(PSXDMA_LOG) || defined(CDR_LOG) || defined(PSXHW_LOG) || \
	defined(PSXBIOS_LOG) || defined(PSXMEM_LOG) || defined(GTE_LOG)    || defined(PAD_LOG)
#define EMU_LOG printf
#endif

#endif /* __DEBUG_H__ */
