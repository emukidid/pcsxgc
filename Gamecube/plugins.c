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
* Plugin library callback/access functions.
*/

#include "../plugins.h"
#include "../cdriso.h"

static char IsoFile[MAXPATHLEN] = "";
static s64 cdOpenCaseTime = 0;

GPUupdateLace         GPU_updateLace;
GPUinit               GPU_init;
GPUshutdown           GPU_shutdown;
GPUconfigure          GPU_configure;
GPUtest               GPU_test;
GPUabout              GPU_about;
GPUopen               GPU_open;
GPUclose              GPU_close;
GPUreadStatus         GPU_readStatus;
GPUreadData           GPU_readData;
GPUreadDataMem        GPU_readDataMem;
GPUwriteStatus        GPU_writeStatus;
GPUwriteData          GPU_writeData;
GPUwriteDataMem       GPU_writeDataMem;
GPUdmaChain           GPU_dmaChain;
GPUkeypressed         GPU_keypressed;
GPUdisplayText        GPU_displayText;
GPUmakeSnapshot       GPU_makeSnapshot;
GPUfreeze             GPU_freeze;
GPUgetScreenPic       GPU_getScreenPic;
GPUshowScreenPic      GPU_showScreenPic;
GPUclearDynarec       GPU_clearDynarec;
GPUvBlank             GPU_vBlank;
GPUregisterCallback   GPU_registerCallback;
GPUidle               GPU_idle;
GPUvisualVibration    GPU_visualVibration;
GPUcursor             GPU_cursor;

CDRinit               CDR_init;
CDRshutdown           CDR_shutdown;
CDRopen               CDR_open;
CDRclose              CDR_close;
CDRtest               CDR_test;
CDRgetTN              CDR_getTN;
CDRgetTD              CDR_getTD;
CDRreadTrack          CDR_readTrack;
CDRgetBuffer          CDR_getBuffer;
CDRplay               CDR_play;
CDRstop               CDR_stop;
CDRgetStatus          CDR_getStatus;
CDRgetDriveLetter     CDR_getDriveLetter;
CDRgetBufferSub       CDR_getBufferSub;
CDRconfigure          CDR_configure;
CDRabout              CDR_about;
CDRsetfilename        CDR_setfilename;
CDRreadCDDA           CDR_readCDDA;
CDRgetTE              CDR_getTE;

SPUconfigure          SPU_configure;
SPUabout              SPU_about;
SPUinit               SPU_init;
SPUshutdown           SPU_shutdown;
SPUtest               SPU_test;
SPUopen               SPU_open;
SPUclose              SPU_close;
SPUplaySample         SPU_playSample;
SPUwriteRegister      SPU_writeRegister;
SPUreadRegister       SPU_readRegister;
SPUwriteDMA           SPU_writeDMA;
SPUreadDMA            SPU_readDMA;
SPUwriteDMAMem        SPU_writeDMAMem;
SPUreadDMAMem         SPU_readDMAMem;
SPUplayADPCMchannel   SPU_playADPCMchannel;
SPUfreeze             SPU_freeze;
SPUregisterCallback   SPU_registerCallback;
SPUasync              SPU_async;
SPUplayCDDAchannel    SPU_playCDDAchannel;

PADconfigure          PAD1_configure;
PADabout              PAD1_about;
PADinit               PAD1_init;
PADshutdown           PAD1_shutdown;
PADtest               PAD1_test;
PADopen               PAD1_open;
PADclose              PAD1_close;
PADquery              PAD1_query;
PADreadPort1          PAD1_readPort1;
PADkeypressed         PAD1_keypressed;
PADstartPoll          PAD1_startPoll;
PADpoll               PAD1_poll;
PADsetSensitive       PAD1_setSensitive;
PADregisterVibration  PAD1_registerVibration;
PADregisterCursor     PAD1_registerCursor;

PADconfigure          PAD2_configure;
PADabout              PAD2_about;
PADinit               PAD2_init;
PADshutdown           PAD2_shutdown;
PADtest               PAD2_test;
PADopen               PAD2_open;
PADclose              PAD2_close;
PADquery              PAD2_query;
PADreadPort2          PAD2_readPort2;
PADkeypressed         PAD2_keypressed;
PADstartPoll          PAD2_startPoll;
PADpoll               PAD2_poll;
PADsetSensitive       PAD2_setSensitive;
PADregisterVibration  PAD2_registerVibration;
PADregisterCursor     PAD2_registerCursor;

NETinit               NET_init;
NETshutdown           NET_shutdown;
NETopen               NET_open;
NETclose              NET_close;
NETtest               NET_test;
NETconfigure          NET_configure;
NETabout              NET_about;
NETpause              NET_pause;
NETresume             NET_resume;
NETqueryPlayer        NET_queryPlayer;
NETsendData           NET_sendData;
NETrecvData           NET_recvData;
NETsendPadData        NET_sendPadData;
NETrecvPadData        NET_recvPadData;
NETsetInfo            NET_setInfo;
NETkeypressed         NET_keypressed;

#ifdef ENABLE_SIO1API

SIO1init              SIO1_init;
SIO1shutdown          SIO1_shutdown;
SIO1open              SIO1_open;
SIO1close             SIO1_close;
SIO1test              SIO1_test;
SIO1configure         SIO1_configure;
SIO1about             SIO1_about;
SIO1pause             SIO1_pause;
SIO1resume            SIO1_resume;
SIO1keypressed        SIO1_keypressed;
SIO1writeData8        SIO1_writeData8;
SIO1writeData16       SIO1_writeData16;
SIO1writeData32       SIO1_writeData32;
SIO1writeStat16       SIO1_writeStat16;
SIO1writeStat32       SIO1_writeStat32;
SIO1writeMode16       SIO1_writeMode16;
SIO1writeMode32       SIO1_writeMode32;
SIO1writeCtrl16       SIO1_writeCtrl16;
SIO1writeCtrl32       SIO1_writeCtrl32;
SIO1writeBaud16       SIO1_writeBaud16;
SIO1writeBaud32       SIO1_writeBaud32;
SIO1readData8         SIO1_readData8;
SIO1readData16        SIO1_readData16;
SIO1readData32        SIO1_readData32;
SIO1readStat16        SIO1_readStat16;
SIO1readStat32        SIO1_readStat32;
SIO1readMode16        SIO1_readMode16;
SIO1readMode32        SIO1_readMode32;
SIO1readCtrl16        SIO1_readCtrl16;
SIO1readCtrl32        SIO1_readCtrl32;
SIO1readBaud16        SIO1_readBaud16;
SIO1readBaud32        SIO1_readBaud32;
SIO1registerCallback  SIO1_registerCallback;

#endif

static const char *err;

#define CheckErr(func) { \
	err = SysLibError(); \
	if (err != NULL) { SysMessage(_("Error loading %s: %s"), func, err); return -1; } \
}

#define LoadSym(dest, src, name, checkerr) { \
	dest = (src)SysLoadSym(drv, name); \
	if (checkerr) { CheckErr(name); } else SysLibError(); \
}

void *hGPUDriver = NULL;

void CALLBACK GPU__displayText(char *pText) {
	SysPrintf("%s\n", pText);
}

void CALLBACK GPUbusy( int ticks )
{
    //printf( "GPUbusy( %i )\n", ticks );
    //fflush( 0 );

    psxRegs.interrupt |= (1 << PSXINT_GPUBUSY);
    psxRegs.intCycle[PSXINT_GPUBUSY].cycle = ticks;
    psxRegs.intCycle[PSXINT_GPUBUSY].sCycle = psxRegs.cycle;
}

s32 CALLBACK GPU__configure(void) { return 0; }
s32 CALLBACK GPU__test(void) { return 0; }
void CALLBACK GPU__about(void) {}
void CALLBACK GPU__makeSnapshot(void) {}
void CALLBACK GPU__keypressed(int key) {}
s32 CALLBACK GPU__getScreenPic(unsigned char *pMem) { return -1; }
s32 CALLBACK GPU__showScreenPic(unsigned char *pMem) { return -1; }
void CALLBACK GPU__clearDynarec(void (CALLBACK *callback)(void)) {}
void CALLBACK GPU__vBlank(int val) {}
void CALLBACK GPU__registerCallback(void (CALLBACK *callback)(int)) {}
void CALLBACK GPU__idle(void) {}
void CALLBACK GPU__visualVibration(u32 iSmall, u32 iBig) {}
void CALLBACK GPU__cursor(int player, int x, int y) {}

#define LoadGpuSym1(dest, name) \
	LoadSym(GPU_##dest, GPU##dest, name, TRUE);

#define LoadGpuSym0(dest, name) \
	LoadSym(GPU_##dest, GPU##dest, name, FALSE); \
	if (GPU_##dest == NULL) GPU_##dest = (GPU##dest) GPU__##dest;

#define LoadGpuSymN(dest, name) \
	LoadSym(GPU_##dest, GPU##dest, name, FALSE);

static int LoadGPUplugin(const char *GPUdll) {
	void *drv;

	hGPUDriver = SysLoadLibrary(GPUdll);
	if (hGPUDriver == NULL) {
		GPU_configure = NULL;
		SysMessage (_("Could not load GPU plugin %s!"), GPUdll); return -1;
	}
	drv = hGPUDriver;
	LoadGpuSym1(init, "GPUinit");
	LoadGpuSym1(shutdown, "GPUshutdown");
	LoadGpuSym1(open, "GPUopen");
	LoadGpuSym1(close, "GPUclose");
	LoadGpuSym1(readData, "GPUreadData");
	LoadGpuSym1(readDataMem, "GPUreadDataMem");
	LoadGpuSym1(readStatus, "GPUreadStatus");
	LoadGpuSym1(writeData, "GPUwriteData");
	LoadGpuSym1(writeDataMem, "GPUwriteDataMem");
	LoadGpuSym1(writeStatus, "GPUwriteStatus");
	LoadGpuSym1(dmaChain, "GPUdmaChain");
	LoadGpuSym1(updateLace, "GPUupdateLace");
	LoadGpuSym0(keypressed, "GPUkeypressed");
	LoadGpuSym1(displayText, "GPUdisplayText");
	LoadGpuSym0(makeSnapshot, "GPUmakeSnapshot");
	LoadGpuSym1(freeze, "GPUfreeze");
	LoadGpuSym0(getScreenPic, "GPUgetScreenPic");
	LoadGpuSym0(showScreenPic, "GPUshowScreenPic");
	LoadGpuSym0(clearDynarec, "GPUclearDynarec");
    LoadGpuSym0(vBlank, "GPUvBlank");
    LoadGpuSym0(registerCallback, "GPUregisterCallback");
    LoadGpuSym0(idle, "GPUidle");
    LoadGpuSym0(visualVibration, "GPUvisualVibration");
    LoadGpuSym0(cursor, "GPUcursor");
	LoadGpuSym0(configure, "GPUconfigure");
	LoadGpuSym0(test, "GPUtest");
	LoadGpuSym0(about, "GPUabout");

	return 0;
}

void *hCDRDriver = NULL;

s32 CALLBACK CDR__play(unsigned char *sector) { return 0; }
s32 CALLBACK CDR__stop(void) { return 0; }

s32 CALLBACK CDR__getStatus(struct CdrStat *stat) {
	if (cdOpenCaseTime < 0 || cdOpenCaseTime > (s64)time(NULL))
		stat->Status = 0x10;
	else
		stat->Status = 0;

	return 0;
}

char* CALLBACK CDR__getDriveLetter(void) { return NULL; }
s32 CALLBACK CDR__configure(void) { return 0; }
s32 CALLBACK CDR__test(void) { return 0; }
void CALLBACK CDR__about(void) {}
s32 CALLBACK CDR__setfilename(char*filename) { return 0; }

#define LoadCdrSym1(dest, name) \
	LoadSym(CDR_##dest, CDR##dest, name, TRUE);

#define LoadCdrSym0(dest, name) \
	LoadSym(CDR_##dest, CDR##dest, name, FALSE); \
	if (CDR_##dest == NULL) CDR_##dest = (CDR##dest) CDR__##dest;

#define LoadCdrSymN(dest, name) \
	LoadSym(CDR_##dest, CDR##dest, name, FALSE);

static int LoadCDRplugin(const char *CDRdll) {
	void *drv;

	//if (CDRdll == NULL) {
	//	cdrIsoInit();
	//	return 0;
	//}

	hCDRDriver = SysLoadLibrary(CDRdll);
	if (hCDRDriver == NULL) {
		CDR_configure = NULL;
		SysMessage (_("Could not load CD-ROM plugin %s!"), CDRdll);  return -1;
	}
	drv = hCDRDriver;
	LoadCdrSym1(init, "CDRinit");
	LoadCdrSym1(shutdown, "CDRshutdown");
	LoadCdrSym1(open, "CDRopen");
	LoadCdrSym1(close, "CDRclose");
	LoadCdrSym1(getTN, "CDRgetTN");
	LoadCdrSym1(getTD, "CDRgetTD");
	LoadCdrSym1(readTrack, "CDRreadTrack");
	LoadCdrSym1(getBuffer, "CDRgetBuffer");
	LoadCdrSym1(getBufferSub, "CDRgetBufferSub");
	LoadCdrSym0(play, "CDRplay");
	LoadCdrSym0(stop, "CDRstop");
	LoadCdrSym0(getStatus, "CDRgetStatus");
	LoadCdrSym0(getDriveLetter, "CDRgetDriveLetter");
	LoadCdrSym0(configure, "CDRconfigure");
	LoadCdrSym0(test, "CDRtest");
	LoadCdrSym0(about, "CDRabout");
	LoadCdrSym0(setfilename, "CDRsetfilename");
	LoadCdrSymN(readCDDA, "CDRreadCDDA");
	LoadCdrSymN(getTE, "CDRgetTE");

	return 0;
}

void *hSPUDriver = NULL;

s32 CALLBACK SPU__configure(void) { return 0; }
void CALLBACK SPU__about(void) {}
s32 CALLBACK SPU__test(void) { return 0; }

#define LoadSpuSym1(dest, name) \
	LoadSym(SPU_##dest, SPU##dest, name, TRUE);

#define LoadSpuSym0(dest, name) \
	LoadSym(SPU_##dest, SPU##dest, name, FALSE); \
	if (SPU_##dest == NULL) SPU_##dest = (SPU##dest) SPU__##dest;

#define LoadSpuSymN(dest, name) \
	LoadSym(SPU_##dest, SPU##dest, name, FALSE);

static int LoadSPUplugin(const char *SPUdll) {
	void *drv;

	hSPUDriver = SysLoadLibrary(SPUdll);
	if (hSPUDriver == NULL) {
		SPU_configure = NULL;
		SysMessage (_("Could not load SPU plugin %s!"), SPUdll); return -1;
	}
	drv = hSPUDriver;
	LoadSpuSym1(init, "SPUinit");
	LoadSpuSym1(shutdown, "SPUshutdown");
	LoadSpuSym1(open, "SPUopen");
	LoadSpuSym1(close, "SPUclose");
	LoadSpuSym0(configure, "SPUconfigure");
	LoadSpuSym0(about, "SPUabout");
	LoadSpuSym0(test, "SPUtest");
	LoadSpuSym1(writeRegister, "SPUwriteRegister");
	LoadSpuSym1(readRegister, "SPUreadRegister");
	LoadSpuSym1(writeDMA, "SPUwriteDMA");
	LoadSpuSym1(readDMA, "SPUreadDMA");
	LoadSpuSym1(writeDMAMem, "SPUwriteDMAMem");
	LoadSpuSym1(readDMAMem, "SPUreadDMAMem");
	LoadSpuSym1(playADPCMchannel, "SPUplayADPCMchannel");
	LoadSpuSym1(freeze, "SPUfreeze");
	LoadSpuSym1(registerCallback, "SPUregisterCallback");
	LoadSpuSymN(async, "SPUasync");
	LoadSpuSymN(playCDDAchannel, "SPUplayCDDAchannel");

	return 0;
}

void *hPAD1Driver = NULL;
void *hPAD2Driver = NULL;

/*
static unsigned char buf[256];
unsigned char stdpar[10] = { 0x00, 0x41, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
unsigned char mousepar[8] = { 0x00, 0x12, 0x5a, 0xff, 0xff, 0xff, 0xff };
unsigned char analogpar[9] = { 0x00, 0xff, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static int bufcount, bufc;

PadDataS padd1, padd2;

unsigned char _PADstartPoll(PadDataS *pad) {
    bufc = 0;

    switch (pad->controllerType) {
        case PSE_PAD_TYPE_MOUSE:
            mousepar[3] = pad->buttonStatus & 0xff;
            mousepar[4] = pad->buttonStatus >> 8;
            mousepar[5] = pad->moveX;
            mousepar[6] = pad->moveY;

            memcpy(buf, mousepar, 7);
            bufcount = 6;
            break;
        case PSE_PAD_TYPE_NEGCON: // npc101/npc104(slph00001/slph00069)
            analogpar[1] = 0x23;
            analogpar[3] = pad->buttonStatus & 0xff;
            analogpar[4] = pad->buttonStatus >> 8;
            analogpar[5] = pad->rightJoyX;
            analogpar[6] = pad->rightJoyY;
            analogpar[7] = pad->leftJoyX;
            analogpar[8] = pad->leftJoyY;

            memcpy(buf, analogpar, 9);
            bufcount = 8;
            break;
        case PSE_PAD_TYPE_ANALOGPAD: // scph1150
            analogpar[1] = 0x73;
            analogpar[3] = pad->buttonStatus & 0xff;
            analogpar[4] = pad->buttonStatus >> 8;
            analogpar[5] = pad->rightJoyX;
            analogpar[6] = pad->rightJoyY;
            analogpar[7] = pad->leftJoyX;
            analogpar[8] = pad->leftJoyY;

            memcpy(buf, analogpar, 9);
            bufcount = 8;
            break;
        case PSE_PAD_TYPE_ANALOGJOY: // scph1110
            analogpar[1] = 0x53;
            analogpar[3] = pad->buttonStatus & 0xff;
            analogpar[4] = pad->buttonStatus >> 8;
            analogpar[5] = pad->rightJoyX;
            analogpar[6] = pad->rightJoyY;
            analogpar[7] = pad->leftJoyX;
            analogpar[8] = pad->leftJoyY;

            memcpy(buf, analogpar, 9);
            bufcount = 8;
            break;
        case PSE_PAD_TYPE_STANDARD:
        default:
            stdpar[3] = pad->buttonStatus & 0xff;
            stdpar[4] = pad->buttonStatus >> 8;

            memcpy(buf, stdpar, 5);
            bufcount = 4;
    }

    return buf[bufc++];
}

unsigned char _PADpoll(unsigned char value) {
    if (bufc > bufcount) return 0;
    return buf[bufc++];
}

unsigned char CALLBACK PAD1__startPoll(int pad) {
    PadDataS padd;

    PAD1_readPort1(&padd);

    return _PADstartPoll(&padd);
}

unsigned char CALLBACK PAD1__poll(unsigned char value) {
    return _PADpoll(value);
}*/

s32 CALLBACK PAD1__configure(void) { return 0; }
void CALLBACK PAD1__about(void) {}
s32 CALLBACK PAD1__test(void) { return 0; }
s32 CALLBACK PAD1__query(void) { return 3; }
s32 CALLBACK PAD1__keypressed() { return 0; }
void CALLBACK PAD1__registerVibration(void (CALLBACK *callback)(u32, u32)) {}
void CALLBACK PAD1__registerCursor(void (CALLBACK *callback)(int, int, int)) {}

#define LoadPad1Sym1(dest, name) \
	LoadSym(PAD1_##dest, PAD##dest, name, TRUE);

#define LoadPad1SymN(dest, name) \
	LoadSym(PAD1_##dest, PAD##dest, name, FALSE);

#define LoadPad1Sym0(dest, name) \
	LoadSym(PAD1_##dest, PAD##dest, name, FALSE); \
	if (PAD1_##dest == NULL) PAD1_##dest = (PAD##dest) PAD1__##dest;

static int LoadPAD1plugin(const char *PAD1dll) {
	void *drv;

	hPAD1Driver = SysLoadLibrary(PAD1dll);
	if (hPAD1Driver == NULL) {
		PAD1_configure = NULL;
		SysMessage (_("Could not load Controller 1 plugin %s!"), PAD1dll); return -1;
	}
	drv = hPAD1Driver;
	LoadPad1Sym1(init, "PADinit");
	LoadPad1Sym1(shutdown, "PADshutdown");
	LoadPad1Sym1(open, "PADopen");
	LoadPad1Sym1(close, "PADclose");
	LoadPad1Sym0(query, "PADquery");
	LoadPad1Sym1(readPort1, "PADreadPort1");
	LoadPad1Sym0(configure, "PADconfigure");
	LoadPad1Sym0(test, "PADtest");
	LoadPad1Sym0(about, "PADabout");
	LoadPad1Sym0(keypressed, "PADkeypressed");
	LoadPad1Sym1(startPoll, "PADstartPoll");
	LoadPad1Sym1(poll, "PADpoll");
	LoadPad1SymN(setSensitive, "PADsetSensitive");
    LoadPad1Sym0(registerVibration, "PADregisterVibration");
    LoadPad1Sym0(registerCursor, "PADregisterCursor");

	return 0;
}
/*
unsigned char CALLBACK PAD2__startPoll(int pad) {
	PadDataS padd;

	PAD2_readPort2(&padd);

	return _PADstartPoll(&padd);
}

unsigned char CALLBACK PAD2__poll(unsigned char value) {
	return _PADpoll(value);
}
*/
s32 CALLBACK PAD2__configure(void) { return 0; }
void CALLBACK PAD2__about(void) {}
s32 CALLBACK PAD2__test(void) { return 0; }
s32 CALLBACK PAD2__query(void) { return PSE_PAD_USE_PORT1 | PSE_PAD_USE_PORT2; }
s32 CALLBACK PAD2__keypressed() { return 0; }
void CALLBACK PAD2__registerVibration(void (CALLBACK *callback)(u32, u32)) {}
void CALLBACK PAD2__registerCursor(void (CALLBACK *callback)(int, int, int)) {}

#define LoadPad2Sym1(dest, name) \
	LoadSym(PAD2_##dest, PAD##dest, name, TRUE);

#define LoadPad2Sym0(dest, name) \
	LoadSym(PAD2_##dest, PAD##dest, name, FALSE); \
	if (PAD2_##dest == NULL) PAD2_##dest = (PAD##dest) PAD2__##dest;

#define LoadPad2SymN(dest, name) \
	LoadSym(PAD2_##dest, PAD##dest, name, FALSE);

static int LoadPAD2plugin(const char *PAD2dll) {
	void *drv;

	hPAD2Driver = SysLoadLibrary(PAD2dll);
	if (hPAD2Driver == NULL) {
		PAD2_configure = NULL;
		SysMessage (_("Could not load Controller 2 plugin %s!"), PAD2dll); return -1;
	}
	drv = hPAD2Driver;
	LoadPad2Sym1(init, "PADinit");
	LoadPad2Sym1(shutdown, "PADshutdown");
	LoadPad2Sym1(open, "PADopen");
	LoadPad2Sym1(close, "PADclose");
	LoadPad2Sym0(query, "PADquery");
	LoadPad2Sym1(readPort2, "PADreadPort2");
	LoadPad2Sym0(configure, "PADconfigure");
	LoadPad2Sym0(test, "PADtest");
	LoadPad2Sym0(about, "PADabout");
	LoadPad2Sym0(keypressed, "PADkeypressed");
	LoadPad2Sym1(startPoll, "PADstartPoll");
	LoadPad2Sym1(poll, "PADpoll");
	LoadPad2SymN(setSensitive, "PADsetSensitive");
    LoadPad2Sym0(registerVibration, "PADregisterVibration");
    LoadPad2Sym0(registerCursor, "PADregisterCursor");

	return 0;
}

void *hNETDriver = NULL;

void CALLBACK NET__setInfo(netInfo *info) {}
void CALLBACK NET__keypressed(int key) {}
s32 CALLBACK NET__configure(void) { return 0; }
s32 CALLBACK NET__test(void) { return 0; }
void CALLBACK NET__about(void) {}

#define LoadNetSym1(dest, name) \
	LoadSym(NET_##dest, NET##dest, name, TRUE);

#define LoadNetSymN(dest, name) \
	LoadSym(NET_##dest, NET##dest, name, FALSE);

#define LoadNetSym0(dest, name) \
	LoadSym(NET_##dest, NET##dest, name, FALSE); \
	if (NET_##dest == NULL) NET_##dest = (NET##dest) NET__##dest;

static int LoadNETplugin(const char *NETdll) {
	void *drv;

	hNETDriver = SysLoadLibrary(NETdll);
	if (hNETDriver == NULL) {
		SysMessage (_("Could not load NetPlay plugin %s!"), NETdll); return -1;
	}
	drv = hNETDriver;
	LoadNetSym1(init, "NETinit");
	LoadNetSym1(shutdown, "NETshutdown");
	LoadNetSym1(open, "NETopen");
	LoadNetSym1(close, "NETclose");
	LoadNetSymN(sendData, "NETsendData");
	LoadNetSymN(recvData, "NETrecvData");
	LoadNetSym1(sendPadData, "NETsendPadData");
	LoadNetSym1(recvPadData, "NETrecvPadData");
	LoadNetSym1(queryPlayer, "NETqueryPlayer");
	LoadNetSym1(pause, "NETpause");
	LoadNetSym1(resume, "NETresume");
	LoadNetSym0(setInfo, "NETsetInfo");
	LoadNetSym0(keypressed, "NETkeypressed");
	LoadNetSym0(configure, "NETconfigure");
	LoadNetSym0(test, "NETtest");
	LoadNetSym0(about, "NETabout");

	return 0;
}

#ifdef ENABLE_SIO1API

void *hSIO1Driver = NULL;

s32 CALLBACK SIO1__init(void) { return 0; }
s32 CALLBACK SIO1__shutdown(void) { return 0; }
s32 CALLBACK SIO1__open(void) { return 0; }
s32 CALLBACK SIO1__close(void) { return 0; }
s32 CALLBACK SIO1__configure(void) { return 0; }
s32 CALLBACK SIO1__test(void) { return 0; }
void CALLBACK SIO1__about(void) {}
void CALLBACK SIO1__pause(void) {}
void CALLBACK SIO1__resume(void) {}
s32 CALLBACK SIO1__keypressed(int key) { return 0; }
void CALLBACK SIO1__writeData8(unsigned char val) {}
void CALLBACK SIO1__writeData16(unsigned short val) {}
void CALLBACK SIO1__writeData32(u32 val) {}
void CALLBACK SIO1__writeStat16(unsigned short val) {}
void CALLBACK SIO1__writeStat32(u32 val) {}
void CALLBACK SIO1__writeMode16(unsigned short val) {}
void CALLBACK SIO1__writeMode32(u32 val) {}
void CALLBACK SIO1__writeCtrl16(unsigned short val) {}
void CALLBACK SIO1__writeCtrl32(u32 val) {}
void CALLBACK SIO1__writeBaud16(unsigned short val) {}
void CALLBACK SIO1__writeBaud32(u32 val) {}
unsigned char CALLBACK SIO1__readData8(void) { return 0; }
unsigned short CALLBACK SIO1__readData16(void) { return 0; }
u32 CALLBACK SIO1__readData32(void) { return 0; }
unsigned short CALLBACK SIO1__readStat16(void) { return 0; }
u32 CALLBACK SIO1__readStat32(void) { return 0; }
unsigned short CALLBACK SIO1__readMode16(void) { return 0; }
u32 CALLBACK SIO1__readMode32(void) { return 0; }
unsigned short CALLBACK SIO1__readCtrl16(void) { return 0; }
u32 CALLBACK SIO1__readCtrl32(void) { return 0; }
unsigned short CALLBACK SIO1__readBaud16(void) { return 0; }
u32 CALLBACK SIO1__readBaud32(void) { return 0; }
void CALLBACK SIO1__registerCallback(void (CALLBACK *callback)(void)) {};

void CALLBACK SIO1irq(void) {
    psxHu32ref(0x1070) |= SWAPu32(0x100);
}

#define LoadSio1Sym1(dest, name) \
    LoadSym(SIO1_##dest, SIO1##dest, name, TRUE);

#define LoadSio1SymN(dest, name) \
    LoadSym(SIO1_##dest, SIO1##dest, name, FALSE);

#define LoadSio1Sym0(dest, name) \
    LoadSym(SIO1_##dest, SIO1##dest, name, FALSE); \
    if (SIO1_##dest == NULL) SIO1_##dest = (SIO1##dest) SIO1__##dest;

static int LoadSIO1plugin(const char *SIO1dll) {
    void *drv;

    hSIO1Driver = SysLoadLibrary(SIO1dll);
    if (hSIO1Driver == NULL) {
        SysMessage (_("Could not load SIO1 plugin %s!"), SIO1dll); return -1;
    }
    drv = hSIO1Driver;

    LoadSio1Sym0(init, "SIO1init");
    LoadSio1Sym0(shutdown, "SIO1shutdown");
    LoadSio1Sym0(open, "SIO1open");
    LoadSio1Sym0(close, "SIO1close");
    LoadSio1Sym0(pause, "SIO1pause");
    LoadSio1Sym0(resume, "SIO1resume");
    LoadSio1Sym0(keypressed, "SIO1keypressed");
    LoadSio1Sym0(configure, "SIO1configure");
    LoadSio1Sym0(test, "SIO1test");
    LoadSio1Sym0(about, "SIO1about");
    LoadSio1Sym0(writeData8, "SIO1writeData8");
    LoadSio1Sym0(writeData16, "SIO1writeData16");
    LoadSio1Sym0(writeData32, "SIO1writeData32");
    LoadSio1Sym0(writeStat16, "SIO1writeStat16");
    LoadSio1Sym0(writeStat32, "SIO1writeStat32");
    LoadSio1Sym0(writeMode16, "SIO1writeMode16");
    LoadSio1Sym0(writeMode32, "SIO1writeMode32");
    LoadSio1Sym0(writeCtrl16, "SIO1writeCtrl16");
    LoadSio1Sym0(writeCtrl32, "SIO1writeCtrl32");
    LoadSio1Sym0(writeBaud16, "SIO1writeBaud16");
    LoadSio1Sym0(writeBaud32, "SIO1writeBaud32");
    LoadSio1Sym0(readData16, "SIO1readData16");
    LoadSio1Sym0(readData32, "SIO1readData32");
    LoadSio1Sym0(readStat16, "SIO1readStat16");
    LoadSio1Sym0(readStat32, "SIO1readStat32");
    LoadSio1Sym0(readMode16, "SIO1readMode16");
    LoadSio1Sym0(readMode32, "SIO1readMode32");
    LoadSio1Sym0(readCtrl16, "SIO1readCtrl16");
    LoadSio1Sym0(readCtrl32, "SIO1readCtrl32");
    LoadSio1Sym0(readBaud16, "SIO1readBaud16");
    LoadSio1Sym0(readBaud32, "SIO1readBaud32");
    LoadSio1Sym0(registerCallback, "SIO1registerCallback");

    return 0;
}

#endif

void CALLBACK clearDynarec(void) {
	psxCpu->Reset();
}

int LoadPlugins() {
	int ret;
	char Plugin[MAXPATHLEN];

	ReleasePlugins();

	if (LoadCDRplugin("CDR") == -1) return -1;
	if (LoadGPUplugin("GPU") == -1) return -1;
	if (LoadSPUplugin("SPU") == -1) return -1;
	if (LoadPAD1plugin("PAD1") == -1) return -1;
	if (LoadPAD2plugin("PAD2") == -1) return -1;

	if (!strcmp("Disabled", Config.Net)) Config.UseNet = 0;
	else {
		Config.UseNet = TRUE;
		sprintf(Plugin, "%s/%s", Config.PluginsDir, Config.Net);
		if (LoadNETplugin(Plugin) == -1) Config.UseNet = FALSE;
	}

#ifdef ENABLE_SIO1API
	sprintf(Plugin, "%s/%s", Config.PluginsDir, Config.Sio1);
	if (LoadSIO1plugin(Plugin) == -1) return -1;
#endif

	ret = CDR_init();
	if (ret < 0) { SysMessage (_("Error initializing CD-ROM plugin: %d"), ret); return -1; }
	ret = GPU_init();
	if (ret < 0) { SysMessage (_("Error initializing GPU plugin: %d"), ret); return -1; }
	ret = SPU_init();
	if (ret < 0) { SysMessage (_("Error initializing SPU plugin: %d"), ret); return -1; }
	ret = PAD1_init(1);
	if (ret < 0) { SysMessage (_("Error initializing Controller 1 plugin: %d"), ret); return -1; }
	ret = PAD2_init(2);
	if (ret < 0) { SysMessage (_("Error initializing Controller 2 plugin: %d"), ret); return -1; }

	if (Config.UseNet) {
		ret = NET_init();
		if (ret < 0) { SysMessage (_("Error initializing NetPlay plugin: %d"), ret); return -1; }
	}

#ifdef ENABLE_SIO1API
	ret = SIO1_init();
	if (ret < 0) { SysMessage (_("Error initializing SIO1 plugin: %d"), ret); return -1; }
#endif

	SysPrintf(_("Plugins loaded.\n"));
	return 0;
}

void ReleasePlugins() {
	if (Config.UseNet) {
		int ret = NET_close();
		if (ret < 0) Config.UseNet = FALSE;
	}
	NetOpened = FALSE;

	if (hCDRDriver != NULL || cdrIsoActive()) CDR_shutdown();
	if (hGPUDriver != NULL) GPU_shutdown();
	if (hSPUDriver != NULL) SPU_shutdown();
	if (hPAD1Driver != NULL) PAD1_shutdown();
	if (hPAD2Driver != NULL) PAD2_shutdown();

	if (Config.UseNet && hNETDriver != NULL) NET_shutdown();

	if (hCDRDriver != NULL) { SysCloseLibrary(hCDRDriver); hCDRDriver = NULL; }
	if (hGPUDriver != NULL) { SysCloseLibrary(hGPUDriver); hGPUDriver = NULL; }
	if (hSPUDriver != NULL) { SysCloseLibrary(hSPUDriver); hSPUDriver = NULL; }
	if (hPAD1Driver != NULL) { SysCloseLibrary(hPAD1Driver); hPAD1Driver = NULL; }
	if (hPAD2Driver != NULL) { SysCloseLibrary(hPAD2Driver); hPAD2Driver = NULL; }

	if (Config.UseNet && hNETDriver != NULL) {
		SysCloseLibrary(hNETDriver); hNETDriver = NULL;
	}

#ifdef ENABLE_SIO1API
	if (hSIO1Driver != NULL) {
		SIO1_shutdown();
		SysCloseLibrary(hSIO1Driver);
		hSIO1Driver = NULL;
	}
#endif
}

void SetIsoFile(const char *filename) {
	if (filename == NULL) {
		IsoFile[0] = '\0';
		return;
	}
	strncpy(IsoFile, filename, MAXPATHLEN);
}

const char *GetIsoFile(void) {
	return IsoFile;
}

boolean UsingIso(void) {
	return (IsoFile[0] != '\0');
}

void SetCdOpenCaseTime(s64 time) {
	cdOpenCaseTime = time;
}
