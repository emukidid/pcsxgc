/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2003  Pcsx Team
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#include "../PsxCommon.h"
#include "ppc.h"
#include "reguse.h"
#include "pR3000A.h"
#include "../R3000A.h"
#include "../PsxHLE.h"
#include "../Gamecube/DEBUG.h"

/* variable declarations */
static u32 psxRecLUT[0x010000];
static char recMem[RECMEM_SIZE] __attribute__((aligned(32)));	/* the recompiled blocks will be here */
static char recRAM[0x200000] __attribute__((aligned(32)));	/* and the ptr to the blocks here */
static char recROM[0x080000] __attribute__((aligned(32)));	/* and here */

static u32 pc;			/* recompiler pc */
static u32 pcold;		/* recompiler oldpc */
static s32 count;		/* recompiler intruction count */
static s32 branch;		/* set for branch */
static u32 target;		/* branch target */
static iRegisters iRegs[34];

static void (*recBSC[64])();
static void (*recSPC[64])();
static void (*recREG[32])();
static void (*recCP0[32])();
static void (*recCP2[64])();
static void (*recCP2BSC[32])();

static HWRegister HWRegisters[NUM_HW_REGISTERS];
static s32 HWRegUseCount;
static s32 DstCPUReg;
static s32 UniqueRegAlloc;

static s32 GetFreeHWReg();
static void InvalidateCPURegs();
static void DisposeHWReg(s32 index);
static void FlushHWReg(s32 index);
static void FlushAllHWReg();
static void MapPsxReg32(s32 reg);
static void FlushPsxReg32(s32 hwreg);
static s32 UpdateHWRegUsage(s32 hwreg, s32 usage);
static s32 GetHWReg32(s32 reg);
static s32 PutHWReg32(s32 reg);
static s32 GetSpecialIndexFromHWRegs(s32 which);
static s32 GetHWRegFromCPUReg(s32 cpureg);
static s32 MapRegSpecial(s32 which);
static void FlushRegSpecial(s32 hwreg);
static s32 GetHWRegSpecial(s32 which);
static s32 PutHWRegSpecial(s32 which);
static void recRecompile();
static void recError();

// used in debug.c for dynarec free space printing
u32 dyna_used = 0;
u32 dyna_total = RECMEM_SIZE;

/* --- Generic register mapping --- */

static s32 GetFreeHWReg()
{
	s32 i, least, index;
	
	if (DstCPUReg != -1) {
		index = GetHWRegFromCPUReg(DstCPUReg);
		DstCPUReg = -1;
	} else {
	    // LRU algorith with a twist ;)
	    for (i=0; i<NUM_HW_REGISTERS; i++) {
		    if (!(HWRegisters[i].usage & HWUSAGE_RESERVED)) {
			    break;
		    }
	    }
    
	    least = HWRegisters[i].lastUsed; index = i;
	    for (; i<NUM_HW_REGISTERS; i++) {
		    if (!(HWRegisters[i].usage & HWUSAGE_RESERVED)) {
			    if (HWRegisters[i].usage == HWUSAGE_NONE && HWRegisters[i].code >= 13) {
				    index = i;
				    break;
			    }
			    else if (HWRegisters[i].lastUsed < least) {
				    least = HWRegisters[i].lastUsed;
				    index = i;
			    }
		    }
	    }
		 
		 // Cycle the registers
		 if (HWRegisters[index].usage == HWUSAGE_NONE) {
			for (; i<NUM_HW_REGISTERS; i++) {
				if (!(HWRegisters[i].usage & HWUSAGE_RESERVED)) {
					if (HWRegisters[i].usage == HWUSAGE_NONE && 
						 HWRegisters[i].code >= 13 && 
						 HWRegisters[i].lastUsed < least) {
						least = HWRegisters[i].lastUsed;
						index = i;
						break;
					}
				}
			}
		 }
	}
	
	if (HWRegisters[index].usage & (HWUSAGE_RESERVED | HWUSAGE_HARDWIRED)) {
		if (HWRegisters[index].usage & HWUSAGE_RESERVED) {
			SysPrintf("Error! Trying to map a new register to a reserved register (r%i)", 
						HWRegisters[index].code);
		}
		if (HWRegisters[index].usage & HWUSAGE_HARDWIRED) {
			SysPrintf("Error! Trying to map a new register to a hardwired register (r%i)", 
						HWRegisters[index].code);
		}
	}
	
	if (HWRegisters[index].lastUsed != 0) {
		UniqueRegAlloc = 0;
	}
	
	// Make sure the register is really flushed!
	FlushHWReg(index);
	HWRegisters[index].usage = HWUSAGE_NONE;
	HWRegisters[index].flush = NULL;
	
	return index;
}

static void FlushHWReg(s32 index)
{
	if (index < 0) return;
	if (HWRegisters[index].usage == HWUSAGE_NONE) return;
	
	if (HWRegisters[index].flush) {
		HWRegisters[index].usage |= HWUSAGE_RESERVED;
		HWRegisters[index].flush(index);
		HWRegisters[index].flush = NULL;
	}
	
	if (HWRegisters[index].usage & HWUSAGE_HARDWIRED) {
		HWRegisters[index].usage &= ~(HWUSAGE_READ | HWUSAGE_WRITE);
	} else {
		HWRegisters[index].usage = HWUSAGE_NONE;
	}
}

// get rid of a mapped register without flushing the contents to the memory
static void DisposeHWReg(s32 index)
{
	if (index < 0) return;
	if (HWRegisters[index].usage == HWUSAGE_NONE) return;
	
	HWRegisters[index].usage &= ~(HWUSAGE_READ | HWUSAGE_WRITE);
	if (HWRegisters[index].usage == HWUSAGE_NONE) {
		SysPrintf("Error! not correctly disposing register (r%i)", HWRegisters[index].code);
	}
	
	FlushHWReg(index);
}

// operated on cpu registers
__inline static void FlushCPURegRange(s32 start, s32 end)
{
	s32 i;
	
	if (end <= 0) end = 31;
	if (start <= 0) start = 0;
	
	for (i=0; i<NUM_HW_REGISTERS; i++) {
		if (HWRegisters[i].code >= start && HWRegisters[i].code <= end)
			if (HWRegisters[i].flush)
				FlushHWReg(i);
	}

	for (i=0; i<NUM_HW_REGISTERS; i++) {
		if (HWRegisters[i].code >= start && HWRegisters[i].code <= end)
			FlushHWReg(i);
	}
}

static void FlushAllHWReg()
{
	FlushCPURegRange(0,31);
}

static void InvalidateCPURegs()
{
	FlushCPURegRange(0,12);
}

/* --- Mapping utility functions --- */

static void MoveHWRegToCPUReg(s32 cpureg, s32 hwreg)
{
	s32 dstreg;
	
	if (HWRegisters[hwreg].code == cpureg)
		return;
	
	dstreg = GetHWRegFromCPUReg(cpureg);
	
	HWRegisters[dstreg].usage &= ~(HWUSAGE_HARDWIRED | HWUSAGE_ARG);
	if (HWRegisters[hwreg].usage & (HWUSAGE_READ | HWUSAGE_WRITE)) {
		FlushHWReg(dstreg);
		MR(HWRegisters[dstreg].code, HWRegisters[hwreg].code);
	} else {
		if (HWRegisters[dstreg].usage & (HWUSAGE_READ | HWUSAGE_WRITE)) {
			MR(HWRegisters[hwreg].code, HWRegisters[dstreg].code);
		}
		else if (HWRegisters[dstreg].usage != HWUSAGE_NONE) {
			FlushHWReg(dstreg);
		}
	}
	
	HWRegisters[dstreg].code = HWRegisters[hwreg].code;
	HWRegisters[hwreg].code = cpureg;
}

static s32 UpdateHWRegUsage(s32 hwreg, s32 usage)
{
	HWRegisters[hwreg].lastUsed = ++HWRegUseCount;    
	if (usage & HWUSAGE_WRITE) {
		HWRegisters[hwreg].usage &= ~HWUSAGE_CONST;
	}
	if (!(usage & HWUSAGE_INITED)) {
		HWRegisters[hwreg].usage &= ~HWUSAGE_INITED;
	}
	HWRegisters[hwreg].usage |= usage;
	
	return HWRegisters[hwreg].code;
}

static s32 GetHWRegFromCPUReg(s32 cpureg)
{
	s32 i;
	for (i=0; i<NUM_HW_REGISTERS; i++) {
		if (HWRegisters[i].code == cpureg) {
			return i;
		}
	}
	
	SysPrintf("Error! Register location failure (r%i)", cpureg);
	return 0;
}

// this function operates on cpu registers
void SetDstCPUReg(s32 cpureg)
{
	DstCPUReg = cpureg;
}

static void ReserveArgs(s32 args)
{
	s32 index, i;
	
	for (i=0; i<args; i++) {
		index = GetHWRegFromCPUReg(3+i);
		HWRegisters[index].usage |= HWUSAGE_RESERVED | HWUSAGE_HARDWIRED | HWUSAGE_ARG;
	}
}

static void ReleaseArgs()
{
	s32 i;
	
	for (i=0; i<NUM_HW_REGISTERS; i++) {
		if (HWRegisters[i].usage & HWUSAGE_ARG) {
			HWRegisters[i].usage &= ~(HWUSAGE_RESERVED | HWUSAGE_HARDWIRED | HWUSAGE_ARG);
			FlushHWReg(i);
		}
	}
}

/* --- Psx register mapping --- */

static void MapPsxReg32(s32 reg)
{
    s32 hwreg = GetFreeHWReg();
    HWRegisters[hwreg].flush = FlushPsxReg32;
    HWRegisters[hwreg].private = reg;
    
    if (iRegs[reg].reg != -1) {
        SysPrintf("error: double mapped psx register");
    }
    
    iRegs[reg].reg = hwreg;
    iRegs[reg].state |= ST_MAPPED;
}

static void FlushPsxReg32(s32 hwreg)
{
	s32 reg = HWRegisters[hwreg].private;
	
	if (iRegs[reg].reg == -1) {
		SysPrintf("error: flushing unmapped psx register");
	}
	
	if (HWRegisters[hwreg].usage & HWUSAGE_WRITE) {
		if (branch) {
			STW(HWRegisters[hwreg].code, OFFSET(&psxRegs, &psxRegs.GPR.r[reg]), GetHWRegSpecial(PSXREGS));
		} else {
			STW(HWRegisters[hwreg].code, OFFSET(&psxRegs, &psxRegs.GPR.r[reg]), GetHWRegSpecial(PSXREGS));
		}
	}
	
	iRegs[reg].reg = -1;
	iRegs[reg].state = ST_UNK;
}

static s32 GetHWReg32(s32 reg)
{
	s32 usage = HWUSAGE_PSXREG | HWUSAGE_READ;
	
	if (reg == 0) {
		return GetHWRegSpecial(REG_RZERO);
	}
	if (!IsMapped(reg)) {
		usage |= HWUSAGE_INITED;
		MapPsxReg32(reg);
		
		HWRegisters[iRegs[reg].reg].usage |= HWUSAGE_RESERVED;
		if (IsConst(reg)) {
			LIW(HWRegisters[iRegs[reg].reg].code, iRegs[reg].k);
			usage |= HWUSAGE_WRITE | HWUSAGE_CONST;
		}
		else {
			LWZ(HWRegisters[iRegs[reg].reg].code, OFFSET(&psxRegs, &psxRegs.GPR.r[reg]), GetHWRegSpecial(PSXREGS));
		}
		HWRegisters[iRegs[reg].reg].usage &= ~HWUSAGE_RESERVED;
	}
	else if (DstCPUReg != -1) {
		s32 dst = DstCPUReg;
		DstCPUReg = -1;
		
		if (HWRegisters[iRegs[reg].reg].code < 13) {
			MoveHWRegToCPUReg(dst, iRegs[reg].reg);
		} else {
			MR(DstCPUReg, HWRegisters[iRegs[reg].reg].code);
		}
	}
	
	DstCPUReg = -1;
	
	return UpdateHWRegUsage(iRegs[reg].reg, usage);
}

static s32 PutHWReg32(s32 reg)
{
	s32 usage = HWUSAGE_PSXREG | HWUSAGE_WRITE;
	if (reg == 0) {
		return PutHWRegSpecial(REG_WZERO);
	}
	
	if (DstCPUReg != -1 && IsMapped(reg)) {
		if (HWRegisters[iRegs[reg].reg].code != DstCPUReg) {
			s32 tmp = DstCPUReg;
			DstCPUReg = -1;
			DisposeHWReg(iRegs[reg].reg);
			DstCPUReg = tmp;
		}
	}
	if (!IsMapped(reg)) {
		usage |= HWUSAGE_INITED;
		MapPsxReg32(reg);
	}
	
	DstCPUReg = -1;
	iRegs[reg].state &= ~ST_CONST;

	return UpdateHWRegUsage(iRegs[reg].reg, usage);
}

/* --- Special register mapping --- */

static s32 GetSpecialIndexFromHWRegs(s32 which)
{
	s32 i;
	for (i=0; i<NUM_HW_REGISTERS; i++) {
		if (HWRegisters[i].usage & HWUSAGE_SPECIAL) {
			if (HWRegisters[i].private == which) {
				return i;
			}
		}
	}
	return -1;
}

static s32 MapRegSpecial(s32 which)
{
	s32 hwreg = GetFreeHWReg();
	HWRegisters[hwreg].flush = FlushRegSpecial;
	HWRegisters[hwreg].private = which;
	
	return hwreg;
}

static void FlushRegSpecial(s32 hwreg)
{
	s32 which = HWRegisters[hwreg].private;
	
	if (!(HWRegisters[hwreg].usage & HWUSAGE_WRITE))
		return;
	
	switch (which) {
		case CYCLECOUNT:
			STW(HWRegisters[hwreg].code, OFFSET(&psxRegs, &psxRegs.cycle), GetHWRegSpecial(PSXREGS));
			break;
		case PSXPC:
			STW(HWRegisters[hwreg].code, OFFSET(&psxRegs, &psxRegs.pc), GetHWRegSpecial(PSXREGS));
			break;
		case TARGET:
			STW(HWRegisters[hwreg].code, 0, GetHWRegSpecial(TARGETPTR));
			break;
	}
}

static s32 GetHWRegSpecial(s32 which)
{
	s32 index = GetSpecialIndexFromHWRegs(which);
	s32 usage = HWUSAGE_READ | HWUSAGE_SPECIAL;
	
	if (index == -1) {
		usage |= HWUSAGE_INITED;
		index = MapRegSpecial(which);
		
		HWRegisters[index].usage |= HWUSAGE_RESERVED;
		switch (which) {
			case PSXREGS:
			case PSXMEM:
			case PSXRLUT:
			case PSXWLUT:
			case PSXRECLUT:
				print_gecko("error! shouldn't be here!\n");
				break;
			case TARGETPTR:
				HWRegisters[index].flush = NULL;
				LIW(HWRegisters[index].code, (u32)&target);
				break;
			case REG_RZERO:
				HWRegisters[index].flush = NULL;
				LIW(HWRegisters[index].code, 0);
				break;
			case RETVAL:
				MoveHWRegToCPUReg(3, index);
				HWRegisters[index].flush = NULL;
				
				usage |= HWUSAGE_RESERVED;
				break;

			case CYCLECOUNT:
				LWZ(HWRegisters[index].code, OFFSET(&psxRegs, &psxRegs.cycle), GetHWRegSpecial(PSXREGS));
				break;
			case PSXPC:
				LWZ(HWRegisters[index].code, OFFSET(&psxRegs, &psxRegs.pc), GetHWRegSpecial(PSXREGS));
				break;
			case TARGET:
				LWZ(HWRegisters[index].code, 0, GetHWRegSpecial(TARGETPTR));
				break;
			default:
				SysPrintf("Error: Unknown special register in GetHWRegSpecial()\n");
				break;
		}
		HWRegisters[index].usage &= ~HWUSAGE_RESERVED;
	}
	else if (DstCPUReg != -1) {
		s32 dst = DstCPUReg;
		DstCPUReg = -1;
		
		MoveHWRegToCPUReg(dst, index);
	}

	return UpdateHWRegUsage(index, usage);
}

static s32 PutHWRegSpecial(s32 which)
{
	s32 index = GetSpecialIndexFromHWRegs(which);
	s32 usage = HWUSAGE_WRITE | HWUSAGE_SPECIAL;
	
	if (DstCPUReg != -1 && index != -1) {
		if (HWRegisters[index].code != DstCPUReg) {
			s32 tmp = DstCPUReg;
			DstCPUReg = -1;
			DisposeHWReg(index);
			DstCPUReg = tmp;
		}
	}
	switch (which) {
		case PSXREGS:
		case TARGETPTR:
			SysPrintf("Error: Read-only special register in PutHWRegSpecial()\n");
		case REG_WZERO:
			if (index >= 0) {
					if (HWRegisters[index].usage & HWUSAGE_WRITE)
						break;
			}
			index = MapRegSpecial(which);
			HWRegisters[index].flush = NULL;
			break;
		default:
			if (index == -1) {
				usage |= HWUSAGE_INITED;
				index = MapRegSpecial(which);
				
				HWRegisters[index].usage |= HWUSAGE_RESERVED;
				switch (which) {
					case ARG1:
					case ARG2:
					case ARG3:
					case TMP1:
					case TMP2:
					case TMP3:
						MoveHWRegToCPUReg(3+(which-ARG1), index);
						HWRegisters[index].flush = NULL;
						
						usage |= HWUSAGE_RESERVED | HWUSAGE_HARDWIRED | HWUSAGE_ARG;
						break;
				}
			}
			HWRegisters[index].usage &= ~HWUSAGE_RESERVED;
			break;
	}
	
	DstCPUReg = -1;

	return UpdateHWRegUsage(index, usage);
}

static void MapConst(s32 reg, u32 _const) {
	if (reg == 0)
		return;
	if (IsConst(reg) && iRegs[reg].k == _const)
		return;
	
	DisposeHWReg(iRegs[reg].reg);
	iRegs[reg].k = _const;
	iRegs[reg].state = ST_CONST;
}

static void MapCopy(s32 dst, s32 src)
{
    // do it the lazy way for now
    MR(PutHWReg32(dst), GetHWReg32(src));
}

static void iFlushReg(u32 nextpc, s32 reg) {
	if (!IsMapped(reg) && IsConst(reg)) {
		GetHWReg32(reg);
	}
	if (IsMapped(reg)) {
		FlushHWReg(iRegs[reg].reg);
	}
}

static void iFlushRegs(u32 nextpc) {
	s32 i;

	for (i=1; i<NUM_REGISTERS; i++) {
		iFlushReg(nextpc, i);
	}
}

static void Return()
{
	iFlushRegs(0);
	FlushAllHWReg();
	LIW(0, (u32)returnPC);
	MTLR(0);
	BLR();

}

static void iStoreCycle(s32 ahead) {
	/* store cycle */
    count = (((pc+ahead) - pcold)/4)<<1;
    ADDI(PutHWRegSpecial(CYCLECOUNT), GetHWRegSpecial(CYCLECOUNT), count);
}

static void iRet() {
    iStoreCycle(0);
    Return();
}

static s32 iLoadTest() {
	u32 tmp;

	// check for load delay
	tmp = psxRegs.code >> 26;
	switch (tmp) {
		case 0x10: // COP0
			switch (_Rs_) {
				case 0x00: // MFC0
				case 0x02: // CFC0
					return 1;
			}
			break;
		case 0x12: // COP2
			switch (_Funct_) {
				case 0x00:
					switch (_Rs_) {
						case 0x00: // MFC2
						case 0x02: // CFC2
							return 1;
					}
					break;
			}
			break;
		case 0x32: // LWC2
			return 1;
		default:
			if (tmp >= 0x20 && tmp <= 0x26) { // LB/LH/LWL/LW/LBU/LHU/LWR
				return 1;
			}
			break;
	}
	return 0;
}

/* set a pending branch */
static void SetBranch() {
	s32 treg;
	branch = 1;
	psxRegs.code = PSXMu32(pc);
	pc+=4;

	if (iLoadTest() == 1) {
		iFlushRegs(0);
		LIW(0, psxRegs.code);
		STW(0, OFFSET(&psxRegs, &psxRegs.code), GetHWRegSpecial(PSXREGS));
		iStoreCycle(0);
		
		treg = GetHWRegSpecial(TARGET);
		MR(PutHWRegSpecial(ARG2), treg);
		DisposeHWReg(GetHWRegFromCPUReg(treg));
		LIW(PutHWRegSpecial(ARG1), _Rt_);
                LIW(GetHWRegSpecial(PSXPC), pc);
		FlushAllHWReg();
		CALLFunc((u32)psxDelayTest);

		Return();
		return;
	}

	recBSC[psxRegs.code>>26]();

	iFlushRegs(0);
	treg = GetHWRegSpecial(TARGET);
	MR(PutHWRegSpecial(PSXPC), GetHWRegSpecial(TARGET)); // FIXME: this line should not be needed
	DisposeHWReg(GetHWRegFromCPUReg(treg));
	FlushAllHWReg();

	iStoreCycle(0);
	FlushAllHWReg();
	CALLFunc((u32)psxBranchTest);
	CALLFunc((u32)psxJumpTest);
	
	// TODO: don't return if target is compiled
	Return();
}

static void iJump(u32 branchPC) {
	u32 *b1, *b2;
	branch = 1;
	psxRegs.code = PSXMu32(pc);
	pc+=4;

	if (iLoadTest() == 1) {
		iFlushRegs(0);
		LIW(0, psxRegs.code);
		STW(0, OFFSET(&psxRegs, &psxRegs.code), GetHWRegSpecial(PSXREGS));
		/* store cycle */
		iStoreCycle(0);

		LIW(PutHWRegSpecial(ARG2), branchPC);
		LIW(PutHWRegSpecial(ARG1), _Rt_);
		LIW(GetHWRegSpecial(PSXPC), pc);
		FlushAllHWReg();
		CALLFunc((u32)psxDelayTest);
                
		Return();
		return;
	}

	recBSC[psxRegs.code>>26]();

	iFlushRegs(branchPC);
	LIW(PutHWRegSpecial(PSXPC), branchPC);
	FlushAllHWReg();

	iStoreCycle(0);
	FlushAllHWReg();
	CALLFunc((u32)psxBranchTest);
	CALLFunc((u32)psxJumpTest);

	// always return for now...
	Return();
		
	// maybe just happened an interruption, check so
	LIW(0, branchPC);
	CMPLW(GetHWRegSpecial(PSXPC), 0);
	BNE_L(b1);
	
	LIW(3, PC_REC(branchPC));
	LWZ(3, 0, 3);
	CMPLWI(3, 0);
	BNE_L(b2);
	
	B_DST(b1);
	Return();
	
	// next bit is already compiled - jump right to it
	B_DST(b2);
	MTCTR(3);
	BCTR();
}

static void iBranch(u32 branchPC, s32 savectx) {
	HWRegister HWRegistersS[NUM_HW_REGISTERS];
	iRegisters iRegsS[NUM_REGISTERS];
	s32 HWRegUseCountS = 0;
	u32 *b1, *b2;

	if (savectx) {
		memcpy(iRegsS, iRegs, sizeof(iRegs));
		memcpy(HWRegistersS, HWRegisters, sizeof(HWRegisters));
		HWRegUseCountS = HWRegUseCount;
	}
	
	branch = 1;
	psxRegs.code = PSXMu32(pc);

	// the delay test is only made when the branch is taken
	// savectx == 0 will mean that :)
	if (savectx == 0 && iLoadTest() == 1) {
		iFlushRegs(0);
		LIW(0, psxRegs.code);
		STW(0, OFFSET(&psxRegs, &psxRegs.code), GetHWRegSpecial(PSXREGS));
		/* store cycle */
		iStoreCycle(4);

		LIW(PutHWRegSpecial(ARG2), branchPC);
		LIW(PutHWRegSpecial(ARG1), _Rt_);
                LIW(GetHWRegSpecial(PSXPC), pc);
		FlushAllHWReg();
		CALLFunc((u32)psxDelayTest);

		Return();
		return;
	}

	pc+= 4;
	recBSC[psxRegs.code>>26]();
	
	iFlushRegs(branchPC);
	LIW(PutHWRegSpecial(PSXPC), branchPC);
	FlushAllHWReg();

	iStoreCycle(0);
	FlushAllHWReg();
	CALLFunc((u32)psxBranchTest);
	CALLFunc((u32)psxJumpTest);
	
	// always return for now...
	//Return();

	LIW(0, branchPC);
	CMPLW(GetHWRegSpecial(PSXPC), 0);
	BNE_L(b1);
	
	LIW(3, PC_REC(branchPC));
	LWZ(3, 0, 3);
	CMPLWI(3, 0);
	BNE_L(b2);
	
	B_DST(b1);
	Return();
	
	B_DST(b2);
	MTCTR(3);
	BCTR();

	// maybe just happened an interruption, check so
/*	CMP32ItoM((u32)&psxRegs.pc, branchPC);
	j8Ptr[1] = JE8(0);
	RET();

	x86SetJ8(j8Ptr[1]);
	MOV32MtoR(EAX, PC_REC(branchPC));
	TEST32RtoR(EAX, EAX);
	j8Ptr[2] = JNE8(0);
	RET();

	x86SetJ8(j8Ptr[2]);
	JMP32R(EAX);*/

	pc-= 4;
	if (savectx) {
		memcpy(iRegs, iRegsS, sizeof(iRegs));
		memcpy(HWRegisters, HWRegistersS, sizeof(HWRegisters));
		HWRegUseCount = HWRegUseCountS;
	}
}


void iDumpRegs() {
	s32 i, j;

	printf("%08lx %08lx\n", psxRegs.pc, psxRegs.cycle);
	for (i=0; i<4; i++) {
		for (j=0; j<8; j++)
			printf("%08lx ", psxRegs.GPR.r[j*i]);
		printf("\n");
	}
}

void iDumpBlock(char *ptr) {

}

#define REC_FUNC(f) \
void psx##f(); \
static void rec##f() { \
	iFlushRegs(0); \
	LIW(PutHWRegSpecial(ARG1), (u32)psxRegs.code); \
	STW(GetHWRegSpecial(ARG1), OFFSET(&psxRegs, &psxRegs.code), GetHWRegSpecial(PSXREGS)); \
	LIW(PutHWRegSpecial(PSXPC), (u32)pc); \
	FlushAllHWReg(); \
	CALLFunc((u32)psx##f); \
/*	branch = 2; */\
}

#define REC_SYS(f) \
void psx##f(); \
static void rec##f() { \
	iFlushRegs(0); \
        LIW(PutHWRegSpecial(ARG1), (u32)psxRegs.code); \
        STW(GetHWRegSpecial(ARG1), OFFSET(&psxRegs, &psxRegs.code), GetHWRegSpecial(PSXREGS)); \
        LIW(PutHWRegSpecial(PSXPC), (u32)pc); \
        FlushAllHWReg(); \
	CALLFunc((u32)psx##f); \
	branch = 2; \
	iRet(); \
}

#define REC_BRANCH(f) \
void psx##f(); \
static void rec##f() { \
	iFlushRegs(0); \
        LIW(PutHWRegSpecial(ARG1), (u32)psxRegs.code); \
        STW(GetHWRegSpecial(ARG1), OFFSET(&psxRegs, &psxRegs.code), GetHWRegSpecial(PSXREGS)); \
        LIW(PutHWRegSpecial(PSXPC), (u32)pc); \
        FlushAllHWReg(); \
	CALLFunc((u32)psx##f); \
	branch = 2; \
	iRet(); \
}

#define CP2_FUNC(f) \
void gte##f(); \
static void rec##f() { \
	iFlushRegs(0); \
	LIW(0, (u32)psxRegs.code); \
	STW(0, OFFSET(&psxRegs, &psxRegs.code), GetHWRegSpecial(PSXREGS)); \
	FlushAllHWReg(); \
	CALLFunc ((u32)gte##f); \
}

#define CP2_FUNCNC(f) \
void gte##f(); \
static void rec##f() { \
	iFlushRegs(0); \
	CALLFunc ((u32)gte##f); \
/*	branch = 2; */\
}

static s32 allocMem() {
	s32 i;

	for (i=0; i<0x80; i++) psxRecLUT[i + 0x0000] = (u32)&recRAM[(i & 0x1f) << 16];
	memcpy(psxRecLUT + 0x8000, psxRecLUT, 0x80 * 4);
	memcpy(psxRecLUT + 0xa000, psxRecLUT, 0x80 * 4);

	for (i=0; i<0x08; i++) psxRecLUT[i + 0xbfc0] = (u32)&recROM[i << 16];
	
	return 0;
}

static s32 recInit() {
	return allocMem();
}

static void recReset() {
	memset(recRAM, 0, 0x200000);
	memset(recROM, 0, 0x080000);

	ppcInit();
	ppcSetPtr((u32 *)recMem);

	branch = 0;
	memset(iRegs, 0, sizeof(iRegs));
	iRegs[0].state = ST_CONST;
	iRegs[0].k     = 0;
}

static void recShutdown() {
	ppcShutdown();
}

static void recError() {
	SysReset();
	ClosePlugins();
	SysMessage("Unrecoverable error while running recompiler\n");
	SysRunGui();
}

//static int executedBlockCount = 0;
__inline static void execute() {
	void (**recFunc)();
	char *p;

	//print_gecko("START PC: %08X COUNT: %08X\r\n", psxRegs.pc, psxRegs.cycle);
	p =	(char*)PC_REC(psxRegs.pc);
	recFunc = (void (**)()) (u32)p;

	if (*recFunc == 0) {
#ifdef PROFILE
		start_section(COMPILER_SECTION);
#endif
		recRecompile();
#ifdef PROFILE
		end_section(COMPILER_SECTION);
#endif
	}
#ifdef PROFILE
		start_section(CORE_SECTION);
#endif
	recRun(*recFunc, (u32)&psxRegs, (u32)&psxM, (u32)&psxMemRLUT);
#ifdef PROFILE
		end_section(CORE_SECTION);
#endif
	//print_gecko("END PC: %08X COUNT: %08X\r\n", psxRegs.pc, psxRegs.cycle);
	//executedBlockCount++;
	//if(executedBlockCount == 8) exit(1);
}

static void recExecute() {
	while(!stop) execute();
}

static void recExecuteBlock() {
	execute();
}

static void recClear(u32 Addr, u32 Size) {
	memset((void*)PC_REC(Addr), 0, Size * 4);
}

static void recNULL() {
//	SysMessage("recUNK: %8.8x\n", psxRegs.code);
}

/*********************************************************
* goes to opcodes tables...                              *
* Format:  table[something....]                          *
*********************************************************/

//REC_SYS(SPECIAL);
static void recSPECIAL() {
	recSPC[_Funct_]();
}

static void recREGIMM() {
	recREG[_Rt_]();
}

static void recCOP0() {
	recCP0[_Rs_]();
}

//REC_SYS(COP2);
static void recCOP2() {
	recCP2[_Funct_]();
}

static void recBASIC() {
	recCP2BSC[_Rs_]();
}

//end of Tables opcodes...

/* - Arithmetic with immediate operand - */
/*********************************************************
* Arithmetic with immediate operand                      *
* Format:  OP rt, rs, immediate                          *
*********************************************************/

static void recADDIU()  {
// Rt = Rs + Im
	if (!_Rt_) return;

	if (IsConst(_Rs_)) {
		MapConst(_Rt_, iRegs[_Rs_].k + _Imm_);
	} else {
		if (_Imm_ == 0) {
			MapCopy(_Rt_, _Rs_);
		} else {
			ADDI(PutHWReg32(_Rt_), GetHWReg32(_Rs_), _Imm_);
		}
	}
}

static void recADDI()  {
// Rt = Rs + Im
	recADDIU();
}

//CR0:	SIGN      | POSITIVE | ZERO  | SOVERFLOW | SOVERFLOW | OVERFLOW | CARRY
static void recSLTI() {
// Rt = Rs < Im (signed)
	if (!_Rt_) return;

	if (IsConst(_Rs_)) {
		MapConst(_Rt_, (s32)iRegs[_Rs_].k < _Imm_);
	} else {
		if (_Imm_ == 0) {
			SRWI(PutHWReg32(_Rt_), GetHWReg32(_Rs_), 31);
		} else {
			s32 reg;
			CMPWI(GetHWReg32(_Rs_), _Imm_);
			reg = PutHWReg32(_Rt_);
			LI(reg, 1);
			BLT(1);
			LI(reg, 0);
		}
	}
}

static void recSLTIU() {
// Rt = Rs < Im (unsigned)
	if (!_Rt_) return;

	if (IsConst(_Rs_)) {
		MapConst(_Rt_, iRegs[_Rs_].k < _ImmU_);
	} else {
		s32 reg;
		CMPLWI(GetHWReg32(_Rs_), _Imm_);
		reg = PutHWReg32(_Rt_);
		LI(reg, 1);
		BLT(1);
		LI(reg, 0);
	}
}

static void recANDI() {
// Rt = Rs And Im
    if (!_Rt_) return;

    if (IsConst(_Rs_)) {
        MapConst(_Rt_, iRegs[_Rs_].k & _ImmU_);
    } else {
        ANDI_(PutHWReg32(_Rt_), GetHWReg32(_Rs_), _ImmU_);
    }
}

static void recORI() {
// Rt = Rs Or Im
	if (!_Rt_) return;

	if (IsConst(_Rs_)) {
		MapConst(_Rt_, iRegs[_Rs_].k | _ImmU_);
	} else {
		if (_Imm_ == 0) {
			MapCopy(_Rt_, _Rs_);
		} else {
			ORI(PutHWReg32(_Rt_), GetHWReg32(_Rs_), _ImmU_);
		}
	}
}

static void recXORI() {
// Rt = Rs Xor Im
    if (!_Rt_) return;

    if (IsConst(_Rs_)) {
        MapConst(_Rt_, iRegs[_Rs_].k ^ _ImmU_);
    } else {
        XORI(PutHWReg32(_Rt_), GetHWReg32(_Rs_), _ImmU_);
    }
}

//end of * Arithmetic with immediate operand  

/*********************************************************
* Load higher 16 bits of the first word in GPR with imm  *
* Format:  OP rt, immediate                              *
*********************************************************/

static void recLUI()  {
// Rt = Imm << 16
	if (!_Rt_) return;

	MapConst(_Rt_, psxRegs.code << 16);
}

//End of Load Higher .....

/* - Register arithmetic - */
/*********************************************************
* Register arithmetic                                    *
* Format:  OP rd, rs, rt                                 *
*********************************************************/

static void recADDU() {
// Rd = Rs + Rt 
	if (!_Rd_) return;

	if (IsConst(_Rs_) && IsConst(_Rt_)) {
		MapConst(_Rd_, iRegs[_Rs_].k + iRegs[_Rt_].k);
	} else {
		ADD(PutHWReg32(_Rd_), GetHWReg32(_Rs_), GetHWReg32(_Rt_));
	}
}

static void recADD() {
// Rd = Rs + Rt
	recADDU();
}

static void recSUBU() {
// Rd = Rs - Rt
    if (!_Rd_) return;

    if (IsConst(_Rs_) && IsConst(_Rt_)) {
        MapConst(_Rd_, iRegs[_Rs_].k - iRegs[_Rt_].k);
    } else {
        SUB(PutHWReg32(_Rd_), GetHWReg32(_Rs_), GetHWReg32(_Rt_));
    }
}   

static void recSUB() {
// Rd = Rs - Rt
	recSUBU();
}

static void recAND() {
// Rd = Rs And Rt
    if (!_Rd_) return;
    
    if (IsConst(_Rs_) && IsConst(_Rt_)) {
        MapConst(_Rd_, iRegs[_Rs_].k & iRegs[_Rt_].k);
    } else {
        AND(PutHWReg32(_Rd_), GetHWReg32(_Rs_), GetHWReg32(_Rt_));
    }
}   

static void recOR() {
// Rd = Rs Or Rt
    if (!_Rd_) return;
    
    if (IsConst(_Rs_) && IsConst(_Rt_)) {
        MapConst(_Rd_, iRegs[_Rs_].k | iRegs[_Rt_].k);
    }
    else {
        OR(PutHWReg32(_Rd_), GetHWReg32(_Rs_), GetHWReg32(_Rt_));
    }
}

static void recXOR() {
// Rd = Rs Xor Rt
    if (!_Rd_) return;
    
    if (IsConst(_Rs_) && IsConst(_Rt_)) {
        MapConst(_Rd_, iRegs[_Rs_].k ^ iRegs[_Rt_].k);
    } else {
        XOR(PutHWReg32(_Rd_), GetHWReg32(_Rs_), GetHWReg32(_Rt_));
    }
}

static void recNOR() {
// Rd = Rs Nor Rt
    if (!_Rd_) return;
    
    if (IsConst(_Rs_) && IsConst(_Rt_)) {
        MapConst(_Rd_, ~(iRegs[_Rs_].k | iRegs[_Rt_].k));
    } else {
        NOR(PutHWReg32(_Rd_), GetHWReg32(_Rs_), GetHWReg32(_Rt_));
    }
}

static void recSLT() {
// Rd = Rs < Rt (signed)
    if (!_Rd_) return;

    if (IsConst(_Rs_) && IsConst(_Rt_)) {
        MapConst(_Rd_, (s32)iRegs[_Rs_].k < (s32)iRegs[_Rt_].k);
    } else { 
		/*SUBFC(0, GetHWReg32(_Rt_), GetHWReg32(_Rs_));
		EQV(PutHWReg32(_Rd_), GetHWReg32(_Rt_), GetHWReg32(_Rs_));
		SRWI(PutHWReg32(_Rd_), GetHWReg32(_Rd_), 31);
		ADDZE(PutHWReg32(_Rd_), GetHWReg32(_Rd_));
		RLWINM(PutHWReg32(_Rd_), GetHWReg32(_Rd_), 0, 31, 31);*/
        s32 reg;
        CMPW(GetHWReg32(_Rs_), GetHWReg32(_Rt_));
        reg = PutHWReg32(_Rd_);
        LI(reg, 1);
        BLT(1);
        LI(reg, 0);
    }
}

static void recSLTU() { 
// Rd = Rs < Rt (unsigned)
    if (!_Rd_) return;

    if (IsConst(_Rs_) && IsConst(_Rt_)) {
        MapConst(_Rd_, iRegs[_Rs_].k < iRegs[_Rt_].k);
    } else {
        SUBFC(PutHWReg32(_Rd_), GetHWReg32(_Rt_), GetHWReg32(_Rs_));
        SUBFE(PutHWReg32(_Rd_), GetHWReg32(_Rd_), GetHWReg32(_Rd_));
        NEG(PutHWReg32(_Rd_), GetHWReg32(_Rd_));
    }
}

//End of * Register arithmetic

/* - mult/div & Register trap logic - */
/*********************************************************
* Register mult/div & Register trap logic                *
* Format:  OP rs, rt                                     *
*********************************************************/

static void recMULT() {
// Lo/Hi = Rs * Rt (signed)
	if ((IsConst(_Rs_) && iRegs[_Rs_].k == 0) ||
		(IsConst(_Rt_) && iRegs[_Rt_].k == 0)) {
		MapConst(REG_LO, 0);
		MapConst(REG_HI, 0);
		return;
	}
	
	if (IsConst(_Rs_) && IsConst(_Rt_)) {
		u64 res = (s64)((s64)(s32)iRegs[_Rs_].k * (s64)(s32)iRegs[_Rt_].k);
		MapConst(REG_LO, (res & 0xffffffff));
		MapConst(REG_HI, ((res >> 32) & 0xffffffff));
		return;
	}
	if (GetHWReg32(_Rs_) && GetHWReg32(_Rt_)) {
		MULLW(PutHWReg32(REG_LO), GetHWReg32(_Rs_), GetHWReg32(_Rt_));
		MULHW(PutHWReg32(REG_HI), GetHWReg32(_Rs_), GetHWReg32(_Rt_));
	}
	else {
		LI(PutHWReg32(REG_LO), 0);
		LI(PutHWReg32(REG_HI), 0);
	}
}

static void recMULTU() {
// Lo/Hi = Rs * Rt (unsigned)
	if ((IsConst(_Rs_) && iRegs[_Rs_].k == 0) ||
		(IsConst(_Rt_) && iRegs[_Rt_].k == 0)) {
		MapConst(REG_LO, 0);
		MapConst(REG_HI, 0);
		return;
	}
	
	if (IsConst(_Rs_) && IsConst(_Rt_)) {
		u64 res = (u64)((u64)(u32)iRegs[_Rs_].k * (u64)(u32)iRegs[_Rt_].k);
		MapConst(REG_LO, (res & 0xffffffff));
		MapConst(REG_HI, ((res >> 32) & 0xffffffff));
		return;
	}
	if (GetHWReg32(_Rs_) && GetHWReg32(_Rt_)) {
		MULLW(PutHWReg32(REG_LO), GetHWReg32(_Rs_), GetHWReg32(_Rt_));
		MULHWU(PutHWReg32(REG_HI), GetHWReg32(_Rs_), GetHWReg32(_Rt_));
	}
	else {
		LI(PutHWReg32(REG_LO), 0);
		LI(PutHWReg32(REG_HI), 0);
	}
}

static void recDIV() {
// Lo/Hi = Rs / Rt (signed)
	if (IsConst(_Rs_) && iRegs[_Rs_].k == 0) {
		MapConst(REG_LO, 0);
		MapConst(REG_HI, 0);
		return;
	}
	if (IsConst(_Rt_) && IsConst(_Rs_)) {
		MapConst(REG_LO, (s32)iRegs[_Rs_].k / (s32)iRegs[_Rt_].k);
		MapConst(REG_HI, (s32)iRegs[_Rs_].k % (s32)iRegs[_Rt_].k);
		return;
	}
	DIVW(PutHWReg32(REG_LO), GetHWReg32(_Rs_), GetHWReg32(_Rt_));
	MULLW(PutHWReg32(REG_HI), GetHWReg32(REG_LO), GetHWReg32(_Rt_));
	SUBF(PutHWReg32(REG_HI), GetHWReg32(REG_HI), GetHWReg32(_Rs_));
}

static void recDIVU() {
// Lo/Hi = Rs / Rt (unsigned)
	if (IsConst(_Rs_) && iRegs[_Rs_].k == 0) {
		MapConst(REG_LO, 0);
		MapConst(REG_HI, 0);
		return;
	}
	if (IsConst(_Rt_) && IsConst(_Rs_)) {
		MapConst(REG_LO, (u32)iRegs[_Rs_].k / (u32)iRegs[_Rt_].k);
		MapConst(REG_HI, (u32)iRegs[_Rs_].k % (u32)iRegs[_Rt_].k);
		return;
	}
	DIVWU(PutHWReg32(REG_LO), GetHWReg32(_Rs_), GetHWReg32(_Rt_));
	MULLW(PutHWReg32(REG_HI), GetHWReg32(REG_LO), GetHWReg32(_Rt_));
	SUBF(PutHWReg32(REG_HI), GetHWReg32(REG_HI), GetHWReg32(_Rs_));
}

//End of * Register mult/div & Register trap logic  

/* - memory access - */

static void preMemRead()
{
	s32 rs;
	
	ReserveArgs(1);
	if (_Rs_ != _Rt_) {
		DisposeHWReg(iRegs[_Rt_].reg);
	}
	rs = GetHWReg32(_Rs_);
	if (rs != 3 || _Imm_ != 0) {
		ADDI(PutHWRegSpecial(ARG1), rs, _Imm_);
	}
	if (_Rs_ == _Rt_) {
		DisposeHWReg(iRegs[_Rt_].reg);
	}
	InvalidateCPURegs();
	//FlushAllHWReg();
}

static void preMemWrite(s32 size)
{
	s32 rs;
	
	ReserveArgs(2);
	rs = GetHWReg32(_Rs_);
	if (rs != 3 || _Imm_ != 0) {
		ADDI(PutHWRegSpecial(ARG1), rs, _Imm_);
	}
	if (size == 1) {
		RLWINM(PutHWRegSpecial(ARG2), GetHWReg32(_Rt_), 0, 24, 31);
	} else if (size == 2) {
		RLWINM(PutHWRegSpecial(ARG2), GetHWReg32(_Rt_), 0, 16, 31);
	} else {
		MR(PutHWRegSpecial(ARG2), GetHWReg32(_Rt_));
	}
	
	InvalidateCPURegs();
	//FlushAllHWReg();
}

static void recLB() {
// Rt = mem[Rs + Im] (signed)
	
    if (IsConst(_Rs_)) {
        u32 addr = iRegs[_Rs_].k + _Imm_;
        s32 t = addr >> 16;
		if (!_Rt_) return;
        if ((t & 0xfff0)  == 0xbfc0) {	// Static BIOS read
            MapConst(_Rt_, psxRs8(addr));
            return;
        }
        if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {	// PSX Mem read
            LIW(PutHWReg32(_Rt_), (u32)&psxM[addr & 0x1fffff]);
            LBZ(PutHWReg32(_Rt_), 0, GetHWReg32(_Rt_));
            EXTSB(PutHWReg32(_Rt_), GetHWReg32(_Rt_));
            return;
        }
        if (t == 0x1f80 && addr < 0x1f801000) {	// PSX Hardware registers read
            LIW(PutHWReg32(_Rt_), (u32)&psxH[addr & 0xfff]);
            LBZ(PutHWReg32(_Rt_), 0, GetHWReg32(_Rt_));
            EXTSB(PutHWReg32(_Rt_), GetHWReg32(_Rt_));
            return;
        }
    }
	
	u32 *endload, *fastload, *endload2;
	preMemRead();
	s32 tmp1 = PutHWRegSpecial(TMP1), tmp2 = PutHWRegSpecial(TMP2);

	// Begin actual PPC generation	
	RLWINM(tmp1, 3, 16, 16, 31);		//r15 = (Addr>>16) & 0xFFFF
	CMPWI(tmp1, 0x1f80);				//If AddrHi == 0x1f80, interpret load
	BNE_L(fastload);
	
	CALLFunc((u32)psxDynaMemRead8);
	B_L(endload);
	B_DST(fastload);
	SLWI(tmp1, tmp1, 2);
	LWZX(tmp2, tmp1, GetHWRegSpecial(PSXRLUT));	// r15 = (char *)(psxMemRLUT[Addr16]);
	CMPWI(tmp2, 0);
	RLWINM(tmp1, 3, 0, 16, 31);				// r14 = (Addr) & 0xFFFF
	LI(3, 0);
	BEQ_L(endload2);
	LBZX(3, tmp2, tmp1);						// rt = swap32(&psxmem + addr&0x1fffff) 

	B_DST(endload);
	B_DST(endload2);
	if (_Rt_) {
		EXTSB(3, 3);
		SetDstCPUReg(3);
		PutHWReg32(_Rt_);
	}
}

static void recLBU() {
// Rt = mem[Rs + Im] (unsigned)

    if (IsConst(_Rs_)) {
        u32 addr = iRegs[_Rs_].k + _Imm_;
        s32 t = addr >> 16;
		if (!_Rt_) return;
        if ((t & 0xfff0)  == 0xbfc0) {	// Static BIOS read
            MapConst(_Rt_, psxRu8(addr));
            return;
        }
        if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {	// PSX Mem read
            LIW(PutHWReg32(_Rt_), (u32)&psxM[addr & 0x1fffff]);
            LBZ(PutHWReg32(_Rt_), 0, GetHWReg32(_Rt_));
            return;
        }
        if (t == 0x1f80 && addr < 0x1f801000) {	// PSX Hardware registers read
            LIW(PutHWReg32(_Rt_), (u32)&psxH[addr & 0xfff]);
            LBZ(PutHWReg32(_Rt_), 0, GetHWReg32(_Rt_));
            return;
        }
    }
	u32 *endload, *fastload, *endload2;
	preMemRead();
	s32 tmp1 = PutHWRegSpecial(TMP1), tmp2 = PutHWRegSpecial(TMP2);
	
	// Begin actual PPC generation	
	RLWINM(tmp1, 3, 16, 16, 31);		//r15 = (Addr>>16) & 0xFFFF
	CMPWI(tmp1, 0x1f80);				//If AddrHi == 0x1f80, interpret load
	BNE_L(fastload);
	
	CALLFunc((u32)psxDynaMemRead8);
	B_L(endload);
	B_DST(fastload);
	SLWI(tmp1, tmp1, 2);
	LWZX(tmp2, tmp1, GetHWRegSpecial(PSXRLUT));	// r15 = (char *)(psxMemRLUT[Addr16]);
	CMPWI(tmp2, 0);
	RLWINM(tmp1, 3, 0, 16, 31);				// r14 = (Addr) & 0xFFFF
	LI(3, 0);
	BEQ_L(endload2);
	LBZX(3, tmp2, tmp1);						// rt = swap32(&psxmem + addr&0x1fffff) 

	B_DST(endload);
	B_DST(endload2);
	if (_Rt_) {
		SetDstCPUReg(3);
		PutHWReg32(_Rt_);
	}
}

static void recLH() {
// Rt = mem[Rs + Im] (signed)

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		s32 t = addr >> 16;
		if (!_Rt_) return;
		if ((t & 0xfff0)  == 0xbfc0) {	// Static BIOS read
			MapConst(_Rt_, psxRs16(addr));
			return;
		}
		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {	// PSX Mem read
			LIW(PutHWReg32(_Rt_), (u32)&psxM[addr & 0x1fffff]);
			LHBRX(PutHWReg32(_Rt_), 0, GetHWReg32(_Rt_));
			EXTSH(PutHWReg32(_Rt_), GetHWReg32(_Rt_));
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {	// PSX Hardware registers read
			LIW(PutHWReg32(_Rt_), (u32)&psxH[addr & 0xfff]);
			LHBRX(PutHWReg32(_Rt_), 0, GetHWReg32(_Rt_));
			EXTSH(PutHWReg32(_Rt_), GetHWReg32(_Rt_));
			return;
		}
	}
    
	u32 *endload, *fastload, *endload2;
	preMemRead();
	s32 tmp1 = PutHWRegSpecial(TMP1), tmp2 = PutHWRegSpecial(TMP2);
	
	// Begin actual PPC generation	
	RLWINM(tmp1, 3, 16, 16, 31);		//r15 = (Addr>>16) & 0xFFFF
	CMPWI(tmp1, 0x1f80);				//If AddrHi == 0x1f80, interpret load
	BNE_L(fastload);
	
	CALLFunc((u32)psxDynaMemRead16);
	B_L(endload);
	B_DST(fastload);
	SLWI(tmp1, tmp1, 2);
	LWZX(tmp2, tmp1, GetHWRegSpecial(PSXRLUT));	// r15 = (char *)(psxMemRLUT[Addr16]);
	CMPWI(tmp2, 0);
	RLWINM(tmp1, 3, 0, 16, 31);					// r14 = (Addr) & 0xFFFF
	LI(3, 0);
	BEQ_L(endload2);
	LHBRX(3, tmp2, tmp1);						// rt = swap32(&psxmem + addr&0x1fffff) 

	B_DST(endload);
	B_DST(endload2);
	if (_Rt_) {
		EXTSH(3, 3);
		SetDstCPUReg(3);
		PutHWReg32(_Rt_);
	}
}

static void recLHU() {
// Rt = mem[Rs + Im] (unsigned)

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		s32 t = addr >> 16;
		if (!_Rt_) return;
	
		if ((t & 0xfff0) == 0xbfc0) {	// Static BIOS read
			MapConst(_Rt_, psxRu16(addr));
			return;
		}
		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {	// PSX Mem read
			LIW(PutHWReg32(_Rt_), (u32)&psxM[addr & 0x1fffff]);
			LHBRX(PutHWReg32(_Rt_), 0, GetHWReg32(_Rt_));
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {	// PSX Hardware registers read
			LIW(PutHWReg32(_Rt_), (u32)&psxH[addr & 0xfff]);
			LHBRX(PutHWReg32(_Rt_), 0, GetHWReg32(_Rt_));
			return;
		}
		if (t == 0x1f80) {
			if (addr >= 0x1f801c00 && addr < 0x1f801e00) {				
					ReserveArgs(1);
					LIW(PutHWRegSpecial(ARG1), addr);
					DisposeHWReg(iRegs[_Rt_].reg);
					InvalidateCPURegs();
					CALLFunc((u32)SPU_readRegister);

					SetDstCPUReg(3);
					PutHWReg32(_Rt_);
					return;
			}
			switch (addr) {
					case 0x1f801100: case 0x1f801110: case 0x1f801120:						
						ReserveArgs(1);
						LIW(PutHWRegSpecial(ARG1), (addr >> 4) & 0x3);
						DisposeHWReg(iRegs[_Rt_].reg);
						InvalidateCPURegs();
						CALLFunc((u32)psxRcntRcount);
						
						SetDstCPUReg(3);
						PutHWReg32(_Rt_);
						return;

					case 0x1f801104: case 0x1f801114: case 0x1f801124:
						ReserveArgs(1);
						LIW(PutHWRegSpecial(ARG1), (addr >> 4) & 0x3);
						DisposeHWReg(iRegs[_Rt_].reg);
						InvalidateCPURegs();
						CALLFunc((u32)psxRcntRmode);
						SetDstCPUReg(3);
						PutHWReg32(_Rt_);
						return;
	
					case 0x1f801108: case 0x1f801118: case 0x1f801128:
						ReserveArgs(1);
						LIW(PutHWRegSpecial(ARG1), (addr >> 4) & 0x3);
						DisposeHWReg(iRegs[_Rt_].reg);
						InvalidateCPURegs();
						CALLFunc((u32)psxRcntRtarget);
						SetDstCPUReg(3);
						PutHWReg32(_Rt_);
						return;
					}
		}
	}

	u32 *endload, *fastload, *endload2;
	preMemRead();
	s32 tmp1 = PutHWRegSpecial(TMP1), tmp2 = PutHWRegSpecial(TMP2);
	// Begin actual PPC generation	
	RLWINM(tmp1, 3, 16, 16, 31);		//r15 = (Addr>>16) & 0xFFFF
	CMPWI(tmp1, 0x1f80);				//If AddrHi == 0x1f80, interpret load
	BNE_L(fastload);
	
	CALLFunc((u32)psxDynaMemRead16);
	B_L(endload);
	B_DST(fastload);
	SLWI(tmp1, tmp1, 2);
	LWZX(tmp2, tmp1, GetHWRegSpecial(PSXRLUT));	// r15 = (char *)(psxMemRLUT[Addr16]);
	CMPWI(tmp2, 0);
	RLWINM(tmp1, 3, 0, 16, 31);					// r14 = (Addr) & 0xFFFF
	LI(3, 0);
	BEQ_L(endload2);
	LHBRX(3, tmp2, tmp1);						// rt = swap32(&psxmem + addr&0x1fffff) 

	B_DST(endload);
	B_DST(endload2);
	if (_Rt_) {
		SetDstCPUReg(3);
		PutHWReg32(_Rt_);
	}
}

static void recLW() {
// Rt = mem[Rs + Im] (unsigned)
	
	if (IsConst(_Rs_)) {
		if (!_Rt_) return;
		u32 addr = iRegs[_Rs_].k + _Imm_;
		s32 t = addr >> 16;

		if ((t & 0xfff0) == 0xbfc0) {	// Static BIOS read
			MapConst(_Rt_, psxRu32(addr));
			return;
		}
		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {	// PSX Mem read
			LIW(PutHWReg32(_Rt_), (u32)&psxM[addr & 0x1fffff]);
			LWBRX(PutHWReg32(_Rt_), 0, GetHWReg32(_Rt_));
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {	// PSX Hardware registers read
			LIW(PutHWReg32(_Rt_), (u32)&psxH[addr & 0xfff]);
			LWBRX(PutHWReg32(_Rt_), 0, GetHWReg32(_Rt_));
			return;
		}
		if (t == 0x1f80) {
			switch (addr) {
				case 0x1f801080: case 0x1f801084: case 0x1f801088: 
				case 0x1f801090: case 0x1f801094: case 0x1f801098: 
				case 0x1f8010a0: case 0x1f8010a4: case 0x1f8010a8: 
				case 0x1f8010b0: case 0x1f8010b4: case 0x1f8010b8: 
				case 0x1f8010c0: case 0x1f8010c4: case 0x1f8010c8: 
				case 0x1f8010d0: case 0x1f8010d4: case 0x1f8010d8: 
				case 0x1f8010e0: case 0x1f8010e4: case 0x1f8010e8: 
				case 0x1f801070: case 0x1f801074:
				case 0x1f8010f0: case 0x1f8010f4:			
					LIW(PutHWReg32(_Rt_), (u32)&psxH[addr & 0xffff]);
					LWBRX(PutHWReg32(_Rt_), 0, GetHWReg32(_Rt_));
					return;

				case 0x1f801810:
					DisposeHWReg(iRegs[_Rt_].reg);
					InvalidateCPURegs();
					CALLFunc((u32)GPU_readData);
					
					SetDstCPUReg(3);
					PutHWReg32(_Rt_);
					return;

				case 0x1f801814:
					DisposeHWReg(iRegs[_Rt_].reg);
					InvalidateCPURegs();
					CALLFunc((u32)GPU_readStatus);
					
					SetDstCPUReg(3);
					PutHWReg32(_Rt_);
					return;
			}
		}
	}
	
	u32 *endload, *fastload, *endload2;
	preMemRead();

	s32 tmp1 = PutHWRegSpecial(TMP1), tmp2 = PutHWRegSpecial(TMP2);
	// Begin actual PPC generation	
	RLWINM(tmp1, 3, 16, 16, 31);		//r15 = (Addr>>16) & 0xFFFF
	CMPWI(tmp1, 0x1f80);				//If AddrHi == 0x1f80, interpret load
	BNE_L(fastload);
	
	CALLFunc((u32)psxDynaMemRead32);
	B_L(endload);
	B_DST(fastload);
	SLWI(tmp1, tmp1, 2);
	LWZX(tmp2, tmp1, GetHWRegSpecial(PSXRLUT));	// r15 = (char *)(psxMemRLUT[Addr16]);
	CMPWI(tmp2, 0);
	RLWINM(tmp1, 3, 0, 16, 31);				// r14 = (Addr) & 0xFFFF
	LI(3, 0);
	BEQ_L(endload2);
	LWBRX(3, tmp2, tmp1);						// rt = swap32(&psxmem + addr&0x1fffff) 

	B_DST(endload);
	B_DST(endload2);
	if (_Rt_) {
		SetDstCPUReg(3);
		PutHWReg32(_Rt_);
	}
}

REC_FUNC(LWL);
REC_FUNC(LWR);
REC_FUNC(SWL);
REC_FUNC(SWR);

static void recSB() {
// mem[Rs + Im] = Rt

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		s32 t = addr >> 16;
		
		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {	// Write to PSX MEM
			LIW(PutHWRegSpecial(PSXMEM), (u32)&psxM[addr & 0x1fffff]);
			STB(GetHWReg32(_Rt_), 0, GetHWRegSpecial(PSXMEM));
			// need to clear mem here
			ReserveArgs(2);
			LIW(PutHWRegSpecial(ARG1), addr & ~3);
			LI(PutHWRegSpecial(ARG2), 1);
			//FlushAllHWReg();
			InvalidateCPURegs();
			CALLFunc((u32)recClear);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {			// Write to Hardware Reg
			LIW(PutHWRegSpecial(PSXMEM), (u32)&psxH[addr & 0xfff]);
			STB(GetHWReg32(_Rt_), 0, GetHWRegSpecial(PSXMEM));
			return;
		}
	}

	preMemWrite(1);
	u32 *endstore, *faststore, *endstore2;
	s32 tmp1 = PutHWRegSpecial(TMP1), tmp2 = PutHWRegSpecial(TMP2), tmp3 = PutHWRegSpecial(TMP3);
	// Begin actual PPC generation	
	RLWINM(tmp1, 3, 16, 16, 31);		//tmp1 = (Addr>>16) & 0xFFFF
	CMPWI(tmp1, 0x1f80);				//If AddrHi == 0x1f80, interpret store
	BNE_L(faststore);
	
	CALLFunc((u32)psxDynaMemWrite8);
	B_L(endstore);
	B_DST(faststore);
	// Lookup the Write LUT, do the store
	SLWI(tmp1, tmp1, 2);
	LIW(tmp3, (u32)&psxMemWLUT);			// TODO remove this if I can get special regs working
	LWZX(tmp2, tmp1, tmp3);					// tmp2 = (char *)(psxMemWLUT[Addr16]);
	CMPWI(tmp2, 0);
	RLWINM(tmp1, 3, 0, 16, 31);				// tmp1 = (Addr) & 0xFFFF
	BEQ_L(endstore2);
	ADD(tmp2, tmp2, tmp1);
	STB(4, 0, tmp2);						// *(u8 *)(p + (mem & 0xffff)) = value;
	// Invalidate Dynarec memory
	LIW(tmp3, (u32)&psxRecLUT);				// TODO remove this if I can get special regs working
	RLWINM(tmp1, 3, 16, 16, 31);			// tmp1 = (Addr>>16) & 0xFFFF
	SLWI(tmp1, tmp1, 2);
	LWZX(tmp2, tmp1, tmp3);
	RLWINM(tmp1, 3, 0, 16, 29);				// tmp1 = (Addr) & 0xFFFC
	ADD(tmp2, tmp2, tmp1);
	LI(tmp3, 0);
	STW(tmp3, 0, tmp2);						// recClear(mem, 1)
	
	B_DST(endstore);
	B_DST(endstore2);
}

static void recSH() {
// mem[Rs + Im] = Rt

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		s32 t = addr >> 16;

		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			LIW(PutHWRegSpecial(PSXMEM), (u32)&psxM[addr & 0x1fffff]);
			STHBRX(GetHWReg32(_Rt_), 0, GetHWRegSpecial(PSXMEM));
			// need to clear mem here
			ReserveArgs(2);
			LIW(PutHWRegSpecial(ARG1), addr & ~3);
			LI(PutHWRegSpecial(ARG2), 1);
			//FlushAllHWReg();
			InvalidateCPURegs();
			CALLFunc((u32)recClear);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			LIW(PutHWRegSpecial(PSXMEM), (u32)&psxH[addr & 0xfff]);
			STHBRX(GetHWReg32(_Rt_), 0, GetHWRegSpecial(PSXMEM));
			return;
		}
	}

	preMemWrite(2);
	u32 *endstore, *faststore, *endstore2;
	s32 tmp1 = PutHWRegSpecial(TMP1), tmp2 = PutHWRegSpecial(TMP2), tmp3 = PutHWRegSpecial(TMP3);
	// Begin actual PPC generation	
	RLWINM(tmp1, 3, 16, 16, 31);		//tmp1 = (Addr>>16) & 0xFFFF
	CMPWI(tmp1, 0x1f80);				//If AddrHi == 0x1f80, interpret store
	BNE_L(faststore);
	
	CALLFunc((u32)psxDynaMemWrite16);
	B_L(endstore);
	B_DST(faststore);
	// Lookup the Write LUT, do the store
	SLWI(tmp1, tmp1, 2);
	LIW(tmp3, (u32)&psxMemWLUT);			// TODO remove this if I can get special regs working
	LWZX(tmp2, tmp1, tmp3);					// tmp2 = (char *)(psxMemWLUT[Addr16]);
	CMPWI(tmp2, 0);
	RLWINM(tmp1, 3, 0, 16, 31);				// tmp1 = (Addr) & 0xFFFF
	BEQ_L(endstore2);
	ADD(tmp2, tmp2, tmp1);
	STHBRX(4, 0, tmp2);						// *(u8 *)(p + (mem & 0xffff)) = value;
	// Invalidate Dynarec memory
	LIW(tmp3, (u32)&psxRecLUT);				// TODO remove this if I can get special regs working
	RLWINM(tmp1, 3, 16, 16, 31);			// tmp1 = (Addr>>16) & 0xFFFF
	SLWI(tmp1, tmp1, 2);
	LWZX(tmp2, tmp1, tmp3);
	RLWINM(tmp1, 3, 0, 16, 29);				// tmp1 = (Addr) & 0xFFFC
	ADD(tmp2, tmp2, tmp1);
	LI(tmp3, 0);
	STW(tmp3, 0, tmp2);						// recClear(mem, 1)
	
	B_DST(endstore);
	B_DST(endstore2);
}

static void recSW() {
// mem[Rs + Im] = Rt

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		s32 t = addr >> 16;

		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			LIW(PutHWRegSpecial(PSXMEM), (u32)&psxM[addr & 0x1fffff]);
			STWBRX(GetHWReg32(_Rt_), 0, GetHWRegSpecial(PSXMEM));
			// need to clear mem here
			ReserveArgs(2);
			LIW(PutHWRegSpecial(ARG1), addr & ~3);
			LI(PutHWRegSpecial(ARG2), 1);
			//FlushAllHWReg();
			InvalidateCPURegs();
			CALLFunc((u32)recClear);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			LIW(PutHWRegSpecial(PSXMEM), (u32)&psxH[addr & 0xfff]);
			STWBRX(GetHWReg32(_Rt_), 0, GetHWRegSpecial(PSXMEM));
			return;
		}
	}

	preMemWrite(4);
	u32 *endstore, *faststore, *endstore2, *endstore3;
	s32 tmp1 = PutHWRegSpecial(TMP1), tmp2 = PutHWRegSpecial(TMP2), tmp3 = PutHWRegSpecial(TMP3);
	// Begin actual PPC generation	
	RLWINM(tmp1, 3, 16, 16, 31);		//tmp1 = (Addr>>16) & 0xFFFF
	CMPWI(tmp1, 0x1f80);				//If AddrHi == 0x1f80, interpret store
	BNE_L(faststore);
	
	CALLFunc((u32)psxMemWrite32);
	B_L(endstore);
	B_DST(faststore);
	// Lookup the Write LUT, do the store
	SLWI(tmp1, tmp1, 2);
	LIW(tmp3, (u32)&psxMemWLUT);			// TODO remove this if I can get special regs working
	LWZX(tmp2, tmp1, tmp3);					// tmp2 = (char *)(psxMemWLUT[Addr16]);
	CMPWI(tmp2, 0);
	RLWINM(tmp1, 3, 0, 16, 31);				// tmp1 = (Addr) & 0xFFFF
	BEQ_L(endstore2);
	ADD(tmp2, tmp2, tmp1);
	STWBRX(4, 0, tmp2);						// *(u8 *)(p + (mem & 0xffff)) = value;
	// Invalidate Dynarec memory
	LIW(tmp3, (u32)&psxRecLUT);				// TODO remove this if I can get special regs working
	RLWINM(tmp1, 3, 16, 16, 31);			// tmp1 = (Addr>>16) & 0xFFFF
	SLWI(tmp1, tmp1, 2);
	LWZX(tmp2, tmp1, tmp3);
	RLWINM(tmp1, 3, 0, 16, 29);				// tmp1 = (Addr) & 0xFFFC
	ADD(tmp2, tmp2, tmp1);
	LI(tmp3, 0);
	STW(tmp3, 0, tmp2);						// recClear(mem, 1)
	B_L(endstore3);
	B_DST(endstore2);
	CALLFunc((u32)psxMemWrite32);
	B_DST(endstore);
	B_DST(endstore3);
}

static void recSLL() {
// Rd = Rt << Sa
    if (!_Rd_) return;

    if (IsConst(_Rt_)) {
        MapConst(_Rd_, iRegs[_Rt_].k << _Sa_);
    } else {
        SLWI(PutHWReg32(_Rd_), GetHWReg32(_Rt_), _Sa_);
    }
}

static void recSRL() {
// Rd = Rt >> Sa
    if (!_Rd_) return;

    if (IsConst(_Rt_)) {
        MapConst(_Rd_, iRegs[_Rt_].k >> _Sa_);
    } else {
        SRWI(PutHWReg32(_Rd_), GetHWReg32(_Rt_), _Sa_);
    }
}

static void recSRA() {
// Rd = Rt >> Sa
    if (!_Rd_) return;

    if (IsConst(_Rt_)) {
        MapConst(_Rd_, (s32)iRegs[_Rt_].k >> _Sa_);
    } else {
        SRAWI(PutHWReg32(_Rd_), GetHWReg32(_Rt_), _Sa_);
    }
}


/* - shift ops - */

static void recSLLV() {
// Rd = Rt << Rs
	if (!_Rd_) return;

	if (IsConst(_Rt_) && IsConst(_Rs_)) {
		MapConst(_Rd_, iRegs[_Rt_].k << iRegs[_Rs_].k);
	} else if (IsConst(_Rs_) && !IsMapped(_Rs_)) {
		SLWI(PutHWReg32(_Rd_), GetHWReg32(_Rt_), iRegs[_Rs_].k);
	} else {
		SLW(PutHWReg32(_Rd_), GetHWReg32(_Rt_), GetHWReg32(_Rs_));
	}
}

static void recSRLV() {
// Rd = Rt >> Rs
	if (!_Rd_) return;

	if (IsConst(_Rt_) && IsConst(_Rs_)) {
		MapConst(_Rd_, iRegs[_Rt_].k >> iRegs[_Rs_].k);
	} else if (IsConst(_Rs_) && !IsMapped(_Rs_)) {
		SRWI(PutHWReg32(_Rd_), GetHWReg32(_Rt_), iRegs[_Rs_].k);
	} else {
		SRW(PutHWReg32(_Rd_), GetHWReg32(_Rt_), GetHWReg32(_Rs_));
	}
}

static void recSRAV() {
// Rd = Rt >> Rs
	if (!_Rd_) return;

	if (IsConst(_Rt_) && IsConst(_Rs_)) {
		MapConst(_Rd_, (s32)iRegs[_Rt_].k >> iRegs[_Rs_].k);
	} else if (IsConst(_Rs_) && !IsMapped(_Rs_)) {
		SRAWI(PutHWReg32(_Rd_), GetHWReg32(_Rt_), iRegs[_Rs_].k);
	} else {
		SRAW(PutHWReg32(_Rd_), GetHWReg32(_Rt_), GetHWReg32(_Rs_));
	}
}

static void recSYSCALL() {
//	dump=1;
	iFlushRegs(0);
	
	ReserveArgs(2);
	LIW(PutHWRegSpecial(PSXPC), pc - 4);
	LIW(PutHWRegSpecial(ARG1), 0x20);
	LIW(PutHWRegSpecial(ARG2), (branch == 1 ? 1 : 0));
	FlushAllHWReg();
	CALLFunc ((u32)psxException);

	branch = 2;
	iRet();
}

static void recBREAK() {
}

static void recMFHI() {
// Rd = Hi
	if (!_Rd_) return;
	
	if (IsConst(REG_HI)) {
		MapConst(_Rd_, iRegs[REG_HI].k);
	} else {
		MapCopy(_Rd_, REG_HI);
	}
}

static void recMTHI() {
// Hi = Rs

	if (IsConst(_Rs_)) {
		MapConst(REG_HI, iRegs[_Rs_].k);
	} else {
		MapCopy(REG_HI, _Rs_);
	}
}

static void recMFLO() {
// Rd = Lo
	if (!_Rd_) return;

	if (IsConst(REG_LO)) {
		MapConst(_Rd_, iRegs[REG_LO].k);
	} else {
		MapCopy(_Rd_, REG_LO);
	}
}

static void recMTLO() {
// Lo = Rs

	if (IsConst(_Rs_)) {
		MapConst(REG_LO, iRegs[_Rs_].k);
	} else {
		MapCopy(REG_LO, _Rs_);
	}
}

/* - branch ops - */

static void recBLTZ() {
// Branch if Rs < 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 *b;

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k < 0) {
			iJump(bpc); return;
		} else {
			iJump(pc+4); return;
		}
	}

	CMPWI(GetHWReg32(_Rs_), 0);
	BLT_L(b);
	
	iBranch(pc+4, 1);

	B_DST(b);
	
	iBranch(bpc, 0);
	pc+=4;
}

static void recBGTZ() {
// Branch if Rs > 0
    u32 bpc = _Imm_ * 4 + pc;
    u32 *b;

    if (IsConst(_Rs_)) {
        if ((s32)iRegs[_Rs_].k > 0) {
            iJump(bpc); return;
        } else {
            iJump(pc+4); return;
        }
    }

    CMPWI(GetHWReg32(_Rs_), 0);
    BGT_L(b);
    
    iBranch(pc+4, 1);

    B_DST(b);

    iBranch(bpc, 0);
    pc+=4;
}

static void recBLTZAL() {
// Branch if Rs < 0
    u32 bpc = _Imm_ * 4 + pc;
    u32 *b;

    if (IsConst(_Rs_)) {
        if ((s32)iRegs[_Rs_].k < 0) {
            MapConst(31, pc + 4);

            iJump(bpc); return;
        } else {
            iJump(pc+4); return;
        }
    }

    CMPWI(GetHWReg32(_Rs_), 0);
    BLT_L(b);
    
    iBranch(pc+4, 1);

    B_DST(b);
    
    MapConst(31, pc + 4);

    iBranch(bpc, 0);
    pc+=4;
}

static void recBGEZAL() {
// Branch if Rs >= 0
    u32 bpc = _Imm_ * 4 + pc;
    u32 *b;

    if (IsConst(_Rs_)) {
        if ((s32)iRegs[_Rs_].k >= 0) {
            MapConst(31, pc + 4);

            iJump(bpc); return;
        } else {
            iJump(pc+4); return;
        }
    }

    CMPWI(GetHWReg32(_Rs_), 0);
    BGE_L(b);
    
    iBranch(pc+4, 1);

    B_DST(b);
    
    MapConst(31, pc + 4);

    iBranch(bpc, 0);
    pc+=4;
}

static void recJ() {
// j target

	iJump(_Target_ * 4 + (pc & 0xf0000000));
}

static void recJAL() {
// jal target
	MapConst(31, pc + 4);

	iJump(_Target_ * 4 + (pc & 0xf0000000));
}

static void recJR() {
// jr Rs

	if (IsConst(_Rs_)) {
		iJump(iRegs[_Rs_].k);
		//LIW(PutHWRegSpecial(TARGET), iRegs[_Rs_].k);
	} else {
		MR(PutHWRegSpecial(TARGET), GetHWReg32(_Rs_));
		SetBranch();
	}
}

static void recJALR() {
// jalr Rs

	if (_Rd_) {
		MapConst(_Rd_, pc + 4);
	}
	
	if (IsConst(_Rs_)) {
		iJump(iRegs[_Rs_].k);
		//LIW(PutHWRegSpecial(TARGET), iRegs[_Rs_].k);
	} else {
		MR(PutHWRegSpecial(TARGET), GetHWReg32(_Rs_));
		SetBranch();
	}
}

static void recBEQ() {
// Branch if Rs == Rt
	u32 bpc = _Imm_ * 4 + pc;
	u32 *b;

	if (_Rs_ == _Rt_) {
		iJump(bpc);
	}
	else {
		if (IsConst(_Rs_) && IsConst(_Rt_)) {
			if (iRegs[_Rs_].k == iRegs[_Rt_].k) {
				iJump(bpc); return;
			} else {
				iJump(pc+4); return;
			}
		}
		else if (IsConst(_Rs_) && !IsMapped(_Rs_)) {
			if ((iRegs[_Rs_].k & 0xffff) == iRegs[_Rs_].k) {
				CMPLWI(GetHWReg32(_Rt_), iRegs[_Rs_].k);
			}
			else if ((s32)(s16)iRegs[_Rs_].k == (s32)iRegs[_Rs_].k) {
				CMPWI(GetHWReg32(_Rt_), iRegs[_Rs_].k);
			}
			else {
				CMPLW(GetHWReg32(_Rs_), GetHWReg32(_Rt_));
			}
		}
		else if (IsConst(_Rt_) && !IsMapped(_Rt_)) {
			if ((iRegs[_Rt_].k & 0xffff) == iRegs[_Rt_].k) {
				CMPLWI(GetHWReg32(_Rs_), iRegs[_Rt_].k);
			}
			else if ((s32)(s16)iRegs[_Rt_].k == (s32)iRegs[_Rt_].k) {
				CMPWI(GetHWReg32(_Rs_), iRegs[_Rt_].k);
			}
			else {
				CMPLW(GetHWReg32(_Rs_), GetHWReg32(_Rt_));
			}
		}
		else {
			CMPLW(GetHWReg32(_Rs_), GetHWReg32(_Rt_));
		}
		
		BEQ_L(b);
		
		iBranch(pc+4, 1);
	
		B_DST(b);
		
		iBranch(bpc, 0);
		pc+=4;
	}
}

static void recBNE() {
// Branch if Rs != Rt
	u32 bpc = _Imm_ * 4 + pc;
	u32 *b;

	if (_Rs_ == _Rt_) {
		iJump(pc+4);
	}
	else {
		if (IsConst(_Rs_) && IsConst(_Rt_)) {
			if (iRegs[_Rs_].k != iRegs[_Rt_].k) {
				iJump(bpc); return;
			} else {
				iJump(pc+4); return;
			}
		}
		else if (IsConst(_Rs_) && !IsMapped(_Rs_)) {
			if ((iRegs[_Rs_].k & 0xffff) == iRegs[_Rs_].k) {
				CMPLWI(GetHWReg32(_Rt_), iRegs[_Rs_].k);
			}
			else if ((s32)(s16)iRegs[_Rs_].k == (s32)iRegs[_Rs_].k) {
				CMPWI(GetHWReg32(_Rt_), iRegs[_Rs_].k);
			}
			else {
				CMPLW(GetHWReg32(_Rs_), GetHWReg32(_Rt_));
			}
		}
		else if (IsConst(_Rt_) && !IsMapped(_Rt_)) {
			if ((iRegs[_Rt_].k & 0xffff) == iRegs[_Rt_].k) {
				CMPLWI(GetHWReg32(_Rs_), iRegs[_Rt_].k);
			}
			else if ((s32)(s16)iRegs[_Rt_].k == (s32)iRegs[_Rt_].k) {
				CMPWI(GetHWReg32(_Rs_), iRegs[_Rt_].k);
			}
			else {
				CMPLW(GetHWReg32(_Rs_), GetHWReg32(_Rt_));
			}
		}
		else {
			CMPLW(GetHWReg32(_Rs_), GetHWReg32(_Rt_));
		}
		
		BNE_L(b);
		
		iBranch(pc+4, 1);
	
		B_DST(b);
		
		iBranch(bpc, 0);
		pc+=4;
	}
}

static void recBLEZ() {
// Branch if Rs <= 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 *b;

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k <= 0) {
			iJump(bpc); return;
		} else {
			iJump(pc+4); return;
		}
	}

	CMPWI(GetHWReg32(_Rs_), 0);
	BLE_L(b);
	
	iBranch(pc+4, 1);

	B_DST(b);
	
	iBranch(bpc, 0);
	pc+=4;
}

static void recBGEZ() {
// Branch if Rs >= 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 *b;

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k >= 0) {
			iJump(bpc); return;
		} else {
			iJump(pc+4); return;
		}
	}

	CMPWI(GetHWReg32(_Rs_), 0);
	BGE_L(b);
	
	iBranch(pc+4, 1);

	B_DST(b);
	
	iBranch(bpc, 0);
	pc+=4;
}

static void recRFE() {
	
	s32 tmp1 = PutHWRegSpecial(TMP1), tmp2 = PutHWRegSpecial(TMP2);
	
	LWZ(tmp1, OFFSET(&psxRegs, &psxRegs.CP0.n.Status), GetHWRegSpecial(PSXREGS)); //tmp1 = psxRegs.CP0.n.Status
	RLWINM(tmp2,tmp1,0,0,27); // tmp2 = tmp1 & 0xfffffff0
	RLWINM(tmp1,tmp1,(32-2),28,31); // tmp1 = ((tmp1 & 0x3c) >> 2)
	OR(tmp1, tmp2, tmp1);	// tmp1 = tmp2 | tmp1
	STW(tmp1, OFFSET(&psxRegs, &psxRegs.CP0.n.Status), GetHWRegSpecial(PSXREGS));
	
	iFlushRegs(0);
	LIW(PutHWRegSpecial(PSXPC), (u32)pc);
	FlushAllHWReg();
	CALLFunc((u32)psxTestSWInts);
	if(branch == 0) {
		branch = 2;
		iRet();
	}
}

static void recMFC0() {
// Rt = Cop0->Rd
	if (!_Rt_) return;
	
	LWZ(PutHWReg32(_Rt_), OFFSET(&psxRegs, &psxRegs.CP0.r[_Rd_]), GetHWRegSpecial(PSXREGS));
}

static void recCFC0() {
// Rt = Cop0->Rd

	recMFC0();
}

static void recMTC0() {
// Cop0->Rd = Rt

	/*if (IsConst(_Rt_)) {
		switch (_Rd_) {
			case 12:
				MOV32ItoM((u32)&psxRegs.CP0.r[_Rd_], iRegs[_Rt_].k);
				break;
			case 13:
				MOV32ItoM((u32)&psxRegs.CP0.r[_Rd_], iRegs[_Rt_].k & ~(0xfc00));
				break;
			default:
				MOV32ItoM((u32)&psxRegs.CP0.r[_Rd_], iRegs[_Rt_].k);
				break;
		}
	} else*/ {
		switch (_Rd_) {
			case 13:
				RLWINM(0,GetHWReg32(_Rt_),0,22,15); // & ~(0xfc00)
				STW(0, OFFSET(&psxRegs, &psxRegs.CP0.r[_Rd_]), GetHWRegSpecial(PSXREGS));
				break;
			default:
				STW(GetHWReg32(_Rt_), OFFSET(&psxRegs, &psxRegs.CP0.r[_Rd_]), GetHWRegSpecial(PSXREGS));
				break;
		}
	}

	if (_Rd_ == 12 || _Rd_ == 13) {
		iFlushRegs(0);
		LIW(PutHWRegSpecial(PSXPC), (u32)pc);
		FlushAllHWReg();
		CALLFunc((u32)psxTestSWInts);
		/*if(_Rd_ == 12) {
		  LWZ(0, OFFSET(&psxRegs, &psxRegs.interrupt), GetHWRegSpecial(PSXREGS));
		  ORIS(0, 0, 0x8000);
		  STW(0, OFFSET(&psxRegs, &psxRegs.interrupt), GetHWRegSpecial(PSXREGS));
		}*/
		if (branch == 0) {
			branch = 2;
			iRet();
		}
	}
}

static void recCTC0() {
// Cop0->Rd = Rt

	recMTC0();
}

// GTE function callers
CP2_FUNC(MFC2);
CP2_FUNC(MTC2);
CP2_FUNC(CFC2);
CP2_FUNC(CTC2);
CP2_FUNC(LWC2);
CP2_FUNC(SWC2);

CP2_FUNCNC(RTPS);
CP2_FUNC(OP);
CP2_FUNCNC(NCLIP);
CP2_FUNCNC(DPCS);
CP2_FUNCNC(INTPL);
CP2_FUNC(MVMVA);
CP2_FUNCNC(NCDS);
CP2_FUNCNC(NCDT);
CP2_FUNCNC(CDP);
CP2_FUNCNC(NCCS);
CP2_FUNCNC(CC);
CP2_FUNCNC(NCS);
CP2_FUNCNC(NCT);
CP2_FUNC(SQR);
CP2_FUNCNC(DCPL);
CP2_FUNCNC(DPCT);
CP2_FUNCNC(AVSZ3);
CP2_FUNCNC(AVSZ4);
CP2_FUNCNC(RTPT);
CP2_FUNC(GPF);
CP2_FUNC(GPL);
CP2_FUNCNC(NCCT);

static void recHLE() {
	iFlushRegs(0);
	FlushAllHWReg();
	
	if ((psxRegs.code & 0x3ffffff) == (psxRegs.code & 0x7)) {
		CALLFunc((u32)psxHLEt[psxRegs.code & 0x7]);
	} else {
		// somebody else must have written to current opcode for this to happen!!!!
		CALLFunc((u32)psxHLEt[0]); // call dummy function
	}
	
	count = ((pc - pcold)/4 + 20)<<1;
	ADDI(PutHWRegSpecial(CYCLECOUNT), GetHWRegSpecial(CYCLECOUNT), count);
	FlushAllHWReg();
	CALLFunc((u32)psxBranchTest);
	Return();
	
	branch = 2;
}

static void (*recBSC[64])() = {
	recSPECIAL, recREGIMM, recJ   , recJAL  , recBEQ , recBNE , recBLEZ, recBGTZ,
	recADDI   , recADDIU , recSLTI, recSLTIU, recANDI, recORI , recXORI, recLUI ,
	recCOP0   , recNULL  , recCOP2, recNULL , recNULL, recNULL, recNULL, recNULL,
	recNULL   , recNULL  , recNULL, recNULL , recNULL, recNULL, recNULL, recNULL,
	recLB     , recLH    , recLWL , recLW   , recLBU , recLHU , recLWR , recNULL,
	recSB     , recSH    , recSWL , recSW   , recNULL, recNULL, recSWR , recNULL,
	recNULL   , recNULL  , recLWC2, recNULL , recNULL, recNULL, recNULL, recNULL,
	recNULL   , recNULL  , recSWC2, recHLE  , recNULL, recNULL, recNULL, recNULL
};

static void (*recSPC[64])() = {
	recSLL , recNULL, recSRL , recSRA , recSLLV   , recNULL , recSRLV, recSRAV,
	recJR  , recJALR, recNULL, recNULL, recSYSCALL, recBREAK, recNULL, recNULL,
	recMFHI, recMTHI, recMFLO, recMTLO, recNULL   , recNULL , recNULL, recNULL,
	recMULT, recMULTU, recDIV, recDIVU, recNULL   , recNULL , recNULL, recNULL,
	recADD , recADDU, recSUB , recSUBU, recAND    , recOR   , recXOR , recNOR ,
	recNULL, recNULL, recSLT , recSLTU, recNULL   , recNULL , recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL   , recNULL , recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL   , recNULL , recNULL, recNULL
};

static void (*recREG[32])() = {
	recBLTZ  , recBGEZ  , recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL  , recNULL  , recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recBLTZAL, recBGEZAL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL  , recNULL  , recNULL, recNULL, recNULL, recNULL, recNULL, recNULL
};

static void (*recCP0[32])() = {
	recMFC0, recNULL, recCFC0, recNULL, recMTC0, recNULL, recCTC0, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recRFE , recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL
};

static void (*recCP2[64])() = {
	recBASIC, recRTPS , recNULL , recNULL, recNULL, recNULL , recNCLIP, recNULL, // 00
	recNULL , recNULL , recNULL , recNULL, recOP  , recNULL , recNULL , recNULL, // 08
	recDPCS , recINTPL, recMVMVA, recNCDS, recCDP , recNULL , recNCDT , recNULL, // 10
	recNULL , recNULL , recNULL , recNCCS, recCC  , recNULL , recNCS  , recNULL, // 18
	recNCT  , recNULL , recNULL , recNULL, recNULL, recNULL , recNULL , recNULL, // 20
	recSQR  , recDCPL , recDPCT , recNULL, recNULL, recAVSZ3, recAVSZ4, recNULL, // 28 
	recRTPT , recNULL , recNULL , recNULL, recNULL, recNULL , recNULL , recNULL, // 30
	recNULL , recNULL , recNULL , recNULL, recNULL, recGPF  , recGPL  , recNCCT  // 38
};

static void (*recCP2BSC[32])() = {
	recMFC2, recNULL, recCFC2, recNULL, recMTC2, recNULL, recCTC2, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL
};

static void recRecompile() {
	char *p;
	u32 *ptr;
	s32 i;
	

	// initialize state variables
	UniqueRegAlloc = 1;
	HWRegUseCount = 0;
	DstCPUReg = -1;
	memset(HWRegisters, 0, sizeof(HWRegisters));
	for (i=0; i<NUM_HW_REGISTERS; i++)
		HWRegisters[i].code = cpuHWRegisters[NUM_HW_REGISTERS-i-1];
	
	// reserve the special psxReg register
	HWRegisters[0].usage = HWUSAGE_SPECIAL | HWUSAGE_RESERVED | HWUSAGE_HARDWIRED;
	HWRegisters[0].private = PSXREGS;
	HWRegisters[0].k = (u32)&psxRegs;

	HWRegisters[1].usage = HWUSAGE_SPECIAL | HWUSAGE_RESERVED | HWUSAGE_HARDWIRED;
	HWRegisters[1].private = PSXMEM;
	HWRegisters[1].k = (u32)&psxM;
	
	HWRegisters[2].usage = HWUSAGE_SPECIAL | HWUSAGE_RESERVED | HWUSAGE_HARDWIRED;
	HWRegisters[2].private = PSXRLUT;
	HWRegisters[2].k = (u32)&psxMemRLUT;

	// reserve the special psxRegs.cycle register
	//HWRegisters[1].usage = HWUSAGE_SPECIAL | HWUSAGE_RESERVED | HWUSAGE_HARDWIRED;
	//HWRegisters[1].private = CYCLECOUNT;
	
	//memset(iRegs, 0, sizeof(iRegs));
	for (i=0; i<NUM_REGISTERS; i++) {
		iRegs[i].state = ST_UNK;
		iRegs[i].reg = -1;
	}
	iRegs[0].k = 0;
	iRegs[0].state = ST_CONST;
	
	/* if ppcPtr reached the mem limit reset whole mem */
	if (((u32)ppcPtr - (u32)recMem) >= (RECMEM_SIZE - 0x10000)) // fix me. don't just assume 0x10000
		recReset();
#ifdef TAG_CODE
	ppcAlign();
#endif
	ptr = ppcPtr;
		
	// tell the LUT where to find us
	PC_REC32(psxRegs.pc) = (u32)ppcPtr;

	pcold = pc = psxRegs.pc;
	
	//where did 500 come from?
	for (count=0; count<500;) {
		p = (char *)PSXM(pc);
		if (p == NULL) recError();
		psxRegs.code = SWAP32(*(u32 *)p);
		//print_gecko("%s\r\n", disR3000AF(psxRegs.code, pc)); 
		pc+=4; count++;
		recBSC[psxRegs.code>>26]();

		if (branch) {
			branch = 0;
			break;
		}
	}
	if(!branch) {
		iFlushRegs(pc);
		LIW(PutHWRegSpecial(PSXPC), pc);
		iRet();
	}

  DCFlushRange((u8*)ptr,(u32)(u8*)ppcPtr-(u32)(u8*)ptr);
  ICInvalidateRange((u8*)ptr,(u32)(u8*)ppcPtr-(u32)(u8*)ptr);
  
#ifdef TAG_CODE
	sprintf((char *)ppcPtr, "PC=%08x", pcold);  //causes misalignment
	ppcPtr += strlen((char *)ppcPtr);
#endif
	dyna_used = ((u32)ppcPtr - (u32)recMem)/1024;
}


R3000Acpu psxRec = {
	recInit,
	recReset,
	recExecute,
	recExecuteBlock,
	recClear,
	recShutdown
};

