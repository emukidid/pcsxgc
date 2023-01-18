#ifndef ALTERNATESPU

#include <gccore.h>
#include <malloc.h>
#include "franspu.h"
#include "../PsxCommon.h"
#include "../Decode_XA.h"
#include "../Gamecube/DEBUG.h"

extern void SoundFeedStreamData(unsigned char* pSound,long lBytes);
extern void InitADSR(void);
extern void SetupSound(void);
extern void RemoveSound(void);

// Actual SPU Buffer
#define NUM_SPU_BUFFERS 4
unsigned char spuBuffer[NUM_SPU_BUFFERS][7200] __attribute__((aligned(32)));
unsigned int  whichBuffer = 0;

// psx buffer / addresses
unsigned short  regArea[10000];                        
unsigned short  spuMem[256*1024];
unsigned char * spuMemC;
unsigned char * pSpuBuffer;
unsigned char * pMixIrq=0;

void (*irqCallback)(void)=0;                  // func of main emu, called on spu irq
unsigned char * pSpuIrq=0;
int             iSPUIRQWait=0;
int             iSpuAsyncWait=0;
unsigned int    iVolume = 3;

typedef struct
{
	char          szSPUName[8];
	unsigned long ulFreezeVersion;
	unsigned long ulFreezeSize;
	unsigned char cSPUPort[0x200];
	//unsigned char cSPURam[0x80000];
	xa_decode_t   xaS;     
} SPUFreeze_t;

typedef struct
{
 unsigned short  spuIrq;
 unsigned long   pSpuIrq;
 unsigned long   dummy0;
 unsigned long   dummy1;
 unsigned long   dummy2;
 unsigned long   dummy3;

 SPUCHAN  s_chan[MAXCHAN];   

} SPUOSSFreeze_t;

// user settings          
int	iUseXA=1;
int	iSoundMuted=0;
int	iDisStereo=0;

// infos struct for each channel
SPUCHAN         s_chan[MAXCHAN+1];                     // channel + 1 infos (1 is security for fmod handling)
REVERBInfo      rvb;

unsigned long   dwNoiseVal=1;                          // global noise generator
unsigned short  spuCtrl=0;                             // some vars to store psx reg infos
unsigned short  spuStat=0;
unsigned short  spuIrq=0;             
unsigned long   spuAddr=0xffffffff;                    // address into spu mem
int             bEndThread=0;                          // thread handlers
int             bSpuInit=0;
int             bSPUIsOpen=0;

int SSumR[NSSIZE];
int SSumL[NSSIZE];
int iFMod[NSSIZE];
short * pS;

extern void FRAN_SPU_writeRegister(unsigned long reg, unsigned short val);

// START SOUND... called by main thread to setup a new sound on a channel
void StartSound(SPUCHAN * pChannel)
{
	pChannel->ADSRX.lVolume=1;                            // Start ADSR
	pChannel->ADSRX.State=0;
	pChannel->ADSRX.EnvelopeVol=0;
	pChannel->pCurr=pChannel->pStart;                     // set sample start
	pChannel->s_1=0;                                      // init mixing vars
	pChannel->s_2=0;
	pChannel->iSBPos=28;
	pChannel->bNew=0;                                     // init channel flags
	pChannel->bStop=0;                                   
	pChannel->bOn=1;
	pChannel->SB[29]=0;                                   // init our interpolation helpers
	pChannel->SB[30]=0;
	pChannel->spos=0x10000L;
	pChannel->SB[31]=0;    				// -> no/simple interpolation starts with one 44100 decoding
}

void VoiceChangeFrequency(SPUCHAN * pChannel)
{
	pChannel->iUsedFreq=pChannel->iActFreq;               // -> take it and calc steps
	pChannel->sinc=pChannel->iRawPitch<<4;
	if(!pChannel->sinc) pChannel->sinc=1;
}

void FModChangeFrequency(SPUCHAN * pChannel,int ns)
{
	int NP=pChannel->iRawPitch;
	NP=((32768L+iFMod[ns])*NP)/32768L;
	if(NP>0x3fff) NP=0x3fff;
	if(NP<0x1)    NP=0x1;
	NP=(44100L*NP)/(4096L);                               // calc frequency
	pChannel->iActFreq=NP;
	pChannel->iUsedFreq=NP;
	pChannel->sinc=(((NP/10)<<16)/4410);
	if(!pChannel->sinc) pChannel->sinc=1;
	iFMod[ns]=0;
}                    

// noise handler... just produces some noise data
int iGetNoiseVal(SPUCHAN * pChannel)
{
	int fa;
	if((dwNoiseVal<<=1)&0x80000000L)
	{
		dwNoiseVal^=0x0040001L;
		fa=((dwNoiseVal>>2)&0x7fff);
		fa=-fa;
	}
	else
		fa=(dwNoiseVal>>2)&0x7fff;
	// mmm... depending on the noise freq we allow bigger/smaller changes to the previous val
	fa=pChannel->iOldNoise+((fa-pChannel->iOldNoise)/((0x001f-((spuCtrl&0x3f00)>>9))+1));
	if(fa>32767L)  fa=32767L;
	if(fa<-32767L) fa=-32767L;              
	pChannel->iOldNoise=fa;
	pChannel->SB[29] = fa;                               // -> store noise val in "current sample" slot
	return fa;
}                                 

void StoreInterpolationVal(SPUCHAN * pChannel,int fa)
{
	if(pChannel->bFMod==2)                                	// fmod freq channel
		pChannel->SB[29]=fa;
	else
	{
		if((spuCtrl&0x4000)==0)
			fa=0;                       		// muted?
		else                                            // else adjust
		{
			if(fa>32767L)  fa=32767L;
			if(fa<-32767L) fa=-32767L;              
		}
		pChannel->SB[29]=fa;                           	// no interpolation
	}
}

// here is the main job handler... direct func call (calculates 1 msec of sound)
void SPU_async_1ms(SPUCHAN * pChannel,int *SSumL, int *SSumR, int *iFMod)
{
	int ch;			// Channel loop
	int ns; 		// Samples loop
	unsigned int i; 	// Internal loop
	int s_1,s_2,fa;
	unsigned char * start;
	int predict_nr,shift_factor,flags,s;
	const int f[5][2] = {{0,0},{60,0},{115,-52},{98,-55},{122,-60}};
	
	memset(SSumL,0,NSSIZE*sizeof(int));
	memset(SSumR,0,NSSIZE*sizeof(int));
	
	// main channel loop
	for(ch=0;ch<MAXCHAN;ch++,pChannel++)              	// loop em all... we will collect 1 ms of sound of each playing channel
	{    
		if(pChannel->bNew) StartSound(pChannel);        // start new sound
		if(!pChannel->bOn) continue;                    // channel not playing? next
		if(pChannel->iActFreq!=pChannel->iUsedFreq)     // new psx frequency?
			VoiceChangeFrequency(pChannel);
		
		ns=0;
		while(ns<NSSIZE)                                // loop until 1 ms of data is reached
		{
			if(pChannel->bFMod==1 && iFMod[ns])     // fmod freq channel
				FModChangeFrequency(pChannel,ns);
			while(pChannel->spos>=0x10000L)
			{
				if(pChannel->iSBPos==28)        // 28 reached?
				{
					start=pChannel->pCurr;  // set up the current pos
					if (start == (unsigned char*)-1) // special "stop" sign
					{
						pChannel->bOn=0; // -> turn everything off
						pChannel->ADSRX.lVolume=0;
						pChannel->ADSRX.EnvelopeVol=0;
						goto ENDX;       // -> and done for this channel
					}
					pChannel->iSBPos=0;
					s_1=pChannel->s_1;
					s_2=pChannel->s_2;
					predict_nr=(int)*start;start++;
					shift_factor=predict_nr&0xf;
					predict_nr >>= 4;
					flags=(int)*start;start++;
					
					for (i=0;i<28;start++)      
					{
						s=((((int)*start)&0xf)<<12);
						if(s&0x8000) s|=0xffff0000;
						fa=(s >> shift_factor);
						fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
						s_2=s_1;s_1=fa;
						s=((((int)*start) & 0xf0) << 8);
						pChannel->SB[i++]=fa;
						if(s&0x8000) s|=0xffff0000;
						fa=(s>>shift_factor);              
						fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
						s_2=s_1;s_1=fa;
						pChannel->SB[i++]=fa;
					}     
					
					// irq check
					if((u32)irqCallback && (spuCtrl&0x40))         // some callback and irq active?
					{
						if((pSpuIrq >  start-16 &&              // irq address reached?
								pSpuIrq <= start) ||
								((flags&1) &&                        // special: irq on looping addr, when stop/loop flag is set 
										(pSpuIrq >  pChannel->pLoop-16 &&
												pSpuIrq <= pChannel->pLoop)))
						{
							pChannel->iIrqDone=1;                 // -> debug flag
							irqCallback();                        // -> call main emu
							
						}
					}
					
					// flag handler
					if((flags&4) && (!pChannel->bIgnoreLoop))
						pChannel->pLoop=start-16;                // loop address
					if(flags&1)                               	// 1: stop/loop
					{
						// We play this block out first...
						if(flags!=3 || pChannel->pLoop==NULL)   // PETE: if we don't check exactly for 3, loop hang ups will happen (DQ4, for example)
							start = (unsigned char*)-1;	// and checking if pLoop is set avoids crashes, yeah
						else
							start = pChannel->pLoop;
					}
					
					pChannel->pCurr=start;                    // store values for next cycle
					pChannel->s_1=s_1;
					pChannel->s_2=s_2;      
				}
				
				
				
				fa=pChannel->SB[pChannel->iSBPos++];        // get sample data
				StoreInterpolationVal(pChannel,fa);         // store val for later interpolation
				pChannel->spos -= 0x10000L;
			}
			
			if(pChannel->bNoise)
				fa=iGetNoiseVal(pChannel);               // get noise val
			else 
				fa=pChannel->SB[29];       		// get interpolation val
			
			pChannel->sval=(MixADSR(pChannel)*fa)>>10;   // mix adsr
			
			if(pChannel->bFMod==2)                        // fmod freq channel
				iFMod[ns]=pChannel->sval;             // -> store 1T sample data, use that to do fmod on next channel
			else                                          // no fmod freq channel
			{                                          
				SSumL[ns]+=(pChannel->sval*pChannel->iLeftVolume)>>14;
				SSumR[ns]+=(pChannel->sval*pChannel->iRightVolume)>>14;
			}
			
			// ok, go on until 1 ms data of this channel is collected
			ns++;                                          
			pChannel->spos += pChannel->sinc;             
		}        
		ENDX:   ;                                                      
	}
	
	// here we have another 1 ms of sound data
	
	// Mix XA
	if(XAPlay!=XAFeed || XARepeat) MixXA();
	
	if(iDisStereo) {
		// Mono Mix
		for(i=0;i<NSSIZE;i++) {
			int l, r;
			l = *SSumL++;
			if(l < -32767) l = -32767;
			else if(l > 32767) l = 32767;
			
			r = *SSumR++;
			if(r < -32767) r = -32767;
			else if(r > 32767) r = 32767;
			
			*pS++ = (l + r) / 2;
		}
	} else {
		// Stereo Mix
		for(i=0;i<NSSIZE;i++) {
			int d;
			d = *SSumL++;
			if(d < -32767) d = -32767;
			else if(d > 32767) d = 32767;
			*pS++ = d;
			
			d = *SSumR++;
			if(d < -32767) d = -32767;
			else if(d > 32767) d = 32767;
			*pS++ = d;
		}
	}
}

void FRAN_SPU_async(unsigned long cycle)
{
	if( iSoundMuted > 0 ) return;
	if(SoundGetBytesBuffered() > 8*1024) return;
	if(iSpuAsyncWait)
	{
		iSpuAsyncWait++;
		if(iSpuAsyncWait<=64) return;
		iSpuAsyncWait=0;
	}
#ifdef PROFILE
	start_section(AUDIO_SECTION);
#endif
	int i;
	int t=(cycle?32:40); /* cycle 1=NTSC 16 ms, 0=PAL 20 ms; do two frames */for (i=0;i<t;i++)
		SPU_async_1ms(s_chan,SSumL,SSumR,iFMod); // Calculates 1 ms of sound
	SoundFeedStreamData((unsigned char*)pSpuBuffer,
			((unsigned char *)pS)-((unsigned char *)pSpuBuffer));
	pSpuBuffer = spuBuffer[whichBuffer =
			((whichBuffer + 1) % NUM_SPU_BUFFERS)];
	pS=(short *)pSpuBuffer;
#ifdef PROFILE
	end_section(AUDIO_SECTION);
#endif
}

// XA AUDIO
void FRAN_SPU_playADPCMchannel(xa_decode_t *xap)
{
#ifdef PROFILE
	start_section(XA_SECTION);
#endif
	if ((iUseXA)&&(xap)&&(xap->freq))
		FeedXA(xap); // call main XA feeder
#ifdef PROFILE
	end_section(XA_SECTION);
#endif
}

// SPUINIT: this func will be called first by the main emu
long FRAN_SPU_init(void)
{
	spuMemC=(unsigned char *)spuMem;                      // just small setup
	memset((void *)s_chan,0,MAXCHAN*sizeof(SPUCHAN));
	memset((void *)&rvb,0,sizeof(REVERBInfo));
	InitADSR();
	return 0;
}

// SPUOPEN: called by main emu after init
s32 FRAN_SPU_open(void)
{
	int i;
	if(bSPUIsOpen) return 0;                              // security for some stupid main emus
	iUseXA=1;
	spuIrq=0;                       
	spuAddr=0xffffffff;
	bEndThread=0;
	spuMemC=(unsigned char *)spuMem;      
	pMixIrq=0;
	pSpuIrq=0;
	iSPUIRQWait=0;
	memset((void *)s_chan,0,(MAXCHAN+1)*sizeof(SPUCHAN));
	
	SetupSound();                                         // setup sound (before init!)
	
	//Setup streams
	pSpuBuffer = spuBuffer[whichBuffer];            // alloc mixing buffer
	XAStart = (unsigned long *)memalign(32,44100*4);           // alloc xa buffer
	XAPlay  = XAStart;
	XAFeed  = XAStart;
	XAEnd   = XAStart + 44100;
	for(i=0;i<MAXCHAN;i++)                                // loop sound channels
	{
		s_chan[i].ADSRX.SustainLevel = 0xf<<27;       // -> init sustain
		s_chan[i].iIrqDone=0;
		s_chan[i].pLoop=spuMemC;
		s_chan[i].pStart=spuMemC;
		s_chan[i].pCurr=spuMemC;
	}
	
	// Setup timers
	memset(SSumR,0,NSSIZE*sizeof(int));                   // init some mixing buffers
	memset(SSumL,0,NSSIZE*sizeof(int));
	memset(iFMod,0,NSSIZE*sizeof(int));
	pS=(short *)pSpuBuffer;                               // setup soundbuffer pointer
	
	bSPUIsOpen=1;
	return PSE_SPU_ERR_SUCCESS;        
}

// SPUCLOSE: called before shutdown
long FRAN_SPU_close(void)
{
	if(!bSPUIsOpen) return 0;                             // some security
	bSPUIsOpen=0;                                         // no more open
	
	RemoveSound();                                        // no more sound handling
	
	// Remove streams
	pSpuBuffer=NULL;
	free(XAStart);                                        // free XA buffer
	XAStart=0;
	
	return PSE_SPU_ERR_SUCCESS;
}

// SPUSHUTDOWN: called by main emu on final exit
long FRAN_SPU_shutdown(void)
{
	return PSE_SPU_ERR_SUCCESS;
}

void LoadStateV5(SPUFreeze_t * pF)
{
  int i;SPUOSSFreeze_t * pFO;
  
  pFO=(SPUOSSFreeze_t *)(pF+1);
  
  spuIrq   = pFO->spuIrq;
  if(pFO->pSpuIrq)  {
    pSpuIrq  = pFO->pSpuIrq+spuMemC;
  }  
  else {
    pSpuIrq=0;
  }
  
  for(i=0;i<MAXCHAN;i++) {
    memcpy((void *)&s_chan[i],(void *)&pFO->s_chan[i],sizeof(SPUCHAN));
    
    s_chan[i].pStart+=(unsigned long)spuMemC;
    s_chan[i].pCurr+=(unsigned long)spuMemC;
    s_chan[i].pLoop+=(unsigned long)spuMemC;
    s_chan[i].iIrqDone=0;
  }
}

////////////////////////////////////////////////////////////////////////

void LoadStateUnknown(SPUFreeze_t * pF)
{
  int i;
  
  for(i=0;i<MAXCHAN;i++) {
    s_chan[i].bOn=0;
    s_chan[i].bNew=0;
    s_chan[i].bStop=0;
    s_chan[i].ADSR.lVolume=0;
    s_chan[i].pLoop=spuMemC;
    s_chan[i].pStart=spuMemC;
    s_chan[i].pLoop=spuMemC;
    s_chan[i].iIrqDone=0;
  }

  pSpuIrq=0;
  
  for(i=0;i<0xc0;i++) {
    FRAN_SPU_writeRegister(0x1f801c00+i*2,regArea[i]);
  }
}

/*
SPUFREEZE: Used for savestates 
  ulFreezeMode types:
   Info mode == 2
   Save mode == 1
   Load mode == 0
*/
long FRAN_SPU_freeze(unsigned long ulFreezeMode,SPUFreeze_t * pF)
{
  int i;SPUOSSFreeze_t * pFO;
  if(!pF) return 0;                                     // first check
  
  if(ulFreezeMode) {                                      // info or save?
    if(ulFreezeMode==1) {
      memset(pF,0,sizeof(SPUFreeze_t)+sizeof(SPUOSSFreeze_t));
    }
    strcpy(pF->szSPUName,"PBOSS");
    pF->ulFreezeVersion=5;
    pF->ulFreezeSize=sizeof(SPUFreeze_t)+sizeof(SPUOSSFreeze_t);

    if(ulFreezeMode==2) {
      return 1;                       // info mode? ok, bye
    }
    // Save State Mode
    memcpy(pF->cSPUPort,regArea,0x200);
    
    // some xa
    if(xapGlobal && XAPlay!=XAFeed) {                    
      pF->xaS=*xapGlobal;     
    }
    else {
      memset(&pF->xaS,0,sizeof(xa_decode_t));           // or clean xa
    }
   
    pFO=(SPUOSSFreeze_t *)(pF+1);                       // store special stuff
    pFO->spuIrq=spuIrq;
   
    if(pSpuIrq) {
      pFO->pSpuIrq  = (unsigned long)pSpuIrq-(unsigned long)spuMemC;
    }

    for(i=0;i<MAXCHAN;i++) {
      memcpy((void *)&pFO->s_chan[i],(void *)&s_chan[i],sizeof(SPUCHAN));
      if(pFO->s_chan[i].pStart) {
        pFO->s_chan[i].pStart-=(unsigned long)spuMemC;
      }
      if(pFO->s_chan[i].pCurr) {
        pFO->s_chan[i].pCurr-=(unsigned long)spuMemC;
      }
      if(pFO->s_chan[i].pLoop) {
        pFO->s_chan[i].pLoop-=(unsigned long)spuMemC;
      }
    }

   return 1;
  }
                                                       
 if(ulFreezeMode!=0) { 
   return 0;                         // bad mode? bye
 }
 
  // Load State Mode
  //memcpy(spuMem,pF->cSPURam,0x80000);                   // get ram (done in Misc.c)
  memcpy(regArea,pF->cSPUPort,0x200);

  if(pF->xaS.nsamples<=4032) {                           // start xa again
    FRAN_SPU_playADPCMchannel(&pF->xaS);
  }

  xapGlobal=0;

  if(!strcmp(pF->szSPUName,"PBOSS") && pF->ulFreezeVersion==5) {
      LoadStateV5(pF);
  }
  else {
    LoadStateUnknown(pF);
  }

  // repair some globals
  for(i=0;i<=62;i+=2) {
    FRAN_SPU_writeRegister(H_Reverb+i,regArea[(H_Reverb+i-0xc00)>>1]);
  }
  FRAN_SPU_writeRegister(H_SPUReverbAddr,regArea[(H_SPUReverbAddr-0xc00)>>1]);
  FRAN_SPU_writeRegister(H_SPUrvolL,regArea[(H_SPUrvolL-0xc00)>>1]);
  FRAN_SPU_writeRegister(H_SPUrvolR,regArea[(H_SPUrvolR-0xc00)>>1]);
 
  FRAN_SPU_writeRegister(H_SPUctrl,(unsigned short)(regArea[(H_SPUctrl-0xc00)>>1]|0x4000));
  FRAN_SPU_writeRegister(H_SPUstat,regArea[(H_SPUstat-0xc00)>>1]);
  FRAN_SPU_writeRegister(H_CDLeft,regArea[(H_CDLeft-0xc00)>>1]);
  FRAN_SPU_writeRegister(H_CDRight,regArea[(H_CDRight-0xc00)>>1]);

  // fix to prevent new interpolations from crashing
  for(i=0;i<MAXCHAN;i++) {
    s_chan[i].SB[28]=0;
  }

 // repair LDChen's ADSR changes
  for(i=0;i<24;i++) {
    FRAN_SPU_writeRegister(0x1f801c00+(i<<4)+0xc8,regArea[(i<<3)+0x64]);
    FRAN_SPU_writeRegister(0x1f801c00+(i<<4)+0xca,regArea[(i<<3)+0x65]);
  }

  return 1;
}

void FRAN_SPU_setConfigFile(char *cfgfile) {
	
}

void FRAN_SPU_About() {
	
}

long FRAN_SPU_test() {
	return PSE_SPU_ERR_SUCCESS;
}

void FRAN_SPU_registerCallback(void (*callback)(void)) {
	irqCallback = callback;
}

void FRAN_SPU_registerCDDAVolume(void (*CDDAVcallback)(unsigned short,unsigned short)) {
	
}
#endif
