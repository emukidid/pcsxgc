////////////////////////////////////////////////////////////////////////
// XA GLOBALS
////////////////////////////////////////////////////////////////////////

#include "franspu.h"

unsigned long * XAFeed  = NULL;
unsigned long * XAPlay  = NULL;
unsigned long * XAStart = NULL;
unsigned long * XAEnd   = NULL;
unsigned long   XARepeat  = 0;

int             iLeftXAVol  = 32767;
int             iRightXAVol = 32767;

// MIX XA 
void MixXA(void)
{
	int i;
	unsigned long XALastVal = 0;
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
		unsigned long * pS=(unsigned long *)xap->pcm;
		unsigned long l=0;
		
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
		unsigned long l;short s=0;
		
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
