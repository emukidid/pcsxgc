#
# Makefile for Peops soft gpu
#

include ./makes/plg.mk
include ./makes/mk.x11
#include ./makes/mk.fpse

# Dependencies

cfg.o: cfg.c stdafx.h externals.h cfg.h gpu.h
draw.o: draw.c stdafx.h externals.h gpu.h draw.h prim.h menu.h
fps.o: fps.c stdafx.h externals.h fps.h
fpsewp.o: fpsewp.c stdafx.h fpse/type.h fpse/sdk.h fpse/linuxdef.h \
 fpsewp.h externals.h
gpu.o: gpu.c stdafx.h externals.h gpu.h draw.h cfg.h prim.h psemu.h \
 menu.h key.h fps.h
gpupeopssoft.o: gpupeopssoft.c stdafx.h
key.o: key.c stdafx.h externals.h menu.h gpu.h draw.h key.h
menu.o: menu.c stdafx.h externals.h draw.h menu.h gpu.h
prim.o: prim.c stdafx.h externals.h gpu.h draw.h soft.h
record.o: record.c stdafx.h externals.h record.h gpu.h
soft.o: soft.c stdafx.h externals.h gpu.h soft.h prim.h menu.h
zn.o: zn.c stdafx.h externals.h
hq3x32.o: hq3x32.asm
hq2x32.o: hq2x32.asm
hq3x16.o: hq3x16.asm
hq2x16.o: hq2x16.asm

