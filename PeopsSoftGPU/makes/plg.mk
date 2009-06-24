#
# plugin specific makefile
#

PLUGIN = libgpuPeops$(VERSION).so
PLUGINTYPE = libgpu.so
CFLAGS = -g -Wall -fPIC -O4 -fomit-frame-pointer -ffast-math $(INCLUDE)
#CFLAGS = -g -Wall -fPIC -O3 -mpentium -fomit-frame-pointer -ffast-math $(INCLUDE)
INCLUDE = -I/usr/local/include
OBJECTS = gpu.o cfg.o draw.o fps.o key.o menu.o prim.o soft.o zn.o hq3x32.o hq2x32.o hq3x16.o hq2x16.o
LIBS =
