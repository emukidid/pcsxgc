/* 	NULL GFX for cubeSX by emu_kidid

*/

#ifdef __SWITCH__
#include <switch.h>
#else
#include <gccore.h>
#endif
#include <stdint.h>
#include <sys/types.h>
#include "../plugins.h"

s32 GPU__open(void) { return 0; }
s32 GPU__init(void) { return 0; }
s32 GPU__shutdown(void) { return 0; }
s32 GPU__close(void) { return 0; }
void GPU__writeStatus(u32 a){}
void GPU__writeData(u32 a){}
u32 GPU__readStatus(void) { return 0; }
u32 GPU__readData(void) { return 0; }
s32 GPU__dmaChain(u32 *a ,u32 b) { return 0; }
void GPU__updateLace(void) { }
