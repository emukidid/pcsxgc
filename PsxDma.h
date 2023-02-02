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

#ifndef __PSXDMA_H__
#define __PSXDMA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "PsxCommon.h"
#include "R3000A.h"
#include "PsxHw.h"
#include "PsxMem.h"

#define GPUDMA_INT(eCycle) { \
	psxCore.interrupt |= (1 << PSXINT_GPUDMA); \
	psxCore.intCycle[PSXINT_GPUDMA].cycle = eCycle; \
	psxCore.intCycle[PSXINT_GPUDMA].sCycle = psxCore.cycle; \
	new_dyna_set_event(PSXINT_GPUDMA, eCycle); \
}

#define SPUDMA_INT(eCycle) { \
	psxCore.interrupt |= (1 << PSXINT_SPUDMA); \
	psxCore.intCycle[PSXINT_SPUDMA].cycle = eCycle; \
	psxCore.intCycle[PSXINT_SPUDMA].sCycle = psxCore.cycle; \
	new_dyna_set_event(PSXINT_SPUDMA, eCycle); \
}

#define MDECOUTDMA_INT(eCycle) { \
	psxCore.interrupt |= (1 << PSXINT_MDECOUTDMA); \
	psxCore.intCycle[PSXINT_MDECOUTDMA].cycle = eCycle; \
	psxCore.intCycle[PSXINT_MDECOUTDMA].sCycle = psxCore.cycle; \
	new_dyna_set_event(PSXINT_MDECOUTDMA, eCycle); \
}

#define MDECINDMA_INT(eCycle) { \
	psxCore.interrupt |= (1 << PSXINT_MDECINDMA); \
	psxCore.intCycle[PSXINT_MDECINDMA].cycle = eCycle; \
	psxCore.intCycle[PSXINT_MDECINDMA].sCycle = psxCore.cycle; \
	new_dyna_set_event(PSXINT_MDECINDMA, eCycle); \
}

#define GPUOTCDMA_INT(eCycle) { \
	psxCore.interrupt |= (1 << PSXINT_GPUOTCDMA); \
	psxCore.intCycle[PSXINT_GPUOTCDMA].cycle = eCycle; \
	psxCore.intCycle[PSXINT_GPUOTCDMA].sCycle = psxCore.cycle; \
	new_dyna_set_event(PSXINT_GPUOTCDMA, eCycle); \
}

#define CDRDMA_INT(eCycle) { \
	psxCore.interrupt |= (1 << PSXINT_CDRDMA); \
	psxCore.intCycle[PSXINT_CDRDMA].cycle = eCycle; \
	psxCore.intCycle[PSXINT_CDRDMA].sCycle = psxCore.cycle; \
	new_dyna_set_event(PSXINT_CDRDMA, eCycle); \
}

void psxDma2(u32 madr, u32 bcr, u32 chcr);
void psxDma3(u32 madr, u32 bcr, u32 chcr);
void psxDma4(u32 madr, u32 bcr, u32 chcr);
void psxDma6(u32 madr, u32 bcr, u32 chcr);
void gpuInterrupt();
void spuInterrupt();
void gpuotcInterrupt();

#ifdef __cplusplus
}
#endif
#endif
