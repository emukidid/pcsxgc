##############################################################################
# MAKEFILE FOR PETE'S MESAGL GPU... just run "make"
##############################################################################

##############################################################################
# 1. SETS (CCFLAGS3 is used)
##############################################################################

CC = gcc
CCFLAGS1 = -fPIC -c -Wall
CCFLAGS2 = -fPIC -c -Wall -m486 -O2 -ffast-math

#CCFLAGS3 = -fPIC -c -Wall -mpentium -O3 -ffast-math -fomit-frame-pointer
CCFLAGS3 = -fPIC -c -Wall -m32 -mcpu=pentium -O3 -ffast-math -fomit-frame-pointer


INCLUDE =
LINK = gcc
LINKFLAGS = -shared -m32 -Wl,-soname,libgpuPeopsMesaGL.so -o libgpuPeopsMesaGL.so.1.0.78
OBJ =   gpu.o cfg.o draw.o menu.o fps.o key.o texture.o prim.o soft.o zn.o /usr/X11R6/lib/libXxf86vm.a
#LIB = -lc -lMesaGL -lX11 -lXext -lm -L/usr/X11R6/lib -L/usr/lib -lXxf86vm
LIB = -lc -lGL -lX11 -lXext -lm -L/usr/X11R6/lib -L/usr/lib -lpthread

##############################################################################
# 2. MAIN RULE
##############################################################################

gpuPeopsMesaGL :	$(OBJ)
		$(LINK) $(LINKFLAGS) $(OBJ) $(LIB)

##############################################################################
# 3. GENERAL RULES
##############################################################################

%.o     : %.c
	$(CC) $(CCFLAGS3) $(INCLUDE) $<

##############################################################################
# 4. SPECIFIC RULES
##############################################################################

gpu.o  : gpu.c stdafx.h externals.h gpu.h draw.h cfg.h prim.h psemu.h texture.h menu.h fps.h
cfg.o  : cfg.c stdafx.h externals.h cfg.h
draw.o : draw.c stdafx.h externals.h gpu.h draw.h prim.h texture.h menu.h
menu.o  : menu.c stdafx.h externals.h draw.h menu.h gpu.h
fps.o  : fps.c stdafx.h externals.h
key.o  : key.c stdafx.h externals.h draw.h menu.h texture.h fps.h
texture.o : texture.c stdafx.h externals.h texture.h gpu.h prim.h
prim.o : prim.c stdafx.h externals.h gpu.h draw.h soft.h texture.h
soft.o : soft.c stdafx.h externals.h gpu.h soft.h prim.h
zn.o : zn.c stdafx.h externals.h gpu.h
