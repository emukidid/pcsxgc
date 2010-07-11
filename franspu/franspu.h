#ifndef __SPUPSX4ALL_H__
#define __SPUPSX4ALL_H__

#include "../PSXCommon.h"
#include "../Decode_XA.h"

#ifndef __WIN32__
#include <unistd.h>
#include <sys/time.h>  
#endif

#include <stdio.h>
#include <stdlib.h>

#define RRand(range) (random()%range)  
#include <string.h> 
#include <math.h>  

#define DWORD unsigned long
#define LOWORD(l)           ((unsigned short)(l)) 
#define HIWORD(l)           ((unsigned short)(((unsigned long)(l) >> 16) & 0xFFFF)) 

#define PSE_LT_SPU                  4
#define PSE_SPU_ERR_SUCCESS         0
#define PSE_SPU_ERR                 -60
#define PSE_SPU_ERR_NOTCONFIGURED   PSE_SPU_ERR - 1
#define PSE_SPU_ERR_INIT            PSE_SPU_ERR - 2
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

////////////////////////////////////////////////////////////////////////
// spu defines
////////////////////////////////////////////////////////////////////////

// sound buffer sizes
// 400 ms complete sound buffer
#define SOUNDSIZE   70560
// 137 ms test buffer... if less than that is buffered, a new upload will happen
#define TESTSIZE    24192
// num of channels
#define MAXCHAN     24
// ~ 1 ms of data
#ifdef ALTERNATESPU
#define NSSIZE (1024*64) //alot.
#else
#define NSSIZE 45
#endif

extern int SSumR[NSSIZE];
extern int SSumL[NSSIZE];
extern int iFMod[NSSIZE];

extern int bSPUIsOpen;


// byteswappings

#define SWAPSPU16(x) (((x)>>8 & 0xff) | ((x)<<8 & 0xff00))
#define SWAPSPU32(x) (((x)>>24 & 0xfful) | ((x)>>8 & 0xff00ul) | ((x)<<8 & 0xff0000ul) | ((x)<<24 & 0xff000000ul))

#define HOST2LE16(x) SWAPSPU16(x)
#define HOST2BE16(x) (x)
#define LE2HOST16(x) SWAPSPU16(x)
#define BE2HOST16(x) (x)

///////////////////////////////////////////////////////////
// struct defines
///////////////////////////////////////////////////////////

// ADSR INFOS PER CHANNEL
typedef struct
{
	int            AttackModeExp;
	long           AttackTime;
	long           DecayTime;
	long           SustainLevel;
	int            SustainModeExp;
	long           SustainModeDec;
	long           SustainTime;
	int            ReleaseModeExp;
	unsigned long  ReleaseVal;
	long           ReleaseTime;
	long           ReleaseStartTime; 
	long           ReleaseVol; 
	long           lTime;
	long           lVolume;
} ADSRInfo;

typedef struct
{
	int            State;
	int            AttackModeExp;
	int            AttackRate;
	int            DecayRate;
	int            SustainLevel;
	int            SustainModeExp;
	int            SustainIncrease;
	int            SustainRate;
	int            ReleaseModeExp;
	int            ReleaseRate;
	int            EnvelopeVol;
	long           lVolume;
	long           lDummy1;
	long           lDummy2;
} ADSRInfoEx;

///////////////////////////////////////////////////////////

// MAIN CHANNEL STRUCT
typedef struct
{
	int               bNew;                               // start flag
	
	int               iSBPos;                             // mixing stuff
	int               spos;
	int               sinc;
	int               SB[32];                             // Pete added another 32 dwords in 1.6 ... prevents overflow issues with gaussian/cubic interpolation (thanx xodnizel!), and can be used for even better interpolations, eh? :)
	int               sval;
	
	unsigned char *   pStart;                             // start ptr into sound mem
	unsigned char *   pCurr;                              // current pos in sound mem
	unsigned char *   pLoop;                              // loop ptr in sound mem
	
	int               bOn;                                // is channel active (sample playing?)
	int               bStop;                              // is channel stopped (sample _can_ still be playing, ADSR Release phase)
	int               iActFreq;                           // current psx pitch
	int               iUsedFreq;                          // current pc pitch
	int               iLeftVolume;                        // left volume
	int               bIgnoreLoop;                        // ignore loop bit, if an external loop address is used
	int               iRightVolume;                       // right volume
	int               iRawPitch;                          // raw pitch (0...3fff)
	int               iIrqDone;                           // debug irq done flag
	int               s_1;                                // last decoding infos
	int               s_2;
	int               bNoise;                             // noise active flag
	int               bFMod;                              // freq mod (0=off, 1=sound channel, 2=freq channel)
	int               iOldNoise;                          // old noise val for this channel   
	ADSRInfo          ADSR;                               // active ADSR settings
	ADSRInfoEx        ADSRX;                              // next ADSR settings (will be moved to active on sample start)
	
} SPUCHAN;

///////////////////////////////////////////////////////////

typedef struct
{
	int StartAddr;      // reverb area start addr in samples
	int CurrAddr;       // reverb area curr addr in samples
	
	int VolLeft;
	int VolRight;
	int iLastRVBLeft;
	int iLastRVBRight;
	int iRVBLeft;
	int iRVBRight;
	
	
	int FB_SRC_A;       // (offset)
	int FB_SRC_B;       // (offset)
	int IIR_ALPHA;      // (coef.)
	int ACC_COEF_A;     // (coef.)
	int ACC_COEF_B;     // (coef.)
	int ACC_COEF_C;     // (coef.)
	int ACC_COEF_D;     // (coef.)
	int IIR_COEF;       // (coef.)
	int FB_ALPHA;       // (coef.)
	int FB_X;           // (coef.)
	int IIR_DEST_A0;    // (offset)
	int IIR_DEST_A1;    // (offset)
	int ACC_SRC_A0;     // (offset)
	int ACC_SRC_A1;     // (offset)
	int ACC_SRC_B0;     // (offset)
	int ACC_SRC_B1;     // (offset)
	int IIR_SRC_A0;     // (offset)
	int IIR_SRC_A1;     // (offset)
	int IIR_DEST_B0;    // (offset)
	int IIR_DEST_B1;    // (offset)
	int ACC_SRC_C0;     // (offset)
	int ACC_SRC_C1;     // (offset)
	int ACC_SRC_D0;     // (offset)
	int ACC_SRC_D1;     // (offset)
	int IIR_SRC_B1;     // (offset)
	int IIR_SRC_B0;     // (offset)
	int MIX_DEST_A0;    // (offset)
	int MIX_DEST_A1;    // (offset)
	int MIX_DEST_B0;    // (offset)
	int MIX_DEST_B1;    // (offset)
	int IN_COEF_L;      // (coef.)
	int IN_COEF_R;      // (coef.)
} REVERBInfo;

///////////////////////////////////////////////////////////
// SPU.C globals
///////////////////////////////////////////////////////////

extern unsigned short  regArea[];                        
extern unsigned short  spuMem[];
extern unsigned char * spuMemC;
extern unsigned char * pSpuBuffer;
extern unsigned char * pSpuIrq;

// user settings

extern int        iUseXA;
extern int        iSoundMuted;
extern int        iDisStereo;

// MISC

extern SPUCHAN s_chan[];
extern REVERBInfo rvb;

extern unsigned long dwNoiseVal;
extern unsigned short spuCtrl;
extern unsigned short spuStat;
extern unsigned short spuIrq;
extern unsigned long  spuAddr;
extern int      bEndThread; 
extern int      bThreadEnded;
extern int      bSpuInit;

extern int      SSumR[];
extern int      SSumL[];
extern short *  pS;

///////////////////////////////////////////////////////////
// XA.C globals
///////////////////////////////////////////////////////////
extern xa_decode_t   * xapGlobal;
extern unsigned long * XAFeed;
extern unsigned long * XAPlay;
extern unsigned long * XAStart;
extern unsigned long * XAEnd;
extern unsigned long   XARepeat;
extern int           iLeftXAVol;
extern int           iRightXAVol;

///////////////////////////////////////////////////////////
// REVERB.C globals
///////////////////////////////////////////////////////////

// Register Defines
#define H_SPUReverbAddr  0x0da2
#define H_SPUirqAddr     0x0da4
#define H_SPUaddr        0x0da6
#define H_SPUdata        0x0da8
#define H_SPUctrl        0x0daa
#define H_SPUstat        0x0dae
#define H_SPUmvolL       0x0d80
#define H_SPUmvolR       0x0d82
#define H_SPUrvolL       0x0d84
#define H_SPUrvolR       0x0d86
#define H_SPUon1         0x0d88
#define H_SPUon2         0x0d8a
#define H_SPUoff1        0x0d8c
#define H_SPUoff2        0x0d8e
#define H_FMod1          0x0d90
#define H_FMod2          0x0d92
#define H_Noise1         0x0d94
#define H_Noise2         0x0d96
#define H_RVBon1         0x0d98
#define H_RVBon2         0x0d9a
#define H_SPUMute1       0x0d9c
#define H_SPUMute2       0x0d9e
#define H_CDLeft         0x0db0
#define H_CDRight        0x0db2
#define H_ExtLeft        0x0db4
#define H_ExtRight       0x0db6
#define H_Reverb         0x0dc0
#define H_SPUPitch0      0x0c04
#define H_SPUPitch1      0x0c14
#define H_SPUPitch2      0x0c24
#define H_SPUPitch3      0x0c34
#define H_SPUPitch4      0x0c44
#define H_SPUPitch5      0x0c54
#define H_SPUPitch6      0x0c64
#define H_SPUPitch7      0x0c74
#define H_SPUPitch8      0x0c84
#define H_SPUPitch9      0x0c94
#define H_SPUPitch10     0x0ca4
#define H_SPUPitch11     0x0cb4
#define H_SPUPitch12     0x0cc4
#define H_SPUPitch13     0x0cd4
#define H_SPUPitch14     0x0ce4
#define H_SPUPitch15     0x0cf4
#define H_SPUPitch16     0x0d04
#define H_SPUPitch17     0x0d14
#define H_SPUPitch18     0x0d24
#define H_SPUPitch19     0x0d34
#define H_SPUPitch20     0x0d44
#define H_SPUPitch21     0x0d54
#define H_SPUPitch22     0x0d64
#define H_SPUPitch23     0x0d74

#define H_SPUStartAdr0   0x0c06
#define H_SPUStartAdr1   0x0c16
#define H_SPUStartAdr2   0x0c26
#define H_SPUStartAdr3   0x0c36
#define H_SPUStartAdr4   0x0c46
#define H_SPUStartAdr5   0x0c56
#define H_SPUStartAdr6   0x0c66
#define H_SPUStartAdr7   0x0c76
#define H_SPUStartAdr8   0x0c86
#define H_SPUStartAdr9   0x0c96
#define H_SPUStartAdr10  0x0ca6
#define H_SPUStartAdr11  0x0cb6
#define H_SPUStartAdr12  0x0cc6
#define H_SPUStartAdr13  0x0cd6
#define H_SPUStartAdr14  0x0ce6
#define H_SPUStartAdr15  0x0cf6
#define H_SPUStartAdr16  0x0d06
#define H_SPUStartAdr17  0x0d16
#define H_SPUStartAdr18  0x0d26
#define H_SPUStartAdr19  0x0d36
#define H_SPUStartAdr20  0x0d46
#define H_SPUStartAdr21  0x0d56
#define H_SPUStartAdr22  0x0d66
#define H_SPUStartAdr23  0x0d76

#define H_SPULoopAdr0   0x0c0e
#define H_SPULoopAdr1   0x0c1e
#define H_SPULoopAdr2   0x0c2e
#define H_SPULoopAdr3   0x0c3e
#define H_SPULoopAdr4   0x0c4e
#define H_SPULoopAdr5   0x0c5e
#define H_SPULoopAdr6   0x0c6e
#define H_SPULoopAdr7   0x0c7e
#define H_SPULoopAdr8   0x0c8e
#define H_SPULoopAdr9   0x0c9e
#define H_SPULoopAdr10  0x0cae
#define H_SPULoopAdr11  0x0cbe
#define H_SPULoopAdr12  0x0cce
#define H_SPULoopAdr13  0x0cde
#define H_SPULoopAdr14  0x0cee
#define H_SPULoopAdr15  0x0cfe
#define H_SPULoopAdr16  0x0d0e
#define H_SPULoopAdr17  0x0d1e
#define H_SPULoopAdr18  0x0d2e
#define H_SPULoopAdr19  0x0d3e
#define H_SPULoopAdr20  0x0d4e
#define H_SPULoopAdr21  0x0d5e
#define H_SPULoopAdr22  0x0d6e
#define H_SPULoopAdr23  0x0d7e

#define H_SPU_ADSRLevel0   0x0c08
#define H_SPU_ADSRLevel1   0x0c18
#define H_SPU_ADSRLevel2   0x0c28
#define H_SPU_ADSRLevel3   0x0c38
#define H_SPU_ADSRLevel4   0x0c48
#define H_SPU_ADSRLevel5   0x0c58
#define H_SPU_ADSRLevel6   0x0c68
#define H_SPU_ADSRLevel7   0x0c78
#define H_SPU_ADSRLevel8   0x0c88
#define H_SPU_ADSRLevel9   0x0c98
#define H_SPU_ADSRLevel10  0x0ca8
#define H_SPU_ADSRLevel11  0x0cb8
#define H_SPU_ADSRLevel12  0x0cc8
#define H_SPU_ADSRLevel13  0x0cd8
#define H_SPU_ADSRLevel14  0x0ce8
#define H_SPU_ADSRLevel15  0x0cf8
#define H_SPU_ADSRLevel16  0x0d08
#define H_SPU_ADSRLevel17  0x0d18
#define H_SPU_ADSRLevel18  0x0d28
#define H_SPU_ADSRLevel19  0x0d38
#define H_SPU_ADSRLevel20  0x0d48
#define H_SPU_ADSRLevel21  0x0d58
#define H_SPU_ADSRLevel22  0x0d68
#define H_SPU_ADSRLevel23  0x0d78

extern int MixADSR(SPUCHAN *ch);
extern unsigned long SoundGetBytesBuffered(void);
extern void FeedXA(xa_decode_t *xap);
extern void MixXA(void);

#endif
