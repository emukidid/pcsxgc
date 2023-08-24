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
* R3000A CPU functions.
*/

#include "r3000a.h"
#include "cdrom.h"
#include "mdec.h"
#include "gte.h"
#include "psxinterpreter.h"
#include "../include/compiler_features.h"

R3000Acpu *psxCpu = NULL;
#ifdef DRC_DISABLE
psxRegisters psxRegs;
#endif

int psxInit() {
	SysPrintf(_("Running PCSX Version %s (%s).\n"), PCSX_VERSION, __DATE__);

#ifndef DRC_DISABLE
	if (Config.Cpu == CPU_INTERPRETER) {
		psxCpu = &psxInt;
	} else psxCpu = &psxRec;
#else
	Config.Cpu = CPU_INTERPRETER;
	psxCpu = &psxInt;
#endif

	Log = 0;

	if (psxMemInit() == -1) return -1;

	return psxCpu->Init();
}

void psxReset() {
	boolean introBypassed = FALSE;
	psxMemReset();

	memset(&psxRegs, 0, sizeof(psxRegs));

	psxRegs.pc = 0xbfc00000; // Start in bootstrap

	psxRegs.CP0.n.SR   = 0x10600000; // COP0 enabled | BEV = 1 | TS = 1
	psxRegs.CP0.n.PRid = 0x00000002; // PRevID = Revision ID, same as R3000A
	if (Config.HLE) {
		psxRegs.CP0.n.SR |= 1u << 30;    // COP2 enabled
		psxRegs.CP0.n.SR &= ~(1u << 22); // RAM exception vector
	}

	psxCpu->ApplyConfig();
	psxCpu->Reset();

	psxHwReset();
	psxBiosInit();

	if (!Config.HLE) {
		psxExecuteBios();
		if (psxRegs.pc == 0x80030000 && !Config.SlowBoot) {
			BiosBootBypass();
			introBypassed = TRUE;
		}
	}
	if (Config.HLE || introBypassed)
		psxBiosSetupBootState();

#ifdef EMU_LOG
	EMU_LOG("*BIOS END*\n");
#endif
	Log = 0;
}

void psxShutdown() {
	psxBiosShutdown();

	psxCpu->Shutdown();

	psxMemShutdown();
}

// cp0 is passed separately for lightrec to be less messy
void psxException(u32 cause, enum R3000Abdt bdt, psxCP0Regs *cp0) {
	u32 opcode = intFakeFetch(psxRegs.pc);
	
	if (unlikely(!Config.HLE && (opcode >> 25) == 0x25)) {
		// "hokuto no ken" / "Crash Bandicot 2" ...
		// BIOS does not allow to return to GTE instructions
		// (just skips it, supposedly because it's scheduled already)
		// so we execute it here
		psxCP2Regs *cp2 = (psxCP2Regs *)(cp0 + 1);
		psxRegs.code = opcode;
		psxCP2[opcode & 0x3f](cp2);
	}

	// Set the Cause
	cp0->n.Cause = (bdt << 30) | (cp0->n.Cause & 0x700) | cause;

	// Set the EPC & PC
	cp0->n.EPC = bdt ? psxRegs.pc - 4 : psxRegs.pc;

	if (cp0->n.SR & 0x400000)
		psxRegs.pc = 0xbfc00180;
	else
		psxRegs.pc = 0x80000080;

	// Set the SR
	cp0->n.SR = (cp0->n.SR & ~0x3f) | ((cp0->n.SR & 0x0f) << 2);
}

void psxBranchTest() {
	if ((psxRegs.cycle - psxNextsCounter) >= psxNextCounter)
		psxRcntUpdate();

	if (psxRegs.interrupt) {
		if ((psxRegs.interrupt & (1 << PSXINT_SIO))) { // sio
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_SIO].sCycle) >= psxRegs.intCycle[PSXINT_SIO].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_SIO);
				sioInterrupt();
			}
		}
		if (psxRegs.interrupt & (1 << PSXINT_CDR)) { // cdr
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_CDR].sCycle) >= psxRegs.intCycle[PSXINT_CDR].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_CDR);
				cdrInterrupt();
			}
		}
		if (psxRegs.interrupt & (1 << PSXINT_CDREAD)) { // cdr read
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_CDREAD].sCycle) >= psxRegs.intCycle[PSXINT_CDREAD].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_CDREAD);
				cdrPlayReadInterrupt();
			}
		}
		if (psxRegs.interrupt & (1 << PSXINT_GPUDMA)) { // gpu dma
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_GPUDMA].sCycle) >= psxRegs.intCycle[PSXINT_GPUDMA].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_GPUDMA);
				gpuInterrupt();
			}
		}
		if (psxRegs.interrupt & (1 << PSXINT_MDECOUTDMA)) { // mdec out dma
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_MDECOUTDMA].sCycle) >= psxRegs.intCycle[PSXINT_MDECOUTDMA].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_MDECOUTDMA);
				mdec1Interrupt();
			}
		}
		if (psxRegs.interrupt & (1 << PSXINT_SPUDMA)) { // spu dma
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_SPUDMA].sCycle) >= psxRegs.intCycle[PSXINT_SPUDMA].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_SPUDMA);
				spuInterrupt();
			}
		}
		if (psxRegs.interrupt & (1 << PSXINT_MDECINDMA)) { // mdec in
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_MDECINDMA].sCycle) >= psxRegs.intCycle[PSXINT_MDECINDMA].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_MDECINDMA);
				mdec0Interrupt();
			}
		}
		if (psxRegs.interrupt & (1 << PSXINT_GPUOTCDMA)) { // gpu otc
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_GPUOTCDMA].sCycle) >= psxRegs.intCycle[PSXINT_GPUOTCDMA].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_GPUOTCDMA);
				gpuotcInterrupt();
			}
		}
		if (psxRegs.interrupt & (1 << PSXINT_CDRDMA)) { // cdrom
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_CDRDMA].sCycle) >= psxRegs.intCycle[PSXINT_CDRDMA].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_CDRDMA);
				cdrDmaInterrupt();
			}
		}
		if (psxRegs.interrupt & (1 << PSXINT_CDRLID)) { // cdr lid states
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_CDRLID].sCycle) >= psxRegs.intCycle[PSXINT_CDRLID].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_CDRLID);
				cdrLidSeekInterrupt();
			}
		}
		if (psxRegs.interrupt & (1 << PSXINT_SPU_UPDATE)) { // scheduled spu update
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_SPU_UPDATE].sCycle) >= psxRegs.intCycle[PSXINT_SPU_UPDATE].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_SPU_UPDATE);
				spuUpdate();
			}
		}
	}

	psxRegs.CP0.n.Cause &= ~0x400;
	if (psxHu32(0x1070) & psxHu32(0x1074))
		psxRegs.CP0.n.Cause |= 0x400;
	if (((psxRegs.CP0.n.Cause | 1) & psxRegs.CP0.n.SR & 0x401) == 0x401)
		psxException(0, 0, &psxRegs.CP0);
}

void psxJumpTest() {
	if (!Config.HLE && Config.PsxOut) {
		u32 call = psxRegs.GPR.n.t1 & 0xff;
		switch (psxRegs.pc & 0x1fffff) {
			case 0xa0:
#ifdef PSXBIOS_LOG
				if (call != 0x28 && call != 0xe) {
					PSXBIOS_LOG("Bios call a0: %s (%x) %x,%x,%x,%x\n", biosA0n[call], call, psxRegs.GPR.n.a0, psxRegs.GPR.n.a1, psxRegs.GPR.n.a2, psxRegs.GPR.n.a3); }
#endif
				if (biosA0[call])
					biosA0[call]();
				break;
			case 0xb0:
#ifdef PSXBIOS_LOG
				if (call != 0x17 && call != 0xb) {
					PSXBIOS_LOG("Bios call b0: %s (%x) %x,%x,%x,%x\n", biosB0n[call], call, psxRegs.GPR.n.a0, psxRegs.GPR.n.a1, psxRegs.GPR.n.a2, psxRegs.GPR.n.a3); }
#endif
				if (biosB0[call])
					biosB0[call]();
				break;
			case 0xc0:
#ifdef PSXBIOS_LOG
				PSXBIOS_LOG("Bios call c0: %s (%x) %x,%x,%x,%x\n", biosC0n[call], call, psxRegs.GPR.n.a0, psxRegs.GPR.n.a1, psxRegs.GPR.n.a2, psxRegs.GPR.n.a3);
#endif
				if (biosC0[call])
					biosC0[call]();
				break;
		}
	}
}

void psxExecuteBios() {
	int i;
	for (i = 0; i < 5000000; i++) {
		psxCpu->ExecuteBlock(EXEC_CALLER_BOOT);
		if ((psxRegs.pc & 0xff800000) == 0x80000000)
			break;
	}
	if (psxRegs.pc != 0x80030000)
		SysPrintf("non-standard BIOS detected (%d, %08x)\n", i, psxRegs.pc);
}

