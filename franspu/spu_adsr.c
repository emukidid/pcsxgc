
#include "franspu.h"

static unsigned long int RateTable[160];

/* INIT ADSR */
void InitADSR(void)
{
 	unsigned long r=3,rs=1,rd=0;
 	int i;
 	memset(RateTable,0,sizeof(unsigned long)*160);        // build the rate table according to Neill's rules (see at bottom of file)

 	for(i=32;i<160;i++)                                   // we start at pos 32 with the real values... everything before is 0
  	{
   		if(r<0x3FFFFFFF)
    		{
     			r+=rs;
     			rd++;
     			if(rd==5) {
     				rd=1;
     				rs*=2;
     			}
    		}
   		if(r>0x3FFFFFFF)
   			r=0x3FFFFFFF;
   		RateTable[i]=r;
  	}
}

static const unsigned long int TableDisp[] = {
 -0x18+0+32,-0x18+4+32,-0x18+6+32,-0x18+8+32,       // release/decay
 -0x18+9+32,-0x18+10+32,-0x18+11+32,-0x18+12+32,

 -0x1B+0+32,-0x1B+4+32,-0x1B+6+32,-0x1B+8+32,       // sustain
 -0x1B+9+32,-0x1B+10+32,-0x1B+11+32,-0x1B+12+32,
};


/* MIX ADSR */
int MixADSR(SPUCHAN *ch)
{    
 	unsigned long int disp;
 	signed long int EnvelopeVol = ch->ADSRX.EnvelopeVol;
 	
 	if(ch->bStop)                                  // should be stopped:
  	{                                                    // do release
   		if(ch->ADSRX.ReleaseModeExp)
     			disp = TableDisp[(EnvelopeVol>>28)&0x7];
   		else
     			disp=-0x0C+32;

   		EnvelopeVol-=RateTable[ch->ADSRX.ReleaseRate + disp];

   		if(EnvelopeVol<0) 
    		{
     			EnvelopeVol=0;
     			ch->bOn=0;
    		}

   		ch->ADSRX.EnvelopeVol=EnvelopeVol;
   		ch->ADSRX.lVolume=(EnvelopeVol>>=21);
   		return EnvelopeVol;
  	}
 	else                                                  // not stopped yet?
  	{
   		if(ch->ADSRX.State==0)                       // -> attack
    		{
     			disp = -0x10+32;
     			if(ch->ADSRX.AttackModeExp)
      			{
       				if(EnvelopeVol>=0x60000000)
        				disp = -0x18+32;
      			}
     			EnvelopeVol+=RateTable[ch->ADSRX.AttackRate+disp];

     			if(EnvelopeVol<0) 
      			{
       				EnvelopeVol=0x7FFFFFFF;
       				ch->ADSRX.State=1;
      			}

     			ch->ADSRX.EnvelopeVol=EnvelopeVol;
     			ch->ADSRX.lVolume=(EnvelopeVol>>=21);
     			return EnvelopeVol;
    		}

 		if(ch->ADSRX.State==1)                       // -> decay
    		{
     			disp = TableDisp[(EnvelopeVol>>28)&0x7];
     			EnvelopeVol-=RateTable[ch->ADSRX.DecayRate+disp];

     			if(EnvelopeVol<0)
     				EnvelopeVol=0;
     			if(EnvelopeVol <= ch->ADSRX.SustainLevel)
       				ch->ADSRX.State=2;

     			ch->ADSRX.EnvelopeVol=EnvelopeVol;
     			ch->ADSRX.lVolume=(EnvelopeVol>>=21);
     			return EnvelopeVol;
    		}

   		if(ch->ADSRX.State==2)                       // -> sustain
    		{
     			if(ch->ADSRX.SustainIncrease)
      			{
       				disp = -0x10+32;
       				if(ch->ADSRX.SustainModeExp)
        			{
         				if(EnvelopeVol>=0x60000000) 
          					disp = -0x18+32;
        			}
       				EnvelopeVol+=RateTable[ch->ADSRX.SustainRate+disp];

       				if(EnvelopeVol<0) 
         				EnvelopeVol=0x7FFFFFFF;
      			}
     			else
      			{
       				if(ch->ADSRX.SustainModeExp)
         				disp = TableDisp[((EnvelopeVol>>28)&0x7)+8];
       				else
         				disp=-0x0F+32;

       				EnvelopeVol-=RateTable[ch->ADSRX.SustainRate+disp];

       				if(EnvelopeVol<0) 
         				EnvelopeVol=0;
      			}
     		
     			ch->ADSRX.EnvelopeVol=EnvelopeVol;
     			ch->ADSRX.lVolume=(EnvelopeVol>>=21);
     			return EnvelopeVol;
    		}
  	}
 	return 0;
}
