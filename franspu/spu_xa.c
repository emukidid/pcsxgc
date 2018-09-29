////////////////////////////////////////////////////////////////////////
// XA GLOBALS
////////////////////////////////////////////////////////////////////////

#include "franspu.h"

xa_decode_t   * xapGlobal=0;

u32 * XAFeed  = NULL;
u32 * XAPlay  = NULL;
u32 * XAStart = NULL;
u32 * XAEnd   = NULL;
u32   XARepeat  = 0;

int             iLeftXAVol  = 32767;
int             iRightXAVol = 32767;

// MIX XA 
void MixXA(void)
{
	int i;
	u32 XALastVal = 0;
	int leftvol =iLeftXAVol;
	int rightvol=iRightXAVol;
	int *ssuml=SSumL;
	int *ssumr=SSumR;
	
	for(i=0;i<NSSIZE && XAPlay!=XAFeed;i++)
	{
		XALastVal=*XAPlay++;
		if(XAPlay==XAEnd) XAPlay=XAStart;
		(*ssuml++)+=(((short)(XALastVal&0xffff))       * leftvol)/32768;
		(*ssumr++)+=(((short)((XALastVal>>16)&0xffff)) * rightvol)/32768;
	}
	
	if(XAPlay==XAFeed && XARepeat)
	{
		XARepeat--;
		for(;i<NSSIZE;i++)
		{
			(*ssuml++)+=(((short)(XALastVal&0xffff))       * leftvol)/32768;
			(*ssumr++)+=(((short)((XALastVal>>16)&0xffff)) * rightvol)/32768;
		}
	}
}

// FEED XA 
void FeedXA(xa_decode_t *xap)
{
	int sinc,spos,i,iSize;
	
	if(!bSPUIsOpen) return;
	
	xapGlobal = xap;                                      // store info for save states
	XARepeat  = 100;                                      // set up repeat
	
	iSize=((44100*xap->nsamples)/xap->freq);              // get size
	if(!iSize) return;                                    // none? bye
	
	if(XAFeed<XAPlay) {
		if ((XAPlay-XAFeed)==0) return;               // how much space in my buf?
	} else {
		if (((XAEnd-XAFeed) + (XAPlay-XAStart))==0) return;
	}
	
	spos=0x10000L;
	sinc = (xap->nsamples << 16) / iSize;                 // calc freq by num / size
	
	if(xap->stereo)
	{
		u32 * pS=(u32 *)xap->pcm;
		u32 l=0;
		
		for(i=0;i<iSize;i++)
		{
			while(spos>=0x10000L)
			{
				l = *pS++;
				spos -= 0x10000L;
			}
			
			*XAFeed++=l;
			
			if(XAFeed==XAEnd)
				XAFeed=XAStart;
			if(XAFeed==XAPlay) 
			{
				if(XAPlay!=XAStart)
					XAFeed=XAPlay-1;
				break;
			}
			
			spos += sinc;
		}
	}
	else
	{
		unsigned short * pS=(unsigned short *)xap->pcm;
		u32 l;short s=0;
		
		for(i=0;i<iSize;i++)
		{
			while(spos>=0x10000L)
			{
				s = *pS++;
				spos -= 0x10000L;
			}
			l=s;
			
			*XAFeed++=(l|(l<<16));
			
			if(XAFeed==XAEnd)
				XAFeed=XAStart;
			if(XAFeed==XAPlay) 
			{
				if(XAPlay!=XAStart)
					XAFeed=XAPlay-1;
				break;
			}
			
			spos += sinc;
		}
	}
}
