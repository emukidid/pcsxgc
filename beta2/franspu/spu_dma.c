#include "franspu.h"

// READ DMA (one value)
unsigned short  FRAN_SPU_readDMA(void)
{
 	unsigned short s=LE2HOST16(spuMem[spuAddr>>1]);
 	spuAddr+=2;
 	if(spuAddr>=0x80000) spuAddr=0;
 	return s;
}

// READ DMA (many values)
void  FRAN_SPU_readDMAMem(unsigned short * pusPSXMem,int iSize)
{
	if (spuAddr+(iSize<<1)>=0x80000)
 	{
 		memcpy(pusPSXMem,&spuMem[spuAddr>>1],0x7ffff-spuAddr+1);
		memcpy(pusPSXMem+(0x7ffff-spuAddr+1),spuMem,(iSize<<1)-(0x7ffff-spuAddr+1));
		spuAddr=(iSize<<1)-(0x7ffff-spuAddr+1);
	} else {
		memcpy(pusPSXMem,&spuMem[spuAddr>>1],iSize<<1);
		spuAddr+=(iSize<<1);	
	}
}

// WRITE DMA (one value)
void  FRAN_SPU_writeDMA(unsigned short val)
{
 	spuMem[spuAddr>>1] = HOST2LE16(val);
 	spuAddr+=2;              
 	if(spuAddr>=0x80000) spuAddr=0;
}

// WRITE DMA (many values)
void  FRAN_SPU_writeDMAMem(unsigned short * pusPSXMem,int iSize)
{
	if (spuAddr+(iSize<<1)>0x7ffff)
	{
 		memcpy(&spuMem[spuAddr>>1],pusPSXMem,0x7ffff-spuAddr+1);
		memcpy(spuMem,pusPSXMem+(0x7ffff-spuAddr+1),(iSize<<1)-(0x7ffff-spuAddr+1));
		spuAddr=(iSize<<1)-(0x7ffff-spuAddr+1);
  	} else {
  		memcpy(&spuMem[spuAddr>>1],pusPSXMem,iSize<<1);
  		spuAddr+=(iSize<<1);	
  	}
}
