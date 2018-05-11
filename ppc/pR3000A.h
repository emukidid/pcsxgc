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

#ifndef _pR3000A_H_
#define _pR3000A_H_
 
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
#include "../R3000A.h"
#include "../PsxHLE.h"
#include "../Gamecube/DEBUG.h"

/* defines */
#ifdef HW_DOL
#define RECMEM_SIZE		(6*1024*1024)
#elif HW_RVL
#define RECMEM_SIZE		(8*1024*1024)
#endif
#define NUM_REGISTERS	34
#undef _Op_
#define _Op_     _fOp_(psxRegs.code)
#undef _Funct_
#define _Funct_  _fFunct_(psxRegs.code)
#undef _Rd_
#define _Rd_     _fRd_(psxRegs.code)
#undef _Rt_
#define _Rt_     _fRt_(psxRegs.code)
#undef _Rs_
#define _Rs_     _fRs_(psxRegs.code)
#undef _Sa_
#define _Sa_     _fSa_(psxRegs.code)
#undef _Im_
#define _Im_     _fIm_(psxRegs.code)
#undef _Target_
#define _Target_ _fTarget_(psxRegs.code)

#undef _Imm_
#define _Imm_	 _fImm_(psxRegs.code)
#undef _ImmU_
#define _ImmU_	 _fImmU_(psxRegs.code)

#undef PC_REC
#undef PC_REC8
#undef PC_REC16
#undef PC_REC32
#define PC_REC(x)	(psxRecLUT[x >> 16] + (x & 0xffff))
#define PC_REC8(x)	(*(u8 *)PC_REC(x))
#define PC_REC16(x) (*(u16*)PC_REC(x))
#define PC_REC32(x) (*(u32*)PC_REC(x))
#define OFFSET(X,Y) ((u32)(Y)-(u32)(X))

#define ST_UNK      0x00
#define ST_CONST    0x01
#define ST_MAPPED   0x02

//#define NO_CONSTANT
#ifdef NO_CONSTANT
#define IsConst(reg) 0
#else
#define IsConst(reg)  (iRegs[reg].state & ST_CONST)
#endif
#define IsMapped(reg) (iRegs[reg].state & ST_MAPPED)


#define REG_LO			32
#define REG_HI			33

// Hardware register usage
#define HWUSAGE_NONE     0x00

#define HWUSAGE_READ     0x01
#define HWUSAGE_WRITE    0x02
#define HWUSAGE_CONST    0x04
#define HWUSAGE_ARG      0x08	/* used as an argument for a function call */

#define HWUSAGE_RESERVED 0x10	/* won't get flushed when flushing all regs */
#define HWUSAGE_SPECIAL  0x20	/* special purpose register */
#define HWUSAGE_HARDWIRED 0x40	/* specific hardware register mapping that is never disposed */
#define HWUSAGE_INITED    0x80
#define HWUSAGE_PSXREG    0x100

/* externals */
extern void SysRunGui();
extern void SysMessage(char *fmt, ...);
extern void SysReset();
extern void SysPrintf(char *fmt, ...);
extern s32 stop;

/* structs */
typedef struct {
	s32 state;
	u32 k;
	s32 reg;
} iRegisters;

// Remember to invalidate the special registers if they are modified by compiler
enum {
    ARG1 = 3,
    ARG2 = 4,
    ARG3 = 5,
	TMP1 = 6,
	TMP2 = 7,
    PSXREGS,	// ptr
	PSXMEM,		// ptr
	PSXRLUT,	// ptr
	PSXWLUT,	// ptr
	PSXRECLUT,	// ptr
    CYCLECOUNT,	// ptr
    PSXPC,		// ptr
    TARGETPTR,	// ptr
    TARGET,		// ptr
    RETVAL,
    REG_RZERO,
    REG_WZERO
};

typedef struct {
    s32 code;
    u32 k;
    s32 usage;
    s32 lastUsed;
    
    void (*flush)(s32 hwreg);
    s32 private;
} HWRegister;

#endif
