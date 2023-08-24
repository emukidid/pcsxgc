/***************************************************************************
                            spu.c  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de

 Portions (C) Gražvydas "notaz" Ignotas, 2010-2012,2014,2015

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

#define _IN_SPU

#include "externals.h"
#include "registers.h"
#include "out.h"
#include "spu_config.h"

#ifdef __arm__
#include "arm_features.h"
#endif

#ifdef HAVE_ARMV7
 #define ssat32_to_16(v) \
  asm("ssat %0,#16,%1" : "=r" (v) : "r" (v))
#else
 #define ssat32_to_16(v) do { \
  if (v < -32768) v = -32768; \
  else if (v > 32767) v = 32767; \
 } while (0)
#endif

#define PSXCLK	33868800	/* 33.8688 MHz */

// intended to be ~1 frame
#define IRQ_NEAR_BLOCKS 32

/*
#if defined (USEMACOSX)
static char * libraryName     = N_("Mac OS X Sound");
#elif defined (USEALSA)
static char * libraryName     = N_("ALSA Sound");
#elif defined (USEOSS)
static char * libraryName     = N_("OSS Sound");
#elif defined (USESDL)
static char * libraryName     = N_("SDL Sound");
#elif defined (USEPULSEAUDIO)
static char * libraryName     = N_("PulseAudio Sound");
#else
static char * libraryName     = N_("NULL Sound");
#endif

static char * libraryInfo     = N_("P.E.Op.S. Sound Driver V1.7\nCoded by Pete Bernert and the P.E.Op.S. team\n");
*/

// globals

SPUInfo         spu;
SPUConfig       spu_config;

static int iFMod[NSSIZE];
static int RVB[NSSIZE * 2];
int ChanBuf[NSSIZE];

#define CDDA_BUFFER_SIZE (16384 * sizeof(uint32_t)) // must be power of 2

////////////////////////////////////////////////////////////////////////
// CODE AREA
////////////////////////////////////////////////////////////////////////

// dirty inline func includes

#include "reverb.c"
#include "adsr.c"

////////////////////////////////////////////////////////////////////////
// helpers for simple interpolation

//
// easy interpolation on upsampling, no special filter, just "Pete's common sense" tm
//
// instead of having n equal sample values in a row like:
//       ____
//           |____
//
// we compare the current delta change with the next delta change.
//
// if curr_delta is positive,
//
//  - and next delta is smaller (or changing direction):
//         \.
//          -__
//
//  - and next delta significant (at least twice) bigger:
//         --_
//            \.
//
//  - and next delta is nearly same:
//          \.
//           \.
//
//
// if curr_delta is negative,
//
//  - and next delta is smaller (or changing direction):
//          _--
//         /
//
//  - and next delta significant (at least twice) bigger:
//            /
//         __- 
//
//  - and next delta is nearly same:
//           /
//          /
//

static void InterpolateUp(int *SB, int sinc)
{
 if(SB[32]==1)                                         // flag == 1? calc step and set flag... and don't change the value in this pass
  {
   const int id1=SB[30]-SB[29];                        // curr delta to next val
   const int id2=SB[31]-SB[30];                        // and next delta to next-next val :)

   SB[32]=0;

   if(id1>0)                                           // curr delta positive
    {
     if(id2<id1)
      {SB[28]=id1;SB[32]=2;}
     else
     if(id2<(id1<<1))
      SB[28]=(id1*sinc)>>16;
     else
      SB[28]=(id1*sinc)>>17;
    }
   else                                                // curr delta negative
    {
     if(id2>id1)
      {SB[28]=id1;SB[32]=2;}
     else
     if(id2>(id1<<1))
      SB[28]=(id1*sinc)>>16;
     else
      SB[28]=(id1*sinc)>>17;
    }
  }
 else
 if(SB[32]==2)                                         // flag 1: calc step and set flag... and don't change the value in this pass
  {
   SB[32]=0;

   SB[28]=(SB[28]*sinc)>>17;
   //if(sinc<=0x8000)
   //     SB[29]=SB[30]-(SB[28]*((0x10000/sinc)-1));
   //else
   SB[29]+=SB[28];
  }
 else                                                  // no flags? add bigger val (if possible), calc smaller step, set flag1
  SB[29]+=SB[28];
}

//
// even easier interpolation on downsampling, also no special filter, again just "Pete's common sense" tm
//

static void InterpolateDown(int *SB, int sinc)
{
 if(sinc>=0x20000L)                                 // we would skip at least one val?
  {
   SB[29]+=(SB[30]-SB[29])/2;                                  // add easy weight
   if(sinc>=0x30000L)                               // we would skip even more vals?
    SB[29]+=(SB[31]-SB[30])/2;                                 // add additional next weight
  }
}

////////////////////////////////////////////////////////////////////////
// helpers for gauss interpolation

#define gval0 (((short*)(&SB[29]))[gpos&3])
#define gval(x) ((int)((short*)(&SB[29]))[(gpos+x)&3])

#include "gauss_i.h"

////////////////////////////////////////////////////////////////////////

#include "xa.c"

static void do_irq(void)
{
 //if(!(spu.spuStat & STAT_IRQ))
 {
  spu.spuStat |= STAT_IRQ;                             // asserted status?
  if(spu.irqCallback) spu.irqCallback();
 }
}

static int check_irq(int ch, unsigned char *pos)
{
 if((spu.spuCtrl & (CTRL_ON|CTRL_IRQ)) == (CTRL_ON|CTRL_IRQ) && pos == spu.pSpuIrq)
 {
  //printf("ch%d irq %04x\n", ch, pos - spu.spuMemC);
  do_irq();
  return 1;
 }
 return 0;
}

void check_irq_io(unsigned int addr)
{
 unsigned int irq_addr = regAreaGet(H_SPUirqAddr) << 3;
 //addr &= ~7; // ?
 if((spu.spuCtrl & (CTRL_ON|CTRL_IRQ)) == (CTRL_ON|CTRL_IRQ) && addr == irq_addr)
 {
  //printf("io   irq %04x\n", irq_addr);
  do_irq();
 }
}

////////////////////////////////////////////////////////////////////////
// START SOUND... called by main thread to setup a new sound on a channel
////////////////////////////////////////////////////////////////////////

static void StartSoundSB(int *SB)
{
 SB[26]=0;                                             // init mixing vars
 SB[27]=0;

 SB[28]=0;
 SB[29]=0;                                             // init our interpolation helpers
 SB[30]=0;
 SB[31]=0;
}

static void StartSoundMain(int ch)
{
 SPUCHAN *s_chan = &spu.s_chan[ch];

 StartADSR(ch);
 StartREVERB(ch);

 s_chan->prevflags=2;
 s_chan->iSBPos=27;
 s_chan->spos=0;

 s_chan->pCurr = spu.spuMemC + ((regAreaGetCh(ch, 6) & ~1) << 3);

 spu.dwNewChannel&=~(1<<ch);                           // clear new channel bit
 spu.dwChannelDead&=~(1<<ch);
 spu.dwChannelsAudible|=1<<ch;
}

static void StartSound(int ch)
{
 StartSoundMain(ch);
 StartSoundSB(spu.SB + ch * SB_SIZE);
}

////////////////////////////////////////////////////////////////////////
// ALL KIND OF HELPERS
////////////////////////////////////////////////////////////////////////

INLINE int FModChangeFrequency(int *SB, int pitch, int ns)
{
 unsigned int NP=pitch;
 int sinc;

 NP=((32768L+iFMod[ns])*NP)>>15;

 if(NP>0x3fff) NP=0x3fff;
 if(NP<0x1)    NP=0x1;

 sinc=NP<<4;                                           // calc frequency
 iFMod[ns]=0;
 SB[32]=1;                                             // reset interpolation

 return sinc;
}                    

////////////////////////////////////////////////////////////////////////

INLINE void StoreInterpolationVal(int *SB, int sinc, int fa, int fmod_freq)
{
 if(fmod_freq)                                         // fmod freq channel
  SB[29]=fa;
 else
  {
   ssat32_to_16(fa);

   if(spu_config.iUseInterpolation>=2)                 // gauss/cubic interpolation
    {
     int gpos = SB[28];
     gval0 = fa;
     gpos = (gpos+1) & 3;
     SB[28] = gpos;
    }
   else
   if(spu_config.iUseInterpolation==1)                 // simple interpolation
    {
     SB[28] = 0;
     SB[29] = SB[30];                                  // -> helpers for simple linear interpolation: delay real val for two slots, and calc the two deltas, for a 'look at the future behaviour'
     SB[30] = SB[31];
     SB[31] = fa;
     SB[32] = 1;                                       // -> flag: calc new interolation
    }
   else SB[29]=fa;                                     // no interpolation
  }
}

////////////////////////////////////////////////////////////////////////

INLINE int iGetInterpolationVal(int *SB, int sinc, int spos, int fmod_freq)
{
 int fa;

 if(fmod_freq) return SB[29];

 switch(spu_config.iUseInterpolation)
  {
   //--------------------------------------------------//
   case 3:                                             // cubic interpolation
    {
     long xd;int gpos;
     xd = (spos >> 1)+1;
     gpos = SB[28];

     fa  = gval(3) - 3*gval(2) + 3*gval(1) - gval0;
     fa *= (xd - (2<<15)) / 6;
     fa >>= 15;
     fa += gval(2) - gval(1) - gval(1) + gval0;
     fa *= (xd - (1<<15)) >> 1;
     fa >>= 15;
     fa += gval(1) - gval0;
     fa *= xd;
     fa >>= 15;
     fa = fa + gval0;

    } break;
   //--------------------------------------------------//
   case 2:                                             // gauss interpolation
    {
     int vl, vr;int gpos;
     vl = (spos >> 6) & ~3;
     gpos = SB[28];
     vr=(gauss[vl]*(int)gval0) >> 15;
     vr+=(gauss[vl+1]*gval(1)) >> 15;
     vr+=(gauss[vl+2]*gval(2)) >> 15;
     vr+=(gauss[vl+3]*gval(3)) >> 15;
     fa = vr;
    } break;
   //--------------------------------------------------//
   case 1:                                             // simple interpolation
    {
     if(sinc<0x10000L)                                 // -> upsampling?
          InterpolateUp(SB, sinc);                     // --> interpolate up
     else InterpolateDown(SB, sinc);                   // --> else down
     fa=SB[29];
    } break;
   //--------------------------------------------------//
   default:                                            // no interpolation
    {
     fa=SB[29];
    } break;
   //--------------------------------------------------//
  }

 return fa;
}

static void decode_block_data(int *dest, const unsigned char *src, int predict_nr, int shift_factor)
{
 static const int f[16][2] = {
    {    0,  0  },
    {   60,  0  },
    {  115, -52 },
    {   98, -55 },
    {  122, -60 }
 };
 int nSample;
 int fa, s_1, s_2, d, s;

 s_1 = dest[27];
 s_2 = dest[26];

 for (nSample = 0; nSample < 28; src++)
 {
  d = (int)*src;
  s = (int)(signed short)((d & 0x0f) << 12);

  fa = s >> shift_factor;
  fa += ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
  s_2=s_1;s_1=fa;

  dest[nSample++] = fa;

  s = (int)(signed short)((d & 0xf0) << 8);
  fa = s >> shift_factor;
  fa += ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
  s_2=s_1;s_1=fa;

  dest[nSample++] = fa;
 }
}

static int decode_block(void *unused, int ch, int *SB)
{
 SPUCHAN *s_chan = &spu.s_chan[ch];
 unsigned char *start;
 int predict_nr, shift_factor, flags;
 int ret = 0;

 start = s_chan->pCurr;                    // set up the current pos
 if (start == spu.spuMemC)                 // ?
  ret = 1;

 if (s_chan->prevflags & 1)                // 1: stop/loop
 {
  if (!(s_chan->prevflags & 2))
   ret = 1;

  start = s_chan->pLoop;
 }

 check_irq(ch, start);

 predict_nr = start[0];
 shift_factor = predict_nr & 0xf;
 predict_nr >>= 4;

 decode_block_data(SB, start + 2, predict_nr, shift_factor);

 flags = start[1];
 if (flags & 4 && !s_chan->bIgnoreLoop)
  s_chan->pLoop = start;                   // loop adress

 start += 16;

 s_chan->pCurr = start;                    // store values for next cycle
 s_chan->prevflags = flags;

 return ret;
}

// do block, but ignore sample data
static int skip_block(int ch)
{
 SPUCHAN *s_chan = &spu.s_chan[ch];
 unsigned char *start = s_chan->pCurr;
 int flags;
 int ret = 0;

 if (s_chan->prevflags & 1) {
  if (!(s_chan->prevflags & 2))
   ret = 1;

  start = s_chan->pLoop;
 }

 check_irq(ch, start);

 flags = start[1];
 if (flags & 4 && !s_chan->bIgnoreLoop)
  s_chan->pLoop = start;

 start += 16;

 s_chan->pCurr = start;
 s_chan->prevflags = flags;

 return ret;
}

// if irq is going to trigger sooner than in upd_samples, set upd_samples
static void scan_for_irq(int ch, unsigned int *upd_samples)
{
 SPUCHAN *s_chan = &spu.s_chan[ch];
 int pos, sinc, sinc_inv, end;
 unsigned char *block;
 int flags;

 block = s_chan->pCurr;
 pos = s_chan->spos;
 sinc = s_chan->sinc;
 end = pos + *upd_samples * sinc;
 if (s_chan->prevflags & 1)                 // 1: stop/loop
  block = s_chan->pLoop;

 pos += (28 - s_chan->iSBPos) << 16;
 while (pos < end)
 {
  if (block == spu.pSpuIrq)
   break;
  flags = block[1];
  block += 16;
  if (flags & 1) {                          // 1: stop/loop
   block = s_chan->pLoop;
  }
  pos += 28 << 16;
 }

 if (pos < end)
 {
  sinc_inv = s_chan->sinc_inv;
  if (sinc_inv == 0)
   sinc_inv = s_chan->sinc_inv = (0x80000000u / (uint32_t)sinc) << 1;

  pos -= s_chan->spos;
  *upd_samples = (((uint64_t)pos * sinc_inv) >> 32) + 1;
  //xprintf("ch%02d: irq sched: %3d %03d\n",
  // ch, *upd_samples, *upd_samples * 60 * 263 / 44100);
 }
}

#define make_do_samples(name, fmod_code, interp_start, interp1_code, interp2_code, interp_end) \
static noinline int do_samples_##name( \
 int (*decode_f)(void *context, int ch, int *SB), void *ctx, \
 int ch, int ns_to, int *SB, int sinc, int *spos, int *sbpos) \
{                                            \
 int ns, d, fa;                              \
 int ret = ns_to;                            \
 interp_start;                               \
                                             \
 for (ns = 0; ns < ns_to; ns++)              \
 {                                           \
  fmod_code;                                 \
                                             \
  *spos += sinc;                             \
  while (*spos >= 0x10000)                   \
  {                                          \
   fa = SB[(*sbpos)++];                      \
   if (*sbpos >= 28)                         \
   {                                         \
    *sbpos = 0;                              \
    d = decode_f(ctx, ch, SB);               \
    if (d && ns < ret)                       \
     ret = ns;                               \
   }                                         \
                                             \
   interp1_code;                             \
   *spos -= 0x10000;                         \
  }                                          \
                                             \
  interp2_code;                              \
 }                                           \
                                             \
 interp_end;                                 \
                                             \
 return ret;                                 \
}

#define fmod_recv_check \
  if(spu.s_chan[ch].bFMod==1 && iFMod[ns]) \
    sinc = FModChangeFrequency(SB, spu.s_chan[ch].iRawPitch, ns)

make_do_samples(default, fmod_recv_check, ,
  StoreInterpolationVal(SB, sinc, fa, spu.s_chan[ch].bFMod==2),
  ChanBuf[ns] = iGetInterpolationVal(SB, sinc, *spos, spu.s_chan[ch].bFMod==2), )
make_do_samples(noint, , fa = SB[29], , ChanBuf[ns] = fa, SB[29] = fa)

#define simple_interp_store \
  SB[28] = 0; \
  SB[29] = SB[30]; \
  SB[30] = SB[31]; \
  SB[31] = fa; \
  SB[32] = 1

#define simple_interp_get \
  if(sinc<0x10000)                /* -> upsampling? */ \
       InterpolateUp(SB, sinc);   /* --> interpolate up */ \
  else InterpolateDown(SB, sinc); /* --> else down */ \
  ChanBuf[ns] = SB[29]

make_do_samples(simple, , ,
  simple_interp_store, simple_interp_get, )

static int do_samples_skip(int ch, int ns_to)
{
 SPUCHAN *s_chan = &spu.s_chan[ch];
 int spos = s_chan->spos;
 int sinc = s_chan->sinc;
 int ret = ns_to, ns, d;

 spos += s_chan->iSBPos << 16;

 for (ns = 0; ns < ns_to; ns++)
 {
  spos += sinc;
  while (spos >= 28*0x10000)
  {
   d = skip_block(ch);
   if (d && ns < ret)
    ret = ns;
   spos -= 28*0x10000;
  }
 }

 s_chan->iSBPos = spos >> 16;
 s_chan->spos = spos & 0xffff;

 return ret;
}

static void do_lsfr_samples(int ns_to, int ctrl,
 unsigned int *dwNoiseCount, unsigned int *dwNoiseVal)
{
 unsigned int counter = *dwNoiseCount;
 unsigned int val = *dwNoiseVal;
 unsigned int level, shift, bit;
 int ns;

 // modified from DrHell/shalma, no fraction
 level = (ctrl >> 10) & 0x0f;
 level = 0x8000 >> level;

 for (ns = 0; ns < ns_to; ns++)
 {
  counter += 2;
  if (counter >= level)
  {
   counter -= level;
   shift = (val >> 10) & 0x1f;
   bit = (0x69696969 >> shift) & 1;
   bit ^= (val >> 15) & 1;
   val = (val << 1) | bit;
  }

  ChanBuf[ns] = (signed short)val;
 }

 *dwNoiseCount = counter;
 *dwNoiseVal = val;
}

static int do_samples_noise(int ch, int ns_to)
{
 int ret;

 ret = do_samples_skip(ch, ns_to);

 do_lsfr_samples(ns_to, spu.spuCtrl, &spu.dwNoiseCount, &spu.dwNoiseVal);

 return ret;
}

#ifdef HAVE_ARMV5
// asm code; lv and rv must be 0-3fff
extern void mix_chan(int *SSumLR, int count, int lv, int rv);
extern void mix_chan_rvb(int *SSumLR, int count, int lv, int rv, int *rvb);
#else
static void mix_chan(int *SSumLR, int count, int lv, int rv)
{
 const int *src = ChanBuf;
 int l, r;

 while (count--)
  {
   int sval = *src++;

   l = (sval * lv) >> 14;
   r = (sval * rv) >> 14;
   *SSumLR++ += l;
   *SSumLR++ += r;
  }
}

static void mix_chan_rvb(int *SSumLR, int count, int lv, int rv, int *rvb)
{
 const int *src = ChanBuf;
 int *dst = SSumLR;
 int *drvb = rvb;
 int l, r;

 while (count--)
  {
   int sval = *src++;

   l = (sval * lv) >> 14;
   r = (sval * rv) >> 14;
   *dst++ += l;
   *dst++ += r;
   *drvb++ += l;
   *drvb++ += r;
  }
}
#endif

// 0x0800-0x0bff  Voice 1
// 0x0c00-0x0fff  Voice 3
static noinline void do_decode_bufs(unsigned short *mem, int which,
 int count, int decode_pos)
{
 unsigned short *dst = &mem[0x800/2 + which*0x400/2];
 const int *src = ChanBuf;
 int cursor = decode_pos;

 while (count-- > 0)
  {
   cursor &= 0x1ff;
   dst[cursor] = *src++;
   cursor++;
  }

 // decode_pos is updated and irqs are checked later, after voice loop
}

static void do_silent_chans(int ns_to, int silentch)
{
 unsigned int mask;
 SPUCHAN *s_chan;
 int ch;

 mask = silentch & 0xffffff;
 for (ch = 0; mask != 0; ch++, mask >>= 1)
  {
   if (!(mask & 1)) continue;
   if (spu.dwChannelDead & (1<<ch)) continue;

   s_chan = &spu.s_chan[ch];
   if (s_chan->pCurr > spu.pSpuIrq && s_chan->pLoop > spu.pSpuIrq)
    continue;

   s_chan->spos += s_chan->iSBPos << 16;
   s_chan->iSBPos = 0;

   s_chan->spos += s_chan->sinc * ns_to;
   while (s_chan->spos >= 28 * 0x10000)
    {
     unsigned char *start = s_chan->pCurr;

     skip_block(ch);
     if (start == s_chan->pCurr || start - spu.spuMemC < 0x1000)
      {
       // looping on self or stopped(?)
       spu.dwChannelDead |= 1<<ch;
       s_chan->spos = 0;
       break;
      }

     s_chan->spos -= 28 * 0x10000;
    }
  }
}

static void do_channels(int ns_to)
{
 unsigned int mask;
 int do_rvb, ch, d;
 SPUCHAN *s_chan;
 int *SB, sinc;

 do_rvb = spu.rvb->StartAddr && spu_config.iUseReverb;
 if (do_rvb)
  memset(RVB, 0, ns_to * sizeof(RVB[0]) * 2);

 mask = spu.dwNewChannel & 0xffffff;
 for (ch = 0; mask != 0; ch++, mask >>= 1) {
  if (mask & 1)
   StartSound(ch);
 }

 mask = spu.dwChannelsAudible & 0xffffff;
 for (ch = 0; mask != 0; ch++, mask >>= 1)         // loop em all...
  {
   if (!(mask & 1)) continue;                      // channel not playing? next

   s_chan = &spu.s_chan[ch];
   SB = spu.SB + ch * SB_SIZE;
   sinc = s_chan->sinc;
   if (spu.s_chan[ch].bNewPitch)
    SB[32] = 1;                                    // reset interpolation
   spu.s_chan[ch].bNewPitch = 0;

   if (s_chan->bNoise)
    d = do_samples_noise(ch, ns_to);
   else if (s_chan->bFMod == 2
         || (s_chan->bFMod == 0 && spu_config.iUseInterpolation == 0))
    d = do_samples_noint(decode_block, NULL, ch, ns_to,
          SB, sinc, &s_chan->spos, &s_chan->iSBPos);
   else if (s_chan->bFMod == 0 && spu_config.iUseInterpolation == 1)
    d = do_samples_simple(decode_block, NULL, ch, ns_to,
          SB, sinc, &s_chan->spos, &s_chan->iSBPos);
   else
    d = do_samples_default(decode_block, NULL, ch, ns_to,
          SB, sinc, &s_chan->spos, &s_chan->iSBPos);

   d = MixADSR(&s_chan->ADSRX, d);
   if (d < ns_to) {
    spu.dwChannelsAudible &= ~(1 << ch);
    s_chan->ADSRX.State = ADSR_RELEASE;
    s_chan->ADSRX.EnvelopeVol = 0;
    memset(&ChanBuf[d], 0, (ns_to - d) * sizeof(ChanBuf[0]));
   }

   if (ch == 1 || ch == 3)
    {
     do_decode_bufs(spu.spuMem, ch/2, ns_to, spu.decode_pos);
     spu.decode_dirty_ch |= 1 << ch;
    }

   if (s_chan->bFMod == 2)                         // fmod freq channel
    memcpy(iFMod, &ChanBuf, ns_to * sizeof(iFMod[0]));
   if (s_chan->bRVBActive && do_rvb)
    mix_chan_rvb(spu.SSumLR, ns_to, s_chan->iLeftVolume, s_chan->iRightVolume, RVB);
   else
    mix_chan(spu.SSumLR, ns_to, s_chan->iLeftVolume, s_chan->iRightVolume);
  }

  MixXA(spu.SSumLR, RVB, ns_to, spu.decode_pos);

  if (spu.rvb->StartAddr) {
   if (do_rvb)
    REVERBDo(spu.SSumLR, RVB, ns_to, spu.rvb->CurrAddr);

   spu.rvb->CurrAddr += ns_to / 2;
   while (spu.rvb->CurrAddr >= 0x40000)
    spu.rvb->CurrAddr -= 0x40000 - spu.rvb->StartAddr;
  }
}

static void do_samples_finish(int *SSumLR, int ns_to,
 int silentch, int decode_pos);

// optional worker thread handling

#if P_HAVE_PTHREAD || defined(WANT_THREAD_CODE)

// worker thread state
static struct spu_worker {
 union {
  struct {
   unsigned int exit_thread;
   unsigned int i_ready;
   unsigned int i_reaped;
   unsigned int last_boot_cnt; // dsp
   unsigned int ram_dirty;
  };
  // aligning for C64X_DSP
  unsigned int _pad0[128/4];
 };
 union {
  struct {
   unsigned int i_done;
   unsigned int active; // dsp
   unsigned int boot_cnt;
  };
  unsigned int _pad1[128/4];
 };
 struct work_item {
  int ns_to;
  int ctrl;
  int decode_pos;
  int rvb_addr;
  unsigned int channels_new;
  unsigned int channels_on;
  unsigned int channels_silent;
  struct {
   int spos;
   int sbpos;
   int sinc;
   int start;
   int loop;
   short vol_l;
   short vol_r;
   unsigned short ns_to;
   unsigned short bNoise:1;
   unsigned short bFMod:2;
   unsigned short bRVBActive:1;
   unsigned short bNewPitch:1;
   ADSRInfoEx adsr;
  } ch[24];
  int SSumLR[NSSIZE * 2];
 } i[4];
} *worker;

#define WORK_MAXCNT (sizeof(worker->i) / sizeof(worker->i[0]))
#define WORK_I_MASK (WORK_MAXCNT - 1)

static void thread_work_start(void);
static void thread_work_wait_sync(struct work_item *work, int force);
static void thread_sync_caches(void);
static int  thread_get_i_done(void);

static int decode_block_work(void *context, int ch, int *SB)
{
 const unsigned char *ram = spu.spuMemC;
 int predict_nr, shift_factor, flags;
 struct work_item *work = context;
 int start = work->ch[ch].start;
 int loop = work->ch[ch].loop;

 predict_nr = ram[start];
 shift_factor = predict_nr & 0xf;
 predict_nr >>= 4;

 decode_block_data(SB, ram + start + 2, predict_nr, shift_factor);

 flags = ram[start + 1];
 if (flags & 4)
  loop = start;                            // loop adress

 start += 16;

 if (flags & 1)                            // 1: stop/loop
  start = loop;

 work->ch[ch].start = start & 0x7ffff;
 work->ch[ch].loop = loop;

 return 0;
}

static void queue_channel_work(int ns_to, unsigned int silentch)
{
 struct work_item *work;
 SPUCHAN *s_chan;
 unsigned int mask;
 int ch, d;

 work = &worker->i[worker->i_ready & WORK_I_MASK];
 work->ns_to = ns_to;
 work->ctrl = spu.spuCtrl;
 work->decode_pos = spu.decode_pos;
 work->channels_silent = silentch;

 mask = work->channels_new = spu.dwNewChannel & 0xffffff;
 for (ch = 0; mask != 0; ch++, mask >>= 1) {
  if (mask & 1)
   StartSoundMain(ch);
 }

 mask = work->channels_on = spu.dwChannelsAudible & 0xffffff;
 spu.decode_dirty_ch |= mask & 0x0a;

 for (ch = 0; mask != 0; ch++, mask >>= 1)
  {
   if (!(mask & 1)) continue;

   s_chan = &spu.s_chan[ch];
   work->ch[ch].spos = s_chan->spos;
   work->ch[ch].sbpos = s_chan->iSBPos;
   work->ch[ch].sinc = s_chan->sinc;
   work->ch[ch].adsr = s_chan->ADSRX;
   work->ch[ch].vol_l = s_chan->iLeftVolume;
   work->ch[ch].vol_r = s_chan->iRightVolume;
   work->ch[ch].start = s_chan->pCurr - spu.spuMemC;
   work->ch[ch].loop = s_chan->pLoop - spu.spuMemC;
   work->ch[ch].bNoise = s_chan->bNoise;
   work->ch[ch].bFMod = s_chan->bFMod;
   work->ch[ch].bRVBActive = s_chan->bRVBActive;
   work->ch[ch].bNewPitch = s_chan->bNewPitch;
   if (s_chan->prevflags & 1)
    work->ch[ch].start = work->ch[ch].loop;

   d = do_samples_skip(ch, ns_to);
   work->ch[ch].ns_to = d;

   // note: d is not accurate on skip
   d = SkipADSR(&s_chan->ADSRX, d);
   if (d < ns_to) {
    spu.dwChannelsAudible &= ~(1 << ch);
    s_chan->ADSRX.State = ADSR_RELEASE;
    s_chan->ADSRX.EnvelopeVol = 0;
   }
   s_chan->bNewPitch = 0;
  }

 work->rvb_addr = 0;
 if (spu.rvb->StartAddr) {
  if (spu_config.iUseReverb)
   work->rvb_addr = spu.rvb->CurrAddr;

  spu.rvb->CurrAddr += ns_to / 2;
  while (spu.rvb->CurrAddr >= 0x40000)
   spu.rvb->CurrAddr -= 0x40000 - spu.rvb->StartAddr;
 }

 worker->i_ready++;
 thread_work_start();
}

static void do_channel_work(struct work_item *work)
{
 unsigned int mask;
 int *SB, sinc, spos, sbpos;
 int d, ch, ns_to;

 ns_to = work->ns_to;

 if (work->rvb_addr)
  memset(RVB, 0, ns_to * sizeof(RVB[0]) * 2);

 mask = work->channels_new;
 for (ch = 0; mask != 0; ch++, mask >>= 1) {
  if (mask & 1)
   StartSoundSB(spu.SB + ch * SB_SIZE);
 }

 mask = work->channels_on;
 for (ch = 0; mask != 0; ch++, mask >>= 1)
  {
   if (!(mask & 1)) continue;

   d = work->ch[ch].ns_to;
   spos = work->ch[ch].spos;
   sbpos = work->ch[ch].sbpos;
   sinc = work->ch[ch].sinc;

   SB = spu.SB + ch * SB_SIZE;
   if (work->ch[ch].bNewPitch)
    SB[32] = 1; // reset interpolation

   if (work->ch[ch].bNoise)
    do_lsfr_samples(d, work->ctrl, &spu.dwNoiseCount, &spu.dwNoiseVal);
   else if (work->ch[ch].bFMod == 2
         || (work->ch[ch].bFMod == 0 && spu_config.iUseInterpolation == 0))
    do_samples_noint(decode_block_work, work, ch, d, SB, sinc, &spos, &sbpos);
   else if (work->ch[ch].bFMod == 0 && spu_config.iUseInterpolation == 1)
    do_samples_simple(decode_block_work, work, ch, d, SB, sinc, &spos, &sbpos);
   else
    do_samples_default(decode_block_work, work, ch, d, SB, sinc, &spos, &sbpos);

   d = MixADSR(&work->ch[ch].adsr, d);
   if (d < ns_to) {
    work->ch[ch].adsr.EnvelopeVol = 0;
    memset(&ChanBuf[d], 0, (ns_to - d) * sizeof(ChanBuf[0]));
   }

   if (ch == 1 || ch == 3)
    do_decode_bufs(spu.spuMem, ch/2, ns_to, work->decode_pos);

   if (work->ch[ch].bFMod == 2)                         // fmod freq channel
    memcpy(iFMod, &ChanBuf, ns_to * sizeof(iFMod[0]));
   if (work->ch[ch].bRVBActive && work->rvb_addr)
    mix_chan_rvb(work->SSumLR, ns_to,
      work->ch[ch].vol_l, work->ch[ch].vol_r, RVB);
   else
    mix_chan(work->SSumLR, ns_to, work->ch[ch].vol_l, work->ch[ch].vol_r);
  }

  if (work->rvb_addr)
   REVERBDo(work->SSumLR, RVB, ns_to, work->rvb_addr);
}

static void sync_worker_thread(int force)
{
 struct work_item *work;
 int done, used_space;

 // rvb offsets will change, thread may be using them
 force |= spu.rvb->dirty && spu.rvb->StartAddr;

 done = thread_get_i_done() - worker->i_reaped;
 used_space = worker->i_ready - worker->i_reaped;

 //printf("done: %d use: %d dsp: %u/%u\n", done, used_space,
 //  worker->boot_cnt, worker->i_done);

 while ((force && used_space > 0) || used_space >= WORK_MAXCNT || done > 0) {
  work = &worker->i[worker->i_reaped & WORK_I_MASK];
  thread_work_wait_sync(work, force);

  MixXA(work->SSumLR, RVB, work->ns_to, work->decode_pos);
  do_samples_finish(work->SSumLR, work->ns_to,
   work->channels_silent, work->decode_pos);

  worker->i_reaped++;
  done = thread_get_i_done() - worker->i_reaped;
  used_space = worker->i_ready - worker->i_reaped;
 }
 if (force)
  thread_sync_caches();
}

#else

static void queue_channel_work(int ns_to, int silentch) {}
static void sync_worker_thread(int force) {}

static const void * const worker = NULL;

#endif // P_HAVE_PTHREAD || defined(WANT_THREAD_CODE)

////////////////////////////////////////////////////////////////////////
// MAIN SPU FUNCTION
// here is the main job handler...
////////////////////////////////////////////////////////////////////////

void do_samples(unsigned int cycles_to, int do_direct)
{
 unsigned int silentch;
 int cycle_diff;
 int ns_to;

 cycle_diff = cycles_to - spu.cycles_played;
 if (cycle_diff < -2*1048576 || cycle_diff > 2*1048576)
  {
   //xprintf("desync %u %d\n", cycles_to, cycle_diff);
   spu.cycles_played = cycles_to;
   return;
  }

 silentch = ~(spu.dwChannelsAudible | spu.dwNewChannel) & 0xffffff;

 do_direct |= (silentch == 0xffffff);
 if (worker != NULL)
  sync_worker_thread(do_direct);

 if (cycle_diff < 2 * 768)
  return;

 ns_to = (cycle_diff / 768 + 1) & ~1;
 if (ns_to > NSSIZE) {
  // should never happen
  //xprintf("ns_to oflow %d %d\n", ns_to, NSSIZE);
  ns_to = NSSIZE;
 }

  //////////////////////////////////////////////////////
  // special irq handling in the decode buffers (0x0000-0x1000)
  // we know:
  // the decode buffers are located in spu memory in the following way:
  // 0x0000-0x03ff  CD audio left
  // 0x0400-0x07ff  CD audio right
  // 0x0800-0x0bff  Voice 1
  // 0x0c00-0x0fff  Voice 3
  // and decoded data is 16 bit for one sample
  // we assume:
  // even if voices 1/3 are off or no cd audio is playing, the internal
  // play positions will move on and wrap after 0x400 bytes.
  // Therefore: we just need a pointer from spumem+0 to spumem+3ff, and
  // increase this pointer on each sample by 2 bytes. If this pointer
  // (or 0x400 offsets of this pointer) hits the spuirq address, we generate
  // an IRQ.

  if (unlikely((spu.spuCtrl & CTRL_IRQ)
       && spu.pSpuIrq < spu.spuMemC+0x1000))
   {
    int irq_pos = (spu.pSpuIrq - spu.spuMemC) / 2 & 0x1ff;
    int left = (irq_pos - spu.decode_pos) & 0x1ff;
    if (0 < left && left <= ns_to)
     {
      //xprintf("decoder irq %x\n", spu.decode_pos);
      do_irq();
     }
   }
  check_irq_io(spu.spuAddr);

  if (unlikely(spu.rvb->dirty))
   REVERBPrep();

  if (do_direct || worker == NULL || !spu_config.iUseThread) {
   do_channels(ns_to);
   do_samples_finish(spu.SSumLR, ns_to, silentch, spu.decode_pos);
  }
  else {
   queue_channel_work(ns_to, silentch);
   //sync_worker_thread(1); // uncomment for debug
  }

  // advance "stopped" channels that can cause irqs
  // (all chans are always playing on the real thing..)
  if (spu.spuCtrl & CTRL_IRQ)
   do_silent_chans(ns_to, silentch);

  spu.cycles_played += ns_to * 768;
  spu.decode_pos = (spu.decode_pos + ns_to) & 0x1ff;
}

static void do_samples_finish(int *SSumLR, int ns_to,
 int silentch, int decode_pos)
{
  int vol_l = ((int)regAreaGet(H_SPUmvolL) << 17) >> 17;
  int vol_r = ((int)regAreaGet(H_SPUmvolR) << 17) >> 17;
  int ns;
  int d;

  // must clear silent channel decode buffers
  if(unlikely(silentch & spu.decode_dirty_ch & (1<<1)))
   {
    memset(&spu.spuMem[0x800/2], 0, 0x400);
    spu.decode_dirty_ch &= ~(1<<1);
   }
  if(unlikely(silentch & spu.decode_dirty_ch & (1<<3)))
   {
    memset(&spu.spuMem[0xc00/2], 0, 0x400);
    spu.decode_dirty_ch &= ~(1<<3);
   }

  vol_l = vol_l * spu_config.iVolume >> 10;
  vol_r = vol_r * spu_config.iVolume >> 10;

  if (!(spu.spuCtrl & CTRL_MUTE) || !(vol_l | vol_r))
   {
    // muted? (rare)
    memset(spu.pS, 0, ns_to * 2 * sizeof(spu.pS[0]));
    memset(SSumLR, 0, ns_to * 2 * sizeof(SSumLR[0]));
    spu.pS += ns_to * 2;
   }
  else
  for (ns = 0; ns < ns_to * 2; )
   {
    d = SSumLR[ns]; SSumLR[ns] = 0;
    d = d * vol_l >> 14;
    ssat32_to_16(d);
    *spu.pS++ = d;
    ns++;

    d = SSumLR[ns]; SSumLR[ns] = 0;
    d = d * vol_r >> 14;
    ssat32_to_16(d);
    *spu.pS++ = d;
    ns++;
   }
}

void schedule_next_irq(void)
{
 unsigned int upd_samples;
 int ch;

 if (spu.scheduleCallback == NULL)
  return;

 upd_samples = 44100 / 50;

 for (ch = 0; ch < MAXCHAN; ch++)
 {
  if (spu.dwChannelDead & (1 << ch))
   continue;
  if ((unsigned long)(spu.pSpuIrq - spu.s_chan[ch].pCurr) > IRQ_NEAR_BLOCKS * 16
    && (unsigned long)(spu.pSpuIrq - spu.s_chan[ch].pLoop) > IRQ_NEAR_BLOCKS * 16)
   continue;
  if (spu.s_chan[ch].sinc == 0)
   continue;

  scan_for_irq(ch, &upd_samples);
 }

 if (unlikely(spu.pSpuIrq < spu.spuMemC + 0x1000))
 {
  int irq_pos = (spu.pSpuIrq - spu.spuMemC) / 2 & 0x1ff;
  int left = (irq_pos - spu.decode_pos) & 0x1ff;
  if (0 < left && left < upd_samples) {
   //xprintf("decode: %3d (%3d/%3d)\n", left, spu.decode_pos, irq_pos);
   upd_samples = left;
  }
 }

 if (upd_samples < 44100 / 50)
  spu.scheduleCallback(upd_samples * 768);
}

// SPU ASYNC... even newer epsxe func
//  1 time every 'cycle' cycles... harhar

// rearmed: called dynamically now

void CALLBACK SPUasync(unsigned int cycle, unsigned int flags)
{
 do_samples(cycle, spu_config.iUseFixedUpdates);

 if (spu.spuCtrl & CTRL_IRQ)
  schedule_next_irq();

 if (flags & 1) {
  out_current->feed(spu.pSpuBuffer, (unsigned char *)spu.pS - spu.pSpuBuffer);
  spu.pS = (short *)spu.pSpuBuffer;

  if (spu_config.iTempo) {
   if (!out_current->busy())
    // cause more samples to be generated
    // (and break some games because of bad sync)
    spu.cycles_played -= 44100 / 60 / 2 * 768;
  }
 }
}

// SPU UPDATE... new epsxe func
//  1 time every 32 hsync lines
//  (312/32)x50 in pal
//  (262/32)x60 in ntsc

// since epsxe 1.5.2 (linux) uses SPUupdate, not SPUasync, I will
// leave that func in the linux port, until epsxe linux is using
// the async function as well

void CALLBACK SPUupdate(void)
{
}

// XA AUDIO

void CALLBACK SPUplayADPCMchannel(xa_decode_t *xap, unsigned int cycle, int is_start)
{
 if(!xap)       return;
 if(!xap->freq) return;                // no xa freq ? bye

 if (is_start)
  do_samples(cycle, 1);                // catch up to prevent source underflows later

 FeedXA(xap);                          // call main XA feeder
}

// CDDA AUDIO
int CALLBACK SPUplayCDDAchannel(short *pcm, int nbytes, unsigned int cycle, int is_start)
{
 if (!pcm)      return -1;
 if (nbytes<=0) return -1;

 if (is_start)
  do_samples(cycle, 1);                // catch up to prevent source underflows later

 return FeedCDDA((unsigned char *)pcm, nbytes);
}

// to be called after state load
void ClearWorkingState(void)
{
 memset(iFMod, 0, sizeof(iFMod));
 spu.pS=(short *)spu.pSpuBuffer;                       // setup soundbuffer pointer
}

// SETUPSTREAMS: init most of the spu buffers
static void SetupStreams(void)
{ 
 spu.pSpuBuffer = (unsigned char *)malloc(32768);      // alloc mixing buffer
 spu.SSumLR = calloc(NSSIZE * 2, sizeof(spu.SSumLR[0]));

 spu.XAStart = malloc(44100 * sizeof(uint32_t));       // alloc xa buffer
 spu.XAEnd   = spu.XAStart + 44100;
 spu.XAPlay  = spu.XAStart;
 spu.XAFeed  = spu.XAStart;

 spu.CDDAStart = malloc(CDDA_BUFFER_SIZE);             // alloc cdda buffer
 spu.CDDAEnd   = spu.CDDAStart + 16384;
 spu.CDDAPlay  = spu.CDDAStart;
 spu.CDDAFeed  = spu.CDDAStart;

 ClearWorkingState();
}

// REMOVESTREAMS: free most buffer
static void RemoveStreams(void)
{ 
 free(spu.pSpuBuffer);                                 // free mixing buffer
 spu.pSpuBuffer = NULL;
 free(spu.SSumLR);
 spu.SSumLR = NULL;
 free(spu.XAStart);                                    // free XA buffer
 spu.XAStart = NULL;
 free(spu.CDDAStart);                                  // free CDDA buffer
 spu.CDDAStart = NULL;
}

#if defined(C64X_DSP)

/* special code for TI C64x DSP */
#include "spu_c64x.c"

#elif P_HAVE_PTHREAD

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

static struct {
 pthread_t thread;
 sem_t sem_avail;
 sem_t sem_done;
} t;

/* generic pthread implementation */

static void thread_work_start(void)
{
 sem_post(&t.sem_avail);
}

static void thread_work_wait_sync(struct work_item *work, int force)
{
 sem_wait(&t.sem_done);
}

static int thread_get_i_done(void)
{
 return worker->i_done;
}

static void thread_sync_caches(void)
{
}

static void *spu_worker_thread(void *unused)
{
 struct work_item *work;

 while (1) {
  sem_wait(&t.sem_avail);
  if (worker->exit_thread)
   break;

  work = &worker->i[worker->i_done & WORK_I_MASK];
  do_channel_work(work);
  worker->i_done++;

  sem_post(&t.sem_done);
 }

 return NULL;
}

static void init_spu_thread(void)
{
 int ret;

 if (sysconf(_SC_NPROCESSORS_ONLN) <= 1)
  return;

 worker = calloc(1, sizeof(*worker));
 if (worker == NULL)
  return;
 ret = sem_init(&t.sem_avail, 0, 0);
 if (ret != 0)
  goto fail_sem_avail;
 ret = sem_init(&t.sem_done, 0, 0);
 if (ret != 0)
  goto fail_sem_done;

 ret = pthread_create(&t.thread, NULL, spu_worker_thread, NULL);
 if (ret != 0)
  goto fail_thread;

 spu_config.iThreadAvail = 1;
 return;

fail_thread:
 sem_destroy(&t.sem_done);
fail_sem_done:
 sem_destroy(&t.sem_avail);
fail_sem_avail:
 free(worker);
 worker = NULL;
 spu_config.iThreadAvail = 0;
}

static void exit_spu_thread(void)
{
 if (worker == NULL)
  return;
 worker->exit_thread = 1;
 sem_post(&t.sem_avail);
 pthread_join(t.thread, NULL);
 sem_destroy(&t.sem_done);
 sem_destroy(&t.sem_avail);
 free(worker);
 worker = NULL;
}

#else // if !P_HAVE_PTHREAD

static void init_spu_thread(void)
{
}

static void exit_spu_thread(void)
{
}

#endif

// SPUINIT: this func will be called first by the main emu
long CALLBACK SPUinit(void)
{
 int i;

 memset(&spu, 0, sizeof(spu));
 spu.spuMemC = calloc(1, 512 * 1024);
 InitADSR();

 spu.s_chan = calloc(MAXCHAN+1, sizeof(spu.s_chan[0])); // channel + 1 infos (1 is security for fmod handling)
 spu.rvb = calloc(1, sizeof(REVERBInfo));
 spu.SB = calloc(MAXCHAN, sizeof(spu.SB[0]) * SB_SIZE);

 spu.spuAddr = 0;
 spu.decode_pos = 0;
 spu.pSpuIrq = spu.spuMemC;

 SetupStreams();                                       // prepare streaming

 if (spu_config.iVolume == 0)
  spu_config.iVolume = 768; // 1024 is 1.0

 init_spu_thread();

 for (i = 0; i < MAXCHAN; i++)                         // loop sound channels
  {
   spu.s_chan[i].ADSRX.SustainLevel = 0xf;             // -> init sustain
   spu.s_chan[i].ADSRX.SustainIncrease = 1;
   spu.s_chan[i].pLoop = spu.spuMemC;
   spu.s_chan[i].pCurr = spu.spuMemC;
   spu.s_chan[i].bIgnoreLoop = 0;
  }

 spu.bSpuInit=1;                                       // flag: we are inited

 return 0;
}

// SPUOPEN: called by main emu after init
long CALLBACK SPUopen(void)
{
 if (spu.bSPUIsOpen) return 0;                         // security for some stupid main emus

 SetupSound();                                         // setup sound (before init!)

 spu.bSPUIsOpen = 1;

 return PSE_SPU_ERR_SUCCESS;
}

// SPUCLOSE: called before shutdown
long CALLBACK SPUclose(void)
{
 if (!spu.bSPUIsOpen) return 0;                        // some security

 spu.bSPUIsOpen = 0;                                   // no more open

 out_current->finish();                                // no more sound handling

 return 0;
}

// SPUSHUTDOWN: called by main emu on final exit
long CALLBACK SPUshutdown(void)
{
 SPUclose();

 exit_spu_thread();

 free(spu.spuMemC);
 spu.spuMemC = NULL;
 free(spu.SB);
 spu.SB = NULL;
 free(spu.s_chan);
 spu.s_chan = NULL;
 free(spu.rvb);
 spu.rvb = NULL;

 RemoveStreams();                                      // no more streaming
 spu.bSpuInit=0;

 return 0;
}

// SPUTEST: we don't test, we are always fine ;)
long CALLBACK SPUtest(void)
{
 return 0;
}

// SPUCONFIGURE: call config dialog
long CALLBACK SPUconfigure(void)
{
#ifdef _MACOSX
 DoConfiguration();
#else
// StartCfgTool("CFG");
#endif
 return 0;
}

// SPUABOUT: show about window
void CALLBACK SPUabout(void)
{
#ifdef _MACOSX
 DoAbout();
#else
// StartCfgTool("ABOUT");
#endif
}

// SETUP CALLBACKS
// this functions will be called once, 
// passes a callback that should be called on SPU-IRQ/cdda volume change
void CALLBACK SPUregisterCallback(void (CALLBACK *callback)(void))
{
 spu.irqCallback = callback;
}

void CALLBACK SPUregisterCDDAVolume(void (CALLBACK *CDDAVcallback)(short, short))
{
 spu.cddavCallback = CDDAVcallback;
}

void CALLBACK SPUregisterScheduleCb(void (CALLBACK *callback)(unsigned int))
{
 spu.scheduleCallback = callback;
}

// COMMON PLUGIN INFO FUNCS
/*
char * CALLBACK PSEgetLibName(void)
{
 return _(libraryName);
}

unsigned long CALLBACK PSEgetLibType(void)
{
 return  PSE_LT_SPU;
}

unsigned long CALLBACK PSEgetLibVersion(void)
{
 return (1 << 16) | (6 << 8);
}

char * SPUgetLibInfos(void)
{
 return _(libraryInfo);
}
*/

// debug
void spu_get_debug_info(int *chans_out, int *run_chans, int *fmod_chans_out, int *noise_chans_out)
{
 int ch = 0, fmod_chans = 0, noise_chans = 0, irq_chans = 0;

 if (spu.s_chan == NULL)
  return;

 for(;ch<MAXCHAN;ch++)
 {
  if (!(spu.dwChannelsAudible & (1<<ch)))
   continue;
  if (spu.s_chan[ch].bFMod == 2)
   fmod_chans |= 1 << ch;
  if (spu.s_chan[ch].bNoise)
   noise_chans |= 1 << ch;
  if((spu.spuCtrl&CTRL_IRQ) && spu.s_chan[ch].pCurr <= spu.pSpuIrq && spu.s_chan[ch].pLoop <= spu.pSpuIrq)
   irq_chans |= 1 << ch;
 }

 *chans_out = spu.dwChannelsAudible;
 *run_chans = ~spu.dwChannelsAudible & ~spu.dwChannelDead & irq_chans;
 *fmod_chans_out = fmod_chans;
 *noise_chans_out = noise_chans;
}

// vim:shiftwidth=1:expandtab
