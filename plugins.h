/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
 *   schultz.ryan@gmail.com, http://rschultz.ath.cx/code.php               *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef __PLUGINS_H__
#define __PLUGINS_H__

#include "PsxCommon.h"
#include "Spu.h"

typedef void* HWND;
#define CALLBACK
typedef long (* GPUopen)(unsigned long *, char *, char *);
long GPU__open(void);          
typedef long (* SPUopen)(void);
long SPU__open(void);			
typedef long (* PADopen)(unsigned long *);
long PAD1__open(void);			
long PAD2__open(void);
typedef long (* NETopen)(unsigned long *);

#include "PSEmu_Plugin_Defs.h"
#include "Decode_XA.h"

int  LoadPlugins();
void ReleasePlugins();
int  OpenPlugins();
void ClosePlugins();


typedef unsigned long (CALLBACK* PSEgetLibType)(void);
typedef unsigned long (CALLBACK* PSEgetLibVersion)(void);
typedef char *(CALLBACK* PSEgetLibName)(void);

///GPU PLUGIN STUFF 
typedef long (CALLBACK* GPUinit)(void);
typedef long (CALLBACK* GPUshutdown)(void);
typedef long (CALLBACK* GPUclose)(void);
typedef void (CALLBACK* GPUwriteStatus)(uint32_t);
typedef void (CALLBACK* GPUwriteData)(uint32_t);
typedef void (CALLBACK* GPUwriteDataMem)(unsigned long *, int);
typedef uint32_t (CALLBACK* GPUreadStatus)(void);
typedef uint32_t (CALLBACK* GPUreadData)(void);
typedef void (CALLBACK* GPUreadDataMem)(unsigned long *, int);
typedef long (CALLBACK* GPUdmaChain)(uint32_t *,uint32_t);
typedef void (CALLBACK* GPUupdateLace)(void);
typedef long (CALLBACK* GPUconfigure)(void);
typedef long (CALLBACK* GPUtest)(void);
typedef void (CALLBACK* GPUabout)(void);
typedef void (CALLBACK* GPUmakeSnapshot)(void);
typedef void (CALLBACK* GPUkeypressed)(int);
typedef void (CALLBACK* GPUdisplayText)(char *);
typedef struct GPUFREEZETAG
{
 unsigned long ulFreezeVersion;      // should be always 1 for now (set by main emu)
 unsigned long ulStatus;             // current gpu status
 unsigned long ulControl[256];       // latest control register values
 //unsigned char psxVRam[1024*1024*2]; // current VRam image (full 2 MB for ZN)
} GPUFreeze_t;
typedef long (CALLBACK* GPUfreeze)(uint32_t, GPUFreeze_t *);
typedef long (CALLBACK* GPUgetScreenPic)(unsigned char *);
typedef long (CALLBACK* GPUshowScreenPic)(unsigned char *);
typedef void (CALLBACK* GPUclearDynarec)(void (CALLBACK *callback)(void));

//plugin stuff From Shadow
// *** walking in the valley of your darking soul i realize that i was alone
//Gpu function pointers
GPUupdateLace    GPU_updateLace;
GPUinit          GPU_init;
GPUshutdown      GPU_shutdown; 
GPUconfigure     GPU_configure;
GPUtest          GPU_test;
GPUabout         GPU_about;
GPUopen          GPU_open;
GPUclose         GPU_close;
GPUreadStatus    GPU_readStatus;
GPUreadData      GPU_readData;
GPUreadDataMem   GPU_readDataMem;
GPUwriteStatus   GPU_writeStatus; 
GPUwriteData     GPU_writeData;
GPUwriteDataMem  GPU_writeDataMem;
GPUdmaChain      GPU_dmaChain;
GPUkeypressed    GPU_keypressed;
GPUdisplayText   GPU_displayText;
GPUmakeSnapshot  GPU_makeSnapshot;
GPUfreeze        GPU_freeze;
GPUgetScreenPic  GPU_getScreenPic;
GPUshowScreenPic GPU_showScreenPic;
GPUclearDynarec  GPU_clearDynarec;

//cd rom plugin ;)
typedef long (CALLBACK* CDRinit)(void);
typedef long (CALLBACK* CDRshutdown)(void);
typedef long (CALLBACK* CDRopen)(void);
typedef long (CALLBACK* CDRclose)(void);
typedef long (CALLBACK* CDRgetTN)(unsigned char *);
typedef long (CALLBACK* CDRgetTD)(unsigned char , unsigned char *);
typedef long (CALLBACK* CDRreadTrack)(unsigned char *);
typedef unsigned char * (CALLBACK* CDRgetBuffer)(void);
typedef long (CALLBACK* CDRconfigure)(void);
typedef long (CALLBACK* CDRtest)(void);
typedef void (CALLBACK* CDRabout)(void);
typedef long (CALLBACK* CDRplay)(unsigned char *);
typedef long (CALLBACK* CDRstop)(void);
typedef long (CALLBACK* CDRsetfilename)(char *);
struct CdrStat {
	uint32_t Type;
	uint32_t Status;
	unsigned char Time[3];
};
typedef long (CALLBACK* CDRgetStatus)(struct CdrStat *);
typedef char* (CALLBACK* CDRgetDriveLetter)(void);
struct SubQ {
	char res0[11];
	unsigned char ControlAndADR;
	unsigned char TrackNumber;
	unsigned char IndexNumber;
	unsigned char TrackRelativeAddress[3];
	unsigned char Filler;
	unsigned char AbsoluteAddress[3];
	char res1[72];
};
typedef unsigned char* (CALLBACK* CDRgetBufferSub)(void);

//cd rom function pointers 
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

// spu plugin
typedef long (CALLBACK* SPUinit)(void);				
typedef long (CALLBACK* SPUshutdown)(void);	
typedef long (CALLBACK* SPUclose)(void);			
typedef void (CALLBACK* SPUplaySample)(unsigned char);		
typedef void (CALLBACK* SPUstartChannels1)(unsigned short);	
typedef void (CALLBACK* SPUstartChannels2)(unsigned short);
typedef void (CALLBACK* SPUstopChannels1)(unsigned short);	
typedef void (CALLBACK* SPUstopChannels2)(unsigned short);	
typedef void (CALLBACK* SPUputOne)(uint32_t,unsigned short);			
typedef unsigned short (CALLBACK* SPUgetOne)(uint32_t);			
typedef void (CALLBACK* SPUsetAddr)(unsigned char, unsigned short);			
typedef void (CALLBACK* SPUsetPitch)(unsigned char, unsigned short);		
typedef void (CALLBACK* SPUsetVolumeL)(unsigned char, short );		
typedef void (CALLBACK* SPUsetVolumeR)(unsigned char, short );		
//psemu pro 2 functions from now..
typedef void (CALLBACK* SPUwriteRegister)(unsigned long, unsigned short);	
typedef unsigned short (CALLBACK* SPUreadRegister)(unsigned long);		
typedef void (CALLBACK* SPUwriteDMA)(unsigned short);
typedef unsigned short (CALLBACK* SPUreadDMA)(void);
typedef void (CALLBACK* SPUwriteDMAMem)(unsigned short *, int);
typedef void (CALLBACK* SPUreadDMAMem)(unsigned short *, int);
typedef void (CALLBACK* SPUplayADPCMchannel)(xa_decode_t *);
typedef void (CALLBACK* SPUregisterCallback)(void (CALLBACK *callback)(void));
typedef long (CALLBACK* SPUconfigure)(void);
typedef long (CALLBACK* SPUtest)(void);			
typedef void (CALLBACK* SPUabout)(void);
typedef struct
{
 char          szSPUName[8];
 unsigned long ulFreezeVersion;
 unsigned long ulFreezeSize;
 unsigned char cSPUPort[0x200];
 //unsigned char cSPURam[0x80000];
 xa_decode_t   xaS;     
} SPUFreeze_t;
typedef long (CALLBACK* SPUfreeze)(uint32_t, SPUFreeze_t *);
typedef void (CALLBACK* SPUasync)(uint32_t);

//SPU POINTERS
SPUconfigure        SPU_configure;
SPUabout            SPU_about;
SPUinit             SPU_init;
SPUshutdown         SPU_shutdown;
SPUtest             SPU_test;
SPUopen             SPU_open;
SPUclose            SPU_close;
SPUplaySample       SPU_playSample;
SPUstartChannels1   SPU_startChannels1;
SPUstartChannels2   SPU_startChannels2;
SPUstopChannels1    SPU_stopChannels1;
SPUstopChannels2    SPU_stopChannels2;
SPUputOne           SPU_putOne;
SPUgetOne           SPU_getOne;
SPUsetAddr          SPU_setAddr;
SPUsetPitch         SPU_setPitch;
SPUsetVolumeL       SPU_setVolumeL;
SPUsetVolumeR       SPU_setVolumeR;
SPUwriteRegister    SPU_writeRegister;
SPUreadRegister     SPU_readRegister;
SPUwriteDMA         SPU_writeDMA;
SPUreadDMA          SPU_readDMA;
SPUwriteDMAMem      SPU_writeDMAMem;
SPUreadDMAMem       SPU_readDMAMem;
SPUplayADPCMchannel SPU_playADPCMchannel;
SPUfreeze           SPU_freeze;
SPUregisterCallback SPU_registerCallback;
SPUasync            SPU_async;

// PAD Functions

typedef long (CALLBACK* PADconfigure)(void);
typedef void (CALLBACK* PADabout)(void);
typedef long (CALLBACK* PADinit)(long);
typedef long (CALLBACK* PADshutdown)(void);	
typedef long (CALLBACK* PADtest)(void);		
typedef long (CALLBACK* PADclose)(void);
typedef long (CALLBACK* PADquery)(void);
typedef long (CALLBACK*	PADreadPort1)(PadDataS*);
typedef long (CALLBACK* PADreadPort2)(PadDataS*);
typedef long (CALLBACK* PADkeypressed)(void);
typedef unsigned char (CALLBACK* PADstartPoll)(int);
typedef unsigned char (CALLBACK* PADpoll)(unsigned char);
typedef void (CALLBACK* PADsetSensitive)(int);

//PAD POINTERS
PADconfigure        PAD1_configure;
PADabout            PAD1_about;
PADinit             PAD1_init;
PADshutdown         PAD1_shutdown;
PADtest             PAD1_test;
PADopen             PAD1_open;
PADclose            PAD1_close;
PADquery			PAD1_query;
PADreadPort1		PAD1_readPort1;
PADkeypressed		PAD1_keypressed;
PADstartPoll        PAD1_startPoll;
PADpoll             PAD1_poll;
PADsetSensitive     PAD1_setSensitive;

PADconfigure        PAD2_configure;
PADabout            PAD2_about;
PADinit             PAD2_init;
PADshutdown         PAD2_shutdown;
PADtest             PAD2_test;
PADopen             PAD2_open;
PADclose            PAD2_close;
PADquery            PAD2_query;
PADreadPort2		PAD2_readPort2;
PADkeypressed		PAD2_keypressed;
PADstartPoll        PAD2_startPoll;
PADpoll             PAD2_poll;
PADsetSensitive     PAD2_setSensitive;

// NET plugin

typedef long (CALLBACK* NETinit)(void);
typedef long (CALLBACK* NETshutdown)(void);
typedef long (CALLBACK* NETclose)(void);
typedef long (CALLBACK* NETconfigure)(void);
typedef long (CALLBACK* NETtest)(void);
typedef void (CALLBACK* NETabout)(void);
typedef void (CALLBACK* NETpause)(void);
typedef void (CALLBACK* NETresume)(void);
typedef long (CALLBACK* NETqueryPlayer)(void);
typedef long (CALLBACK* NETsendData)(void *, int, int);
typedef long (CALLBACK* NETrecvData)(void *, int, int);
typedef long (CALLBACK* NETsendPadData)(void *, int);
typedef long (CALLBACK* NETrecvPadData)(void *, int);

typedef struct {
	char EmuName[32];
	char CdromID[9];	// ie. 'SCPH12345', no \0 trailing character
	char CdromLabel[11];
	void *psxMem;
	GPUshowScreenPic GPU_showScreenPic;
	GPUdisplayText GPU_displayText;
	PADsetSensitive PAD_setSensitive;
	char GPUpath[256];	// paths must be absolute
	char SPUpath[256];
	char CDRpath[256];
	char MCD1path[256];
	char MCD2path[256];
	char BIOSpath[256];	// 'HLE' for internal bios
	char Unused[1024];
} netInfo;

typedef long (CALLBACK* NETsetInfo)(netInfo *);
typedef long (CALLBACK* NETkeypressed)(int)
;


// NET function pointers 
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

int LoadCDRplugin(char *CDRdll);
int LoadGPUplugin(char *GPUdll);
int LoadSPUplugin(char *SPUdll);
int LoadPAD1plugin(char *PAD1dll);
int LoadPAD2plugin(char *PAD2dll);
int LoadNETplugin(char *NETdll);

void CALLBACK clearDynarec(void);

#endif /* __PLUGINS_H__ */
