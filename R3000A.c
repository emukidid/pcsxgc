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
#include "PsxGpu.h"
#include "gte.h"

R3000Acpu *psxCpu = NULL;
_psxCore psxCore;

int psxInit() {
	SysPrintf(_("Running PCSX Version %s (%s).\n"), PACKAGE_VERSION, __DATE__);

#ifdef PSXREC
	if (Config.Cpu == CPU_INTERPRETER) {
		psxCpu = &psxInt;
	} else psxCpu = &psxRec;
#else
	psxCpu = &psxInt;
#endif

	Log = 0;

	if (psxMemInit() == -1) return -1;

	return psxCpu->Init();
}

void psxReset() {
	psxCpu->Reset();

	psxMemReset();

	memset(&psxCore, 0, offsetof(_psxCore,psxM));	// Don't clear ptr data stored in there.

	psxCore.pc = 0xbfc00000; // Start in bootstrap

	psxCore.CP0.r[12] = 0x10900000; // COP0 enabled | BEV = 1 | TS = 1
	psxCore.CP0.r[15] = 0x00000002; // PRevID = Revision ID, same as R3000A

	psxHwReset();
	psxBiosInit();

	if (!Config.HLE)
		psxExecuteBios();

#ifdef EMU_LOG
	EMU_LOG("*BIOS END*\n");
#endif
	Log = 0;
}

void psxShutdown() {
	psxMemShutdown();
	psxBiosShutdown();

	psxCpu->Shutdown();
}

void psxException(u32 code, u32 bd) {
	// Set the Cause
	psxCore.CP0.n.Cause = code;

	// Set the EPC & PC
	if (bd) {
#ifdef PSXCPU_LOG
		PSXCPU_LOG("bd set!!!\n");
#endif
		SysPrintf("bd set!!!\n");
		psxCore.CP0.n.Cause |= 0x80000000;
		psxCore.CP0.n.EPC = (psxCore.pc - 4);
	} else
		psxCore.CP0.n.EPC = (psxCore.pc);

	if (psxCore.CP0.n.Status & 0x400000)
		psxCore.pc = 0xbfc00180;
	else
		psxCore.pc = 0x80000080;

	// Set the Status
	psxCore.CP0.n.Status = (psxCore.CP0.n.Status &~0x3f) |
						  ((psxCore.CP0.n.Status & 0xf) << 2);

	if (Config.HLE) psxBiosException();
}

void psxBranchTest() {
	// GameShark Sampler: Give VSync pin some delay before exception eats it
	if (psxHu32(0x1070) & psxHu32(0x1074)) {
		if ((psxCore.CP0.n.Status & 0x401) == 0x401) {
			u32 opcode;

			// Crash Bandicoot 2: Don't run exceptions when GTE in pipeline
			opcode = SWAP32(*Read_ICache(psxCore.pc, TRUE));
			if( ((opcode >> 24) & 0xfe) != 0x4a ) {
#ifdef PSXCPU_LOG
				PSXCPU_LOG("Interrupt: %x %x\n", psxHu32(0x1070), psxHu32(0x1074));
#endif
				psxException(0x400, 0);
			}
		}
	}

#if 0
	if( SPU_async )
	{
		static int init;
		int elapsed;

		if( init == 0 ) {
			// 10 apu cycles
			// - Final Fantasy Tactics (distorted - dropped sound effects)
			psxCore.intCycle[PSXINT_SPUASYNC].cycle = PSXCLK / 44100 * 10;

			init = 1;
		}

		elapsed = psxCore.cycle - psxCore.intCycle[PSXINT_SPUASYNC].sCycle;
		if (elapsed >= psxCore.intCycle[PSXINT_SPUASYNC].cycle) {
			SPU_async( elapsed );

			psxCore.intCycle[PSXINT_SPUASYNC].sCycle = psxCore.cycle;
		}
	}
#endif

	if ((psxCore.cycle - psxNextsCounter) >= psxNextCounter)
		psxRcntUpdate();

	if (psxCore.interrupt) {
		if ((psxCore.interrupt & (1 << PSXINT_SIO)) && !Config.Sio) { // sio
			if ((psxCore.cycle - psxCore.intCycle[PSXINT_SIO].sCycle) >= psxCore.intCycle[PSXINT_SIO].cycle) {
				psxCore.interrupt &= ~(1 << PSXINT_SIO);
				sioInterrupt();
			}
		}
		if (psxCore.interrupt & (1 << PSXINT_CDR)) { // cdr
			if ((psxCore.cycle - psxCore.intCycle[PSXINT_CDR].sCycle) >= psxCore.intCycle[PSXINT_CDR].cycle) {
				psxCore.interrupt &= ~(1 << PSXINT_CDR);
				cdrInterrupt();
			}
		}
		if (psxCore.interrupt & (1 << PSXINT_CDREAD)) { // cdr read
			if ((psxCore.cycle - psxCore.intCycle[PSXINT_CDREAD].sCycle) >= psxCore.intCycle[PSXINT_CDREAD].cycle) {
				psxCore.interrupt &= ~(1 << PSXINT_CDREAD);
				cdrReadInterrupt();
			}
		}
		if (psxCore.interrupt & (1 << PSXINT_GPUDMA)) { // gpu dma
			if ((psxCore.cycle - psxCore.intCycle[PSXINT_GPUDMA].sCycle) >= psxCore.intCycle[PSXINT_GPUDMA].cycle) {
				psxCore.interrupt &= ~(1 << PSXINT_GPUDMA);
				gpuInterrupt();
			}
		}
		if (psxCore.interrupt & (1 << PSXINT_MDECOUTDMA)) { // mdec out dma
			if ((psxCore.cycle - psxCore.intCycle[PSXINT_MDECOUTDMA].sCycle) >= psxCore.intCycle[PSXINT_MDECOUTDMA].cycle) {
				psxCore.interrupt &= ~(1 << PSXINT_MDECOUTDMA);
				mdec1Interrupt();
			}
		}
		if (psxCore.interrupt & (1 << PSXINT_SPUDMA)) { // spu dma
			if ((psxCore.cycle - psxCore.intCycle[PSXINT_SPUDMA].sCycle) >= psxCore.intCycle[PSXINT_SPUDMA].cycle) {
				psxCore.interrupt &= ~(1 << PSXINT_SPUDMA);
				spuInterrupt();
			}
		}
    if (psxCore.interrupt & (1 << PSXINT_GPUBUSY)) { // gpu busy
      if ((psxCore.cycle - psxCore.intCycle[PSXINT_GPUBUSY].sCycle) >= psxCore.intCycle[PSXINT_GPUBUSY].cycle) {
        psxCore.interrupt &= ~(1 << PSXINT_GPUBUSY);
        GPU_idle();
      }
    }

		if (psxCore.interrupt & (1 << PSXINT_MDECINDMA)) { // mdec in
			if ((psxCore.cycle - psxCore.intCycle[PSXINT_MDECINDMA].sCycle) >= psxCore.intCycle[PSXINT_MDECINDMA].cycle) {
				psxCore.interrupt &= ~(1 << PSXINT_MDECINDMA);
				mdec0Interrupt();
			}
		}

		if (psxCore.interrupt & (1 << PSXINT_GPUOTCDMA)) { // gpu otc
			if ((psxCore.cycle - psxCore.intCycle[PSXINT_GPUOTCDMA].sCycle) >= psxCore.intCycle[PSXINT_GPUOTCDMA].cycle) {
				psxCore.interrupt &= ~(1 << PSXINT_GPUOTCDMA);
				gpuotcInterrupt();
			}
		}

		if (psxCore.interrupt & (1 << PSXINT_CDRDMA)) { // cdrom
			if ((psxCore.cycle - psxCore.intCycle[PSXINT_CDRDMA].sCycle) >= psxCore.intCycle[PSXINT_CDRDMA].cycle) {
				psxCore.interrupt &= ~(1 << PSXINT_CDRDMA);
				cdrDmaInterrupt();
			}
		}

		if (psxCore.interrupt & (1 << PSXINT_CDRPLAY)) { // cdr play timing
			if ((psxCore.cycle - psxCore.intCycle[PSXINT_CDRPLAY].sCycle) >= psxCore.intCycle[PSXINT_CDRPLAY].cycle) {
				psxCore.interrupt &= ~(1 << PSXINT_CDRPLAY);
				cdrPlayInterrupt();
			}
		}

		if (psxCore.interrupt & (1 << PSXINT_CDRDBUF)) { // cdr decoded buffer
			if ((psxCore.cycle - psxCore.intCycle[PSXINT_CDRDBUF].sCycle) >= psxCore.intCycle[PSXINT_CDRDBUF].cycle) {
				psxCore.interrupt &= ~(1 << PSXINT_CDRDBUF);
				cdrDecodedBufferInterrupt();
			}
		}

		if (psxCore.interrupt & (1 << PSXINT_CDRLID)) { // cdr lid states
			if ((psxCore.cycle - psxCore.intCycle[PSXINT_CDRLID].sCycle) >= psxCore.intCycle[PSXINT_CDRLID].cycle) {
				psxCore.interrupt &= ~(1 << PSXINT_CDRLID);
				cdrLidSeekInterrupt();
			}
		}
	}
}

void psxJumpTest() {
	if (!Config.HLE && Config.PsxOut) {
		u32 call = psxCore.GPR.n.t1 & 0xff;
		switch (psxCore.pc & 0x1fffff) {
			case 0xa0:
#ifdef PSXBIOS_LOG
				if (call != 0x28 && call != 0xe) {
					PSXBIOS_LOG("Bios call a0: %s (%x) %x,%x,%x,%x\n", biosA0n[call], call, psxCore.GPR.n.a0, psxCore.GPR.n.a1, psxCore.GPR.n.a2, psxCore.GPR.n.a3); }
#endif
				if (biosA0[call])
					biosA0[call]();
				break;
			case 0xb0:
#ifdef PSXBIOS_LOG
				if (call != 0x17 && call != 0xb) {
					PSXBIOS_LOG("Bios call b0: %s (%x) %x,%x,%x,%x\n", biosB0n[call], call, psxCore.GPR.n.a0, psxCore.GPR.n.a1, psxCore.GPR.n.a2, psxCore.GPR.n.a3); }
#endif
				if (biosB0[call])
					biosB0[call]();
				break;
			case 0xc0:
#ifdef PSXBIOS_LOG
				PSXBIOS_LOG("Bios call c0: %s (%x) %x,%x,%x,%x\n", biosC0n[call], call, psxCore.GPR.n.a0, psxCore.GPR.n.a1, psxCore.GPR.n.a2, psxCore.GPR.n.a3);
#endif
				if (biosC0[call])
					biosC0[call]();
				break;
		}
	}
}

void psxExecuteBios() {
	while (psxCore.pc != 0x80030000)
		psxCpu->ExecuteBlock();
}

