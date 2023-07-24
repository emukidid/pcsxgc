/***************************************************************************
                          freeze.c  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#include "stdafx.h"

#define _IN_FREEZE

#include "externals.h"
#include "registers.h"
#include "spu.h"

////////////////////////////////////////////////////////////////////////
// freeze structs
////////////////////////////////////////////////////////////////////////

typedef struct
{
 int            AttackModeExp;
 int            AttackTime;
 int            DecayTime;
 int            SustainLevel;
 int            SustainModeExp;
 int            SustainModeDec;
 int            SustainTime;
 int            ReleaseModeExp;
 unsigned int   ReleaseVal;
 int            ReleaseTime;
 int            ReleaseStartTime; 
 int            ReleaseVol; 
 int            lTime;
 int            lVolume;
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
 int            lVolume;
 int            lDummy1;
 int            lDummy2;
} ADSRInfoEx_orig;

typedef struct
{
 // no mutexes used anymore... don't need them to sync access
 //HANDLE            hMutex;

 int               bNew;                               // start flag

 int               iSBPos;                             // mixing stuff
 int               spos;
 int               sinc;
 int               SB[32+32];                          // Pete added another 32 dwords in 1.6 ... prevents overflow issues with gaussian/cubic interpolation (thanx xodnizel!), and can be used for even better interpolations, eh? :)
 int               sval;

 int               iStart;                             // start ptr into sound mem
 int               iCurr;                              // current pos in sound mem
 int               iLoop;                              // loop ptr in sound mem

 int               bOn;                                // is channel active (sample playing?)
 int               bStop;                              // is channel stopped (sample _can_ still be playing, ADSR Release phase)
 int               bReverb;                            // can we do reverb on this channel? must have ctrl register bit, to get active
 int               iActFreq;                           // current psx pitch
 int               iUsedFreq;                          // current pc pitch
 int               iLeftVolume;                        // left volume
 int               iLeftVolRaw;                        // left psx volume value
 int               bIgnoreLoop;                        // ignore loop bit, if an external loop address is used
 int               iMute;                              // mute mode
 int               iRightVolume;                       // right volume
 int               iRightVolRaw;                       // right psx volume value
 int               iRawPitch;                          // raw pitch (0...3fff)
 int               iIrqDone;                           // debug irq done flag
 int               s_1;                                // last decoding infos
 int               s_2;
 int               bRVBActive;                         // reverb active flag
 int               iRVBOffset;                         // reverb offset
 int               iRVBRepeat;                         // reverb repeat
 int               bNoise;                             // noise active flag
 int               bFMod;                              // freq mod (0=off, 1=sound channel, 2=freq channel)
 int               iRVBNum;                            // another reverb helper
 int               iOldNoise;                          // old noise val for this channel   
 ADSRInfo          ADSR;                               // active ADSR settings
 ADSRInfoEx_orig   ADSRX;                              // next ADSR settings (will be moved to active on sample start)
} SPUCHAN_orig;

typedef struct
{
 char          szSPUName[8];
 uint32_t ulFreezeVersion;
 uint32_t ulFreezeSize;
 unsigned char cSPUPort[0x200];
 unsigned char cSPURam[0x80000];
 xa_decode_t   xaS;     
} SPUFreeze_t;

typedef struct
{
 unsigned short  spuIrq;
 unsigned short  decode_pos;
 uint32_t   pSpuIrq;
 uint32_t   spuAddr;
 uint32_t   dummy1;
 uint32_t   dummy2;
 uint32_t   dummy3;

 SPUCHAN_orig s_chan[MAXCHAN];   

} SPUOSSFreeze_t;

////////////////////////////////////////////////////////////////////////

void LoadStateV5(SPUFreeze_t * pF);                    // newest version
void LoadStateUnknown(SPUFreeze_t * pF, uint32_t cycles); // unknown format

// we want to retain compatibility between versions,
// so use original channel struct
static void save_channel(SPUCHAN_orig *d, const SPUCHAN *s, int ch)
{
 memset(d, 0, sizeof(*d));
 d->bNew = !!(spu.dwNewChannel & (1<<ch));
 d->iSBPos = s->iSBPos;
 d->spos = s->spos;
 d->sinc = s->sinc;
 memcpy(d->SB, spu.SB + ch * SB_SIZE, sizeof(d->SB[0]) * SB_SIZE);
 d->iStart = (regAreaGetCh(ch, 6) & ~1) << 3;
 d->iCurr = 0; // set by the caller
 d->iLoop = 0; // set by the caller
 d->bOn = !!(spu.dwChannelsAudible & (1<<ch));
 d->bStop = s->ADSRX.State == ADSR_RELEASE;
 d->bReverb = s->bReverb;
 d->iActFreq = 1;
 d->iUsedFreq = 2;
 d->iLeftVolume = s->iLeftVolume;
 // this one is nasty but safe, save compat is important
 d->bIgnoreLoop = (s->prevflags ^ 2) << 1;
 d->iRightVolume = s->iRightVolume;
 d->iRawPitch = s->iRawPitch;
 d->s_1 = spu.SB[ch * SB_SIZE + 27]; // yes it's reversed
 d->s_2 = spu.SB[ch * SB_SIZE + 26];
 d->bRVBActive = s->bRVBActive;
 d->bNoise = s->bNoise;
 d->bFMod = s->bFMod;
 d->ADSRX.State = s->ADSRX.State;
 d->ADSRX.AttackModeExp = s->ADSRX.AttackModeExp;
 d->ADSRX.AttackRate = s->ADSRX.AttackRate;
 d->ADSRX.DecayRate = s->ADSRX.DecayRate;
 d->ADSRX.SustainLevel = s->ADSRX.SustainLevel;
 d->ADSRX.SustainModeExp = s->ADSRX.SustainModeExp;
 d->ADSRX.SustainIncrease = s->ADSRX.SustainIncrease;
 d->ADSRX.SustainRate = s->ADSRX.SustainRate;
 d->ADSRX.ReleaseModeExp = s->ADSRX.ReleaseModeExp;
 d->ADSRX.ReleaseRate = s->ADSRX.ReleaseRate;
 d->ADSRX.EnvelopeVol = s->ADSRX.EnvelopeVol;
 d->ADSRX.lVolume = d->bOn; // hmh
}

static void load_channel(SPUCHAN *d, const SPUCHAN_orig *s, int ch)
{
 memset(d, 0, sizeof(*d));
 if (s->bNew) spu.dwNewChannel |= 1<<ch;
 d->iSBPos = s->iSBPos;
 if ((uint32_t)d->iSBPos >= 28) d->iSBPos = 27;
 d->spos = s->spos;
 d->sinc = s->sinc;
 d->sinc_inv = 0;
 memcpy(spu.SB + ch * SB_SIZE, s->SB, sizeof(spu.SB[0]) * SB_SIZE);
 d->pCurr = (void *)((uintptr_t)s->iCurr & 0x7fff0);
 d->pLoop = (void *)((uintptr_t)s->iLoop & 0x7fff0);
 d->bReverb = s->bReverb;
 d->iLeftVolume = s->iLeftVolume;
 d->iRightVolume = s->iRightVolume;
 d->iRawPitch = s->iRawPitch;
 d->bRVBActive = s->bRVBActive;
 d->bNoise = s->bNoise;
 d->bFMod = s->bFMod;
 d->prevflags = (s->bIgnoreLoop >> 1) ^ 2;
 d->ADSRX.State = s->ADSRX.State;
 if (s->bStop) d->ADSRX.State = ADSR_RELEASE;
 d->ADSRX.AttackModeExp = s->ADSRX.AttackModeExp;
 d->ADSRX.AttackRate = s->ADSRX.AttackRate;
 d->ADSRX.DecayRate = s->ADSRX.DecayRate;
 d->ADSRX.SustainLevel = s->ADSRX.SustainLevel;
 d->ADSRX.SustainModeExp = s->ADSRX.SustainModeExp;
 d->ADSRX.SustainIncrease = s->ADSRX.SustainIncrease;
 d->ADSRX.SustainRate = s->ADSRX.SustainRate;
 d->ADSRX.ReleaseModeExp = s->ADSRX.ReleaseModeExp;
 d->ADSRX.ReleaseRate = s->ADSRX.ReleaseRate;
 d->ADSRX.EnvelopeVol = s->ADSRX.EnvelopeVol;
 if (s->bOn) spu.dwChannelsAudible |= 1<<ch;
 else d->ADSRX.EnvelopeVol = 0;
}

// force load from regArea to variables
static void load_register(unsigned long reg, unsigned int cycles)
{
 unsigned short *r = &spu.regArea[((reg & 0xfff) - 0xc00) >> 1];
 *r ^= 1;
 SPUwriteRegister(reg, *r ^ 1, cycles);
}

////////////////////////////////////////////////////////////////////////
// SPUFREEZE: called by main emu on savestate load/save
////////////////////////////////////////////////////////////////////////

long CALLBACK SPUfreeze(uint32_t ulFreezeMode, SPUFreeze_t * pF,
 uint32_t cycles)
{
 int i;SPUOSSFreeze_t * pFO;

 if(!pF) return 0;                                     // first check

 do_samples(cycles, 1);

 if(ulFreezeMode)                                      // info or save?
  {//--------------------------------------------------//
   if(ulFreezeMode==1)                                 
    memset(pF,0,sizeof(SPUFreeze_t)+sizeof(SPUOSSFreeze_t));

   strcpy(pF->szSPUName,"PBOSS");
   pF->ulFreezeVersion=5;
   pF->ulFreezeSize=sizeof(SPUFreeze_t)+sizeof(SPUOSSFreeze_t);

   if(ulFreezeMode==2) return 1;                       // info mode? ok, bye
                                                       // save mode:
   memcpy(pF->cSPURam,spu.spuMem,0x80000);             // copy common infos
   memcpy(pF->cSPUPort,spu.regArea,0x200);

   if(spu.xapGlobal && spu.XAPlay!=spu.XAFeed)         // some xa
    {
     pF->xaS=*spu.xapGlobal;
    }
   else 
   memset(&pF->xaS,0,sizeof(xa_decode_t));             // or clean xa

   pFO=(SPUOSSFreeze_t *)(pF+1);                       // store special stuff

   pFO->spuIrq = spu.regArea[(H_SPUirqAddr - 0x0c00) / 2];
   if(spu.pSpuIrq) pFO->pSpuIrq = spu.pSpuIrq - spu.spuMemC;

   pFO->spuAddr=spu.spuAddr;
   if(pFO->spuAddr==0) pFO->spuAddr=0xbaadf00d;
   pFO->decode_pos = spu.decode_pos;

   for(i=0;i<MAXCHAN;i++)
    {
     save_channel(&pFO->s_chan[i],&spu.s_chan[i],i);
     if(spu.s_chan[i].pCurr)
      pFO->s_chan[i].iCurr=spu.s_chan[i].pCurr-spu.spuMemC;
     if(spu.s_chan[i].pLoop)
      pFO->s_chan[i].iLoop=spu.s_chan[i].pLoop-spu.spuMemC;
    }

   return 1;
   //--------------------------------------------------//
  }
                                                       
 if(ulFreezeMode!=0) return 0;                         // bad mode? bye

 memcpy(spu.spuMem,pF->cSPURam,0x80000);               // get ram
 memcpy(spu.regArea,pF->cSPUPort,0x200);
 spu.bMemDirty = 1;

 if(pF->xaS.nsamples<=4032)                            // start xa again
  SPUplayADPCMchannel(&pF->xaS, spu.cycles_played, 0);

 spu.xapGlobal=0;

 if(!strcmp(pF->szSPUName,"PBOSS") && pF->ulFreezeVersion==5)
   LoadStateV5(pF);
 else LoadStateUnknown(pF, cycles);

 // repair some globals
 for(i=0;i<=62;i+=2)
  load_register(H_Reverb+i, cycles);
 load_register(H_SPUReverbAddr, cycles);
 load_register(H_SPUrvolL, cycles);
 load_register(H_SPUrvolR, cycles);

 load_register(H_SPUctrl, cycles);
 load_register(H_SPUstat, cycles);
 load_register(H_CDLeft, cycles);
 load_register(H_CDRight, cycles);

 // fix to prevent new interpolations from crashing
 for(i=0;i<MAXCHAN;i++) spu.SB[i * SB_SIZE + 28]=0;

 ClearWorkingState();
 spu.cycles_played = cycles;

 if (spu.spuCtrl & CTRL_IRQ)
  schedule_next_irq();

 return 1;
}

////////////////////////////////////////////////////////////////////////

void LoadStateV5(SPUFreeze_t * pF)
{
 int i;SPUOSSFreeze_t * pFO;

 pFO=(SPUOSSFreeze_t *)(pF+1);

 spu.pSpuIrq = spu.spuMemC + ((spu.regArea[(H_SPUirqAddr - 0x0c00) / 2] << 3) & ~0xf);

 if(pFO->spuAddr)
  {
   if (pFO->spuAddr == 0xbaadf00d) spu.spuAddr = 0;
   else spu.spuAddr = pFO->spuAddr & 0x7fffe;
  }
 spu.decode_pos = pFO->decode_pos & 0x1ff;

 spu.dwNewChannel=0;
 spu.dwChannelsAudible=0;
 spu.dwChannelDead=0;
 for(i=0;i<MAXCHAN;i++)
  {
   load_channel(&spu.s_chan[i],&pFO->s_chan[i],i);

   spu.s_chan[i].pCurr+=(uintptr_t)spu.spuMemC;
   spu.s_chan[i].pLoop+=(uintptr_t)spu.spuMemC;
  }
}

////////////////////////////////////////////////////////////////////////

void LoadStateUnknown(SPUFreeze_t * pF, uint32_t cycles)
{
 int i;

 for(i=0;i<MAXCHAN;i++)
  {
   spu.s_chan[i].pLoop=spu.spuMemC;
  }

 spu.dwNewChannel=0;
 spu.dwChannelsAudible=0;
 spu.dwChannelDead=0;
 spu.pSpuIrq=spu.spuMemC;

 for(i=0;i<0xc0;i++)
  {
   load_register(0x1f801c00 + i*2, cycles);
  }
}

////////////////////////////////////////////////////////////////////////
