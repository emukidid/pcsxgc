/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2003  Pcsx Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

int psxCP2time[64] = {
        2, 16 , 1 , 1, 1, 1 , 8, 1, // 00
        1 , 1 , 1 , 1, 6 , 1 , 1 , 1, // 08
        8 , 8, 8, 19, 13 , 1 , 44 , 1, // 10
        1 , 1 , 1 , 17, 11 , 1 , 14  , 1, // 18
        30 , 1 , 1 , 1, 1, 1 , 1 , 1, // 20
        5 , 8 , 17 , 1, 1, 5, 6, 1, // 28
        23 , 1 , 1 , 1, 1, 1 , 1 , 1, // 30
        1 , 1 , 1 , 1, 1, 6 , 5  , 39  // 38
};

#define CP2_FUNC(f) \
void gte##f(); \
static void rec##f() { \
	if (pc < cop2readypc) idlecyclecount += ((cop2readypc - pc)>>2); \
	iFlushRegs(0); \
	LIW(0, (u32)psxRegs.code); \
	STW(0, OFFSET(&psxRegs, &psxRegs.code), GetHWRegSpecial(PSXREGS)); \
	FlushAllHWReg(); \
	CALLFunc ((u32)gte##f); \
	cop2readypc = pc + (psxCP2time[_fFunct_(psxRegs.code)]<<2); \
}

#define CP2_FUNCNC(f) \
void gte##f(); \
static void rec##f() { \
	if (pc < cop2readypc) idlecyclecount += ((cop2readypc - pc)>>2); \
	iFlushRegs(0); \
	CALLFunc ((u32)gte##f); \
/*	branch = 2; */\
	cop2readypc = pc + psxCP2time[_fFunct_(psxRegs.code)]; \
}

CP2_FUNC(MFC2);
CP2_FUNC(MTC2);
CP2_FUNC(CFC2);
CP2_FUNC(CTC2);
CP2_FUNC(LWC2);
CP2_FUNC(SWC2);

#if 0
void gteMFC2();
static void recMFC2() {
// Rt = Cop2D->Rd
	if (!_Rt_) return;

	iRegs[_Rt_].state = ST_UNK;

	switch (_Rd_) {
		case 29:
			MOV32ItoM((u32)&psxRegs.code, (u32)psxRegs.code);
			CALLFunc ((u32)gteMFC2);
			break;

		default:
			MOV32MtoR(EAX, (u32)&psxRegs.CP2D.r[_Rd_]);
			MOV32RtoM((u32)&psxRegs.GPR.r[_Rt_], EAX);
			break;
	}
}

void gteMTC2();
static void recMTC2() {
// Cop2D->Rd = Rt
	int fixt = 0;

//	iFlushRegs();

	switch (_Rd_) {
		case 8: case 9: case 10: case 11:
			fixt = 1; break;

		case 16: case 17: case 18: case 19:
			fixt = 2; break;

		case 15:
		case 28:
		case 30:
			MOV32ItoM((u32)&psxRegs.code, (u32)psxRegs.code);
			CALLFunc ((u32)gteMTC2);
			break;
	}

	if (IsConst(_Rt_)) {
		if (fixt == 1) MOV32ItoM((u32)&psxRegs.CP2D.r[_Rd_], (s16)iRegs[_Rt_].k);
		else if (fixt == 2) MOV32ItoM((u32)&psxRegs.CP2D.r[_Rd_], iRegs[_Rt_].k & 0xffff);
		else MOV32ItoM((u32)&psxRegs.CP2D.r[_Rd_], iRegs[_Rt_].k);
	} else {
		MOV32MtoR(EAX, (u32)&psxRegs.GPR.r[_Rt_]);
		if (fixt == 1) MOVSX32R16toR(EAX, EAX);
		else if (fixt == 2) AND32ItoR(EAX, 0xffff);
		MOV32RtoM((u32)&psxRegs.CP2D.r[_Rd_], EAX);
	}
}

void gteLWC2();
static void recLWC2() {
// Cop2D->Rt = mem[Rs + Im] (unsigned)
	int fixt = 0;

	switch (_Rt_) {
		case 8: case 9: case 10: case 11:
			fixt = 1; break;

		case 16: case 17: case 18: case 19:
			fixt = 2; break;

		case 15:
		case 28:
		case 30:
			iFlushRegs();
			MOV32ItoM((u32)&psxRegs.code, (u32)psxRegs.code);
			CALLFunc ((u32)gteLWC2);
			return;
	}

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			MOV32MtoR(EAX, (u32)&psxM[addr & 0x1fffff]);
			if (fixt == 1) MOVSX32R16toR(EAX, EAX);
			else if (fixt == 2) AND32ItoR(EAX, 0xffff);
			MOV32RtoM((u32)&psxRegs.CP2D.r[_Rt_], EAX);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			MOV32MtoR(EAX, (u32)&psxH[addr & 0xfff]);
			if (fixt == 1) MOVSX32R16toR(EAX, EAX);
			else if (fixt == 2) AND32ItoR(EAX, 0xffff);
			MOV32RtoM((u32)&psxRegs.CP2D.r[_Rt_], EAX);
			return;
		}
	}

	iPushOfB();
	CALLFunc((u32)psxMemRead32);
	if (fixt == 1) MOVSX32R16toR(EAX, EAX);
	else if (fixt == 2) AND32ItoR(EAX, 0xffff);
	MOV32RtoM((u32)&psxRegs.CP2D.r[_Rt_], EAX);
//	ADD32ItoR(ESP, 4);
	resp+= 4;
}

void gteSWC2();
static void recSWC2() {
// mem[Rs + Im] = Rt

	switch (_Rt_) {
		case 29:
			iFlushRegs();
			MOV32ItoM((u32)&psxRegs.code, (u32)psxRegs.code);
			CALLFunc ((u32)gteSWC2);
			return;
	}

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			MOV32MtoR(EAX, (u32)&psxRegs.CP2D.r[_Rt_]);
			MOV32RtoM((u32)&psxM[addr & 0x1fffff], EAX);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			MOV32MtoR(EAX, (u32)&psxRegs.CP2D.r[_Rt_]);
			MOV32RtoM((u32)&psxH[addr & 0xfff], EAX);
			return;
		}
	}

	PUSH32M  ((u32)&psxRegs.CP2D.r[_Rt_]);
	iPushOfB();
	CALLFunc((u32)psxMemWrite32);
//	ADD32ItoR(ESP, 8);
	resp+= 8;
}

static void recCFC2() {
// Rt = Cop2C->Rd
	if (!_Rt_) return;

	iRegs[_Rt_].state = ST_UNK;
	MOV32MtoR(EAX, (u32)&psxRegs.CP2C.r[_Rd_]);
	MOV32RtoM((u32)&psxRegs.GPR.r[_Rt_], EAX);
}

static void recCTC2() {
// Cop2C->Rd = Rt

	if (IsConst(_Rt_)) {
		MOV32ItoM((u32)&psxRegs.CP2C.r[_Rd_], iRegs[_Rt_].k);
	} else {
		MOV32MtoR(EAX, (u32)&psxRegs.GPR.r[_Rt_]);
		MOV32RtoM((u32)&psxRegs.CP2C.r[_Rd_], EAX);
	}
}
#endif

CP2_FUNCNC(RTPS);
CP2_FUNC(OP);
CP2_FUNCNC(NCLIP);
CP2_FUNCNC(DPCS);
CP2_FUNCNC(INTPL);
CP2_FUNC(MVMVA);
CP2_FUNCNC(NCDS);
CP2_FUNCNC(NCDT);
CP2_FUNCNC(CDP);
CP2_FUNCNC(NCCS);
CP2_FUNCNC(CC);
CP2_FUNCNC(NCS);
CP2_FUNCNC(NCT);
CP2_FUNC(SQR);
CP2_FUNCNC(DCPL);
CP2_FUNCNC(DPCT);
CP2_FUNCNC(AVSZ3);
CP2_FUNCNC(AVSZ4);
CP2_FUNCNC(RTPT);
CP2_FUNC(GPF);
CP2_FUNC(GPL);
CP2_FUNCNC(NCCT);

#if 0

#define gteVX0     ((s16*)psxRegs.CP2D.r)[0]
#define gteVY0     ((s16*)psxRegs.CP2D.r)[1]
#define gteVZ0     ((s16*)psxRegs.CP2D.r)[2]
#define gteVX1     ((s16*)psxRegs.CP2D.r)[4]
#define gteVY1     ((s16*)psxRegs.CP2D.r)[5]
#define gteVZ1     ((s16*)psxRegs.CP2D.r)[6]
#define gteVX2     ((s16*)psxRegs.CP2D.r)[8]
#define gteVY2     ((s16*)psxRegs.CP2D.r)[9]
#define gteVZ2     ((s16*)psxRegs.CP2D.r)[10]
#define gteRGB     psxRegs.CP2D.r[6]
#define gteOTZ     ((s16*)psxRegs.CP2D.r)[7*2]
#define gteIR0     ((s32*)psxRegs.CP2D.r)[8]
#define gteIR1     ((s32*)psxRegs.CP2D.r)[9]
#define gteIR2     ((s32*)psxRegs.CP2D.r)[10]
#define gteIR3     ((s32*)psxRegs.CP2D.r)[11]
#define gteSX0     ((s16*)psxRegs.CP2D.r)[12*2]
#define gteSY0     ((s16*)psxRegs.CP2D.r)[12*2+1]
#define gteSX1     ((s16*)psxRegs.CP2D.r)[13*2]
#define gteSY1     ((s16*)psxRegs.CP2D.r)[13*2+1]
#define gteSX2     ((s16*)psxRegs.CP2D.r)[14*2]
#define gteSY2     ((s16*)psxRegs.CP2D.r)[14*2+1]
#define gteSXP     ((s16*)psxRegs.CP2D.r)[15*2]
#define gteSYP     ((s16*)psxRegs.CP2D.r)[15*2+1]
#define gteSZx     ((u16*)psxRegs.CP2D.r)[16*2]
#define gteSZ0     ((u16*)psxRegs.CP2D.r)[17*2]
#define gteSZ1     ((u16*)psxRegs.CP2D.r)[18*2]
#define gteSZ2     ((u16*)psxRegs.CP2D.r)[19*2]
#define gteRGB0    psxRegs.CP2D.r[20]
#define gteRGB1    psxRegs.CP2D.r[21]
#define gteRGB2    psxRegs.CP2D.r[22]
#define gteMAC0    psxRegs.CP2D.r[24]
#define gteMAC1    ((s32*)psxRegs.CP2D.r)[25]
#define gteMAC2    ((s32*)psxRegs.CP2D.r)[26]
#define gteMAC3    ((s32*)psxRegs.CP2D.r)[27]
#define gteIRGB    psxRegs.CP2D.r[28]
#define gteORGB    psxRegs.CP2D.r[29]
#define gteLZCS    psxRegs.CP2D.r[30]
#define gteLZCR    psxRegs.CP2D.r[31]

#define gteR       ((u8 *)psxRegs.CP2D.r)[6*4]
#define gteG       ((u8 *)psxRegs.CP2D.r)[6*4+1]
#define gteB       ((u8 *)psxRegs.CP2D.r)[6*4+2]
#define gteCODE    ((u8 *)psxRegs.CP2D.r)[6*4+3]
#define gteC       gteCODE

#define gteR0      ((u8 *)psxRegs.CP2D.r)[20*4]
#define gteG0      ((u8 *)psxRegs.CP2D.r)[20*4+1]
#define gteB0      ((u8 *)psxRegs.CP2D.r)[20*4+2]
#define gteCODE0   ((u8 *)psxRegs.CP2D.r)[20*4+3]
#define gteC0      gteCODE0

#define gteR1      ((u8 *)psxRegs.CP2D.r)[21*4]
#define gteG1      ((u8 *)psxRegs.CP2D.r)[21*4+1]
#define gteB1      ((u8 *)psxRegs.CP2D.r)[21*4+2]
#define gteCODE1   ((u8 *)psxRegs.CP2D.r)[21*4+3]
#define gteC1      gteCODE1

#define gteR2      ((u8 *)psxRegs.CP2D.r)[22*4]
#define gteG2      ((u8 *)psxRegs.CP2D.r)[22*4+1]
#define gteB2      ((u8 *)psxRegs.CP2D.r)[22*4+2]
#define gteCODE2   ((u8 *)psxRegs.CP2D.r)[22*4+3]
#define gteC2      gteCODE2



#define gteR11  ((s16*)psxRegs.CP2C.r)[0]
#define gteR12  ((s16*)psxRegs.CP2C.r)[1]
#define gteR13  ((s16*)psxRegs.CP2C.r)[2]
#define gteR21  ((s16*)psxRegs.CP2C.r)[3]
#define gteR22  ((s16*)psxRegs.CP2C.r)[4]
#define gteR23  ((s16*)psxRegs.CP2C.r)[5]
#define gteR31  ((s16*)psxRegs.CP2C.r)[6]
#define gteR32  ((s16*)psxRegs.CP2C.r)[7]
#define gteR33  ((s16*)psxRegs.CP2C.r)[8]
#define gteTRX  ((s32*)psxRegs.CP2C.r)[5]
#define gteTRY  ((s32*)psxRegs.CP2C.r)[6]
#define gteTRZ  ((s32*)psxRegs.CP2C.r)[7]
#define gteL11  ((s16*)psxRegs.CP2C.r)[16]
#define gteL12  ((s16*)psxRegs.CP2C.r)[17]
#define gteL13  ((s16*)psxRegs.CP2C.r)[18]
#define gteL21  ((s16*)psxRegs.CP2C.r)[19]
#define gteL22  ((s16*)psxRegs.CP2C.r)[20]
#define gteL23  ((s16*)psxRegs.CP2C.r)[21]
#define gteL31  ((s16*)psxRegs.CP2C.r)[22]
#define gteL32  ((s16*)psxRegs.CP2C.r)[23]
#define gteL33  ((s16*)psxRegs.CP2C.r)[24]
#define gteRBK  ((s32*)psxRegs.CP2C.r)[13]
#define gteGBK  ((s32*)psxRegs.CP2C.r)[14]
#define gteBBK  ((s32*)psxRegs.CP2C.r)[15]
#define gteLR1  ((s16*)psxRegs.CP2C.r)[32]
#define gteLR2  ((s16*)psxRegs.CP2C.r)[33]
#define gteLR3  ((s16*)psxRegs.CP2C.r)[34]
#define gteLG1  ((s16*)psxRegs.CP2C.r)[35]
#define gteLG2  ((s16*)psxRegs.CP2C.r)[36]
#define gteLG3  ((s16*)psxRegs.CP2C.r)[37]
#define gteLB1  ((s16*)psxRegs.CP2C.r)[38]
#define gteLB2  ((s16*)psxRegs.CP2C.r)[39]
#define gteLB3  ((s16*)psxRegs.CP2C.r)[40]
#define gteRFC  ((s32*)psxRegs.CP2C.r)[21]
#define gteGFC  ((s32*)psxRegs.CP2C.r)[22]
#define gteBFC  ((s32*)psxRegs.CP2C.r)[23]
#define gteOFX  ((s32*)psxRegs.CP2C.r)[24]
#define gteOFY  ((s32*)psxRegs.CP2C.r)[25]
#define gteH    ((u16*)psxRegs.CP2C.r)[52]
#define gteDQA  ((s16*)psxRegs.CP2C.r)[54]
#define gteDQB  ((s32*)psxRegs.CP2C.r)[28]
#define gteZSF3 ((s16*)psxRegs.CP2C.r)[58]
#define gteZSF4 ((s16*)psxRegs.CP2C.r)[60]
#define gteFLAG psxRegs.CP2C.r[31]

//#define SUM_FLAG if(gteFLAG & 0x7F87E000) gteFLAG |= 0x80000000;

#define SUM_FLAG() { \
	TEST32ItoM((u32)&gteFLAG, 0x7F87E000); \
	j8Ptr[0] = JZ8(0); \
	OR32ItoM((u32)&gteFLAG, 0x80000000); \
 \
 	x86SetJ8(j8Ptr[0]); \
}

#define LIM32X8(reg, gteout, negv, posv, flagb) { \
 	CMP32ItoR(reg, negv); \
	j8Ptr[0] = JL8(0); \
 	CMP32ItoR(reg, posv); \
	j8Ptr[1] = JG8(0); \
 \
	MOV8RtoM((u32)&gteout, reg); \
	j8Ptr[2] = JMP8(0); \
 \
 	x86SetJ8(j8Ptr[0]); \
	MOV8ItoM((u32)&gteout, negv); \
	j8Ptr[3] = JMP8(0); \
 \
 	x86SetJ8(j8Ptr[1]); \
	MOV8ItoM((u32)&gteout, posv); \
 \
	x86SetJ8(j8Ptr[3]); \
	OR32ItoM((u32)&gteFLAG, 1<<flagb); \
 \
	x86SetJ8(j8Ptr[2]); \
}

#define _LIM_B1(reg, gteout) LIM32X8(reg, gteout, 0, 255, 21);
#define _LIM_B2(reg, gteout) LIM32X8(reg, gteout, 0, 255, 20);
#define _LIM_B3(reg, gteout) LIM32X8(reg, gteout, 0, 255, 19);

#define MAC2IRn(reg, ir, flagb, negv, posv) { \
/* 	CMP32ItoR(reg, negv);*/ \
/*	j8Ptr[0] = JL8(0); */\
/* 	CMP32ItoR(reg, posv);*/ \
/*	j8Ptr[1] = JG8(0);*/ \
 \
	MOV32RtoM((u32)&ir, reg); \
/*	j8Ptr[2] = JMP8(0);*/ \
 \
/*	x86SetJ8(j8Ptr[0]);*/ \
/*	MOV32ItoM((u32)&ir, negv);*/ \
/*	j8Ptr[3] = JMP8(0);*/ \
 \
/*	x86SetJ8(j8Ptr[1]);*/ \
/*	MOV32ItoM((u32)&ir, posv);*/ \
 \
/*	x86SetJ8(j8Ptr[3]);*/ \
/*	OR32ItoR((u32)&gteFLAG, 1<<flagb);*/ \
 \
/*	x86SetJ8(j8Ptr[2]);*/ \
}



#define gte_C11 gteLR1
#define gte_C12 gteLR2
#define gte_C13 gteLR3
#define gte_C21 gteLG1
#define gte_C22 gteLG2
#define gte_C23 gteLG3
#define gte_C31 gteLB1
#define gte_C32 gteLB2
#define gte_C33 gteLB3


#define _MVMVA_FUNC(vn, mx) { \
	MOVSX32M16toR(EAX, (u32)&mx##vn##1); \
	IMUL32R(EBX); \
/*	j8Ptr[0] = JO8(0);*/ \
	MOV32RtoR(ECX, EAX); \
 \
	MOVSX32M16toR(EAX, (u32)&mx##vn##2); \
	IMUL32R(EDI); \
/*	j8Ptr[1] = JO8(0);*/ \
	ADD32RtoR(ECX, EAX); \
/*	j8Ptr[2] = JO8(0);*/ \
 \
	MOVSX32M16toR(EAX, (u32)&mx##vn##3); \
	IMUL32R(ESI); \
/*	j8Ptr[3] = JO8(0);*/ \
	ADD32RtoR(ECX, EAX); \
/*	j8Ptr[4] = JO8(0);*/ \
}

/*	SSX = (_v0) * mx##11 + (_v1) * mx##12 + (_v2) * mx##13; 
    SSY = (_v0) * mx##21 + (_v1) * mx##22 + (_v2) * mx##23; 
    SSZ = (_v0) * mx##31 + (_v1) * mx##32 + (_v2) * mx##33; */

#define _MVMVA_ADD(_vx, jn) { \
	ADD32MtoR(ECX, (u32)&_vx); \
/*	j8Ptr[jn] = JO8(0);*/ \
}
/*			SSX+= gteRFC;
			SSY+= gteGFC;
			SSZ+= gteBFC;*/

#define _MVMVA1(vn) { \
	switch (psxRegs.code & 0x60000) { \
		case 0x00000: /* R */ \
			_MVMVA_FUNC(vn, gteR); break; \
		case 0x20000: /* L */ \
			_MVMVA_FUNC(vn, gteL); break; \
		case 0x40000: /* C */ \
			_MVMVA_FUNC(vn, gte_C); break; \
		default: \
			return; \
	} \
}

#define _MVMVA_LOAD(_v0, _v1, _v2) { \
	MOVSX32M16toR(EBX, (u32)&_v0); \
	MOVSX32M16toR(EDI, (u32)&_v1); \
	MOVSX32M16toR(ESI, (u32)&_v2); \
}

static void recMVMVA() {
	int i;

//	SysPrintf("GTE_MVMVA %lx\n", psxRegs.code & 0x1ffffff);

/*	PUSH32R(ESI);
	PUSH32R(EDI);
	PUSH32R(EBX);
*/
	XOR32RtoR(EAX, EAX); /* gteFLAG = 0 */
	MOV32RtoM((u32)&gteFLAG, EAX);

	switch (psxRegs.code & 0x18000) {
		case 0x00000: /* V0 */
			_MVMVA_LOAD(gteVX0, gteVY0, gteVZ0); break;
		case 0x08000: /* V1 */
			_MVMVA_LOAD(gteVX1, gteVY1, gteVZ1); break;
		case 0x10000: /* V2 */
			_MVMVA_LOAD(gteVX2, gteVY2, gteVZ2); break;
		case 0x18000: /* IR */
			_MVMVA_LOAD(gteIR1, gteIR2, gteIR3); break;
	}

// MAC1
	for (i=5; i<8; i++) j8Ptr[i] = 0;
	_MVMVA1(1);

	if (psxRegs.code & 0x80000) {
		SAR32ItoR(ECX, 12);
//		SSX /= 4096.0; SSY /= 4096.0; SSZ /= 4096.0;
	}

	switch (psxRegs.code & 0x6000) {
		case 0x0000: // Add TR
			_MVMVA_ADD(gteTRX, 5); break;
		case 0x2000: // Add BK
			_MVMVA_ADD(gteRBK, 6); break;
		case 0x4000: // Add FC
			_MVMVA_ADD(gteRFC, 7); break;
	}
/*
	j8Ptr[9] = JMP8(0);
	for (i=0; i<5; i++) x86SetJ8(j8Ptr[i]);
	for (i=5; i<8; i++) if (j8Ptr[i]) x86SetJ8(j8Ptr[i]);

//	TEST32ItoR(EDX, 0x80000000);
	OR32ItoM((u32)&gteFLAG, 1<<29);
	x86SetJ8(j8Ptr[9]);*/
	MOV32RtoM((u32)&gteMAC1, ECX);

	if (psxRegs.code & 0x400) {
	 	MAC2IRn(ECX, gteIR1, 24, 0, 32767);
	} else {
	 	MAC2IRn(ECX, gteIR1, 24, -32768, 32767);
	}

// MAC2
	_MVMVA1(2);

	if (psxRegs.code & 0x80000) {
		SAR32ItoR(ECX, 12);
//		SSX /= 4096.0; SSY /= 4096.0; SSZ /= 4096.0;
	}

	switch (psxRegs.code & 0x6000) {
		case 0x0000: // Add TR
			_MVMVA_ADD(gteTRY, 5); break;
		case 0x2000: // Add BK
			_MVMVA_ADD(gteGBK, 6); break;
		case 0x4000: // Add FC
			_MVMVA_ADD(gteGFC, 7); break;
	}

/*	for (i=0; i<5; i++) x86SetJ8(j8Ptr[i]);
	for (i=5; i<8; i++) if (j8Ptr[i]) x86SetJ8(j8Ptr[i]);*/
	MOV32RtoM((u32)&gteMAC2, ECX);

	if (psxRegs.code & 0x400) {
	 	MAC2IRn(ECX, gteIR2, 23, 0, 32767);
	} else {
	 	MAC2IRn(ECX, gteIR2, 23, -32768, 32767);
	}

// MAC3
	_MVMVA1(3);

	if (psxRegs.code & 0x80000) {
		SAR32ItoR(ECX, 12);
//		SSX /= 4096.0; SSY /= 4096.0; SSZ /= 4096.0;
	}

	switch (psxRegs.code & 0x6000) {
		case 0x0000: // Add TR
			_MVMVA_ADD(gteTRZ, 5); break;
		case 0x2000: // Add BK
			_MVMVA_ADD(gteBBK, 6); break;
		case 0x4000: // Add FC
			_MVMVA_ADD(gteBFC, 7); break;
	}

/*	for (i=0; i<5; i++) x86SetJ8(j8Ptr[i]);
	for (i=5; i<8; i++) if (j8Ptr[i]) x86SetJ8(j8Ptr[i]);*/
	MOV32RtoM((u32)&gteMAC3, ECX);

	if (psxRegs.code & 0x400) {
	 	MAC2IRn(ECX, gteIR3, 22, 0, 32767);
	} else {
	 	MAC2IRn(ECX, gteIR3, 22, -32768, 32767);
	}
/*		MAC2IR1()
	else MAC2IR()*/

//	SUM_FLAG();

/*	POP32R(EBX);
	POP32R(EDI);
	POP32R(ESI);*/
}

#if 0

#define _GPF1(vn) { \
	MOV32MtoR(EAX, (u32)&gteIR##vn); \
	IMUL32R(ECX); \
/*	MOV32RtoR(ECX, EAX); */\
}

static void recGPF() {
//	SysPrintf("GTE_GPF %lx\n", psxRegs.code & 0x1ffffff);

	PUSH32R(EBX);

	XOR32RtoR(EBX, EBX); /* gteFLAG = 0 */

/*		gteMAC1 = NC_OVERFLOW1(gteIR0 * gteIR1);
		gteMAC2 = NC_OVERFLOW2(gteIR0 * gteIR2);
        gteMAC3 = NC_OVERFLOW3(gteIR0 * gteIR3);*/
	MOV32MtoR(ECX, (u32)&gteIR0);
// MAC1
	_GPF1(1);

	if (psxRegs.code & 0x80000) {
		SAR32ItoR(EAX, 12);
	}
	MAC2IRn(EAX, gteIR1, 24, -32768, 32767);
	PUSH32R(EAX);

// MAC2
	_GPF1(2);

	if (psxRegs.code & 0x80000) {
		SAR32ItoR(EAX, 12);
	}
	MAC2IRn(EAX, gteIR2, 23, -32768, 32767);
	PUSH32R(EAX);

// MAC3
	_GPF1(3);

	if (psxRegs.code & 0x80000) {
		SAR32ItoR(EAX, 12);
	}
	MAC2IRn(EAX, gteIR3, 22, -32768, 32767);
//	MAC2IR();

//	gteRGB0 = gteRGB1;
//	gteRGB1 = gteRGB2;
	MOV32MtoR(EDX, (u32)&gteRGB1);
	MOV32MtoR(ECX, (u32)&gteRGB2);
	MOV32RtoM((u32)&gteRGB0, EDX);
	MOV32RtoM((u32)&gteRGB1, ECX);

	POP32R(EDX);
	POP32R(ECX);
	SAR32ItoR(ECX, 4);
	SAR32ItoR(EDX, 4);
	SAR32ItoR(EAX, 4);

	_LIM_B1(ECX, gteR2);
	_LIM_B2(EDX, gteG2);
	_LIM_B3(EAX, gteB2);
	MOV8MtoR(EAX, (u32)&gteCODE);
	MOV8RtoM((u32)&gteCODE2, EAX);

/*	gteR2 = limB1(gteMAC1 / 16.0f);
	gteG2 = limB2(gteMAC2 / 16.0f);
	gteB2 = limB3(gteMAC3 / 16.0f); gteCODE2 = gteCODE;*/

	SUM_FLAG();
	MOV32RtoM((u32)&gteFLAG, EBX);

//	POP32R(EBX);
}
#endif
#endif
