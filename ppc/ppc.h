/*
 * ppc definitions v0.5.1
 *  Authors: linuzappz <linuzappz@pcsx.net>
 *           alexey silinov
 */

#ifndef __PPC_H__
#define __PPC_H__

// include basic types
#include "../PsxCommon.h"
#include "ppc_mnemonics.h"

#define NUM_HW_REGISTERS 28

/* general defines */

#define CALLFunc(FUNC) \
{ \
    u32 _func = (FUNC); \
    ReleaseArgs(); \
    if ((_func & 0x1fffffc) == _func) { \
        BLA(_func); \
    } else { \
        LIW(0, _func); \
        MTCTR(0); \
        BCTRL(); \
    } \
}

extern int cpuHWRegisters[NUM_HW_REGISTERS];

extern u32 *ppcPtr;

void ppcInit();
void ppcSetPtr(u32 *ptr);
void ppcShutdown();

void returnPC();
void recRun(void (*func)(), u32 hw1, u32 hw2, u32 hw3, u32 hw4);
u8 psxDynaMemRead8(u32 mem);
u16 psxDynaMemRead16(u32 mem);
u32 psxDynaMemRead32(u32 mem);
void psxDynaMemWrite8(u32 mem, u8 val);
void psxDynaMemWrite32(u32 mem, u32 value);
#endif /* __PPC_H__ */





















