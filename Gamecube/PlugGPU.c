/* 	NULL GFX for cubeSX by emu_kidid

*/

#include <gccore.h>
#include <stdint.h>
#include <sys/types.h>
#include <ogc/pad.h>
#include <libpcsxcore/plugins.h>

long GPU__open(void) { return 0; }
long GPU__init(void) { return 0; }
long GPU__shutdown(void) { return 0; }
long GPU__close(void) { return 0; }
void GPU__writeStatus(unsigned long a){}
void GPU__writeData(unsigned long a){}
unsigned long GPU__readStatus(void) { return 0; }
unsigned long GPU__readData(void) { return 0; }
long GPU__dmaChain(unsigned long *a ,unsigned long b) { return 0; }
void GPU__updateLace(void) { }
