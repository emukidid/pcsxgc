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

/*
* Miscellaneous functions, including savesates and CD-ROM loading.
*/

#include "Misc.h"
#include "CdRom.h"
#include "PsxHw.h"
#include "Mdec.h"
#include "Gamecube/wiiSXconfig.h"
#include "Gamecube/fileBrowser/fileBrowser-libfat.h"

int Log = 0;

/* PSX Executable types */
#define PSX_EXE     1
#define CPE_EXE     2
#define COFF_EXE    3
#define INVALID_EXE 4

#define ISODCL(from, to) (to - from + 1)

struct iso_directory_record {
	char length			[ISODCL (1, 1)]; /* 711 */
	char ext_attr_length		[ISODCL (2, 2)]; /* 711 */
	char extent			[ISODCL (3, 10)]; /* 733 */
	char size			[ISODCL (11, 18)]; /* 733 */
	char date			[ISODCL (19, 25)]; /* 7 by 711 */
	char flags			[ISODCL (26, 26)];
	char file_unit_size		[ISODCL (27, 27)]; /* 711 */
	char interleave			[ISODCL (28, 28)]; /* 711 */
	char volume_sequence_number	[ISODCL (29, 32)]; /* 723 */
	unsigned char name_len		[ISODCL (33, 33)]; /* 711 */
	char name			[1];
};

#define btoi(b)		((b)/16*10 + (b)%16)		/* BCD to u_char */
#define itob(i)		((i)/10*16 + (i)%10)		/* u_char to BCD */

void mmssdd( char *b, char *p )
 {
	int m, s, d;
#if defined(HW_RVL) || defined(HW_DOL) || defined(BIG_ENDIAN)
	int block = (b[0]&0xff) | ((b[1]&0xff)<<8) | ((b[2]&0xff)<<16) | (b[3]<<24);
#else
	int block = *((int*)b);
#endif
	
	block += 150;
	m = block / 4500;			// minuten
	block = block - m * 4500;	// minuten rest
	s = block / 75;				// sekunden
	d = block - s * 75;			// sekunden rest
	
	m = ( ( m / 10 ) << 4 ) | m % 10;
	s = ( ( s / 10 ) << 4 ) | s % 10;
	d = ( ( d / 10 ) << 4 ) | d % 10;	
	
	p[0] = m;
	p[1] = s;
	p[2] = d;
}

#define incTime() \
	time[0] = btoi(time[0]); time[1] = btoi(time[1]); time[2] = btoi(time[2]); \
	time[2]++; \
	if(time[2] == 75) { \
		time[2] = 0; \
		time[1]++; \
		if (time[1] == 60) { \
			time[1] = 0; \
			time[0]++; \
		} \
	} \
	time[0] = itob(time[0]); time[1] = itob(time[1]); time[2] = itob(time[2]);

#define READTRACK() \
	if (CDR_readTrack(time) == -1) return -1; \
	buf = CDR_getBuffer(); if (buf == NULL) return -1;

#define READDIR(_dir) \
	READTRACK(); \
	memcpy(_dir, buf+12, 2048); \
 \
	incTime(); \
	READTRACK(); \
	memcpy(_dir+2048, buf+12, 2048);

int GetCdromFile(u8 *mdir, u8 *time, char *filename) {
	struct iso_directory_record *dir;
	char ddir[4096];
	u8 *buf;
	int i;

	// only try to scan if a filename is given
	if(!strlen((char*)filename)) return -1;
	
	i = 0;
	while (i < 4096) {
		dir = (struct iso_directory_record*) &mdir[i];
		if (dir->length[0] == 0) {
			return -1;
		}
		i += dir->length[0];

		if (dir->flags[0] & 0x2) { // it's a dir
			if (!strnicmp((char*)&dir->name[0], (char*)filename, dir->name_len[0])) {
				if (filename[dir->name_len[0]] != '\\') continue;
				
				filename+= dir->name_len[0] + 1;

				mmssdd(dir->extent, (char*)time);
				READDIR(ddir);
				i = 0;
			}
		} else {
			if (!strnicmp((char*)&dir->name[0], (char*)filename, strlen((char*)filename))) {
				mmssdd(dir->extent, (char*)time);
				break;
			}
		}
	}
	return 0;
}

int LoadCdrom() {
	EXE_HEADER tmpHead;
	struct iso_directory_record *dir;
	u8 time[4],*buf;
	u8 mdir[4096];
	char exename[256];


	if (!Config.HLE) {
		psxRegs.pc = psxRegs.GPR.n.ra;
		return 0;
	}

	time[0] = itob(0); time[1] = itob(2); time[2] = itob(0x10);

	READTRACK();

	// skip head and sub, and go to the root directory record
	dir = (struct iso_directory_record*) &buf[12+156]; 

	mmssdd(dir->extent, (char*)time);

	READDIR(mdir);

	// Load SYSTEM.CNF and scan for the main executable
	if (GetCdromFile(mdir, time, "SYSTEM.CNF;1") == -1) {
		// if SYSTEM.CNF is missing, start an existing PSX.EXE
		if (GetCdromFile(mdir, time, "PSX.EXE;1") == -1) return -1;

		READTRACK();
	}
	else {
		// read the SYSTEM.CNF
		READTRACK();

		sscanf((char*)buf+12, "BOOT = cdrom:\\%256s", exename);
		if (GetCdromFile(mdir, time, exename) == -1) {
			sscanf((char*)buf+12, "BOOT = cdrom:%256s", exename);
			if (GetCdromFile(mdir, time, exename) == -1) {
				char *ptr = strstr((char*)buf+12, "cdrom:");
				if(ptr) {
					strncpy(exename, ptr, 256);
					if (GetCdromFile(mdir, time, exename) == -1)
						return -1;
				}
			}
		}

		// Read the EXE-Header
		READTRACK();
	}

	
	memcpy(&tmpHead, buf+12, sizeof(EXE_HEADER));

	psxRegs.pc = SWAP32(tmpHead.pc0);
	psxRegs.GPR.n.gp = SWAP32(tmpHead.gp0);
	psxRegs.GPR.n.sp = SWAP32(tmpHead.s_addr); 
	if (psxRegs.GPR.n.sp == 0) psxRegs.GPR.n.sp = 0x801fff00;

	tmpHead.t_size = SWAP32(tmpHead.t_size);
	tmpHead.t_addr = SWAP32(tmpHead.t_addr);

	// Read the rest of the main executable
	while (tmpHead.t_size) {
		void *ptr = (void *)PSXM(tmpHead.t_addr);

		incTime();
		READTRACK();

		if (ptr != NULL) memcpy(ptr, buf+12, 2048);

		tmpHead.t_size -= 2048;
		tmpHead.t_addr += 2048;
	}

	return 0;
}

int LoadCdromFile(char *filename, EXE_HEADER *head) {
	struct iso_directory_record *dir;
	u8 time[4],*buf;
	u8 mdir[4096], exename[256];
	u32 size, addr;

	sscanf(filename, "cdrom:\\%256s", exename);

	time[0] = itob(0); time[1] = itob(2); time[2] = itob(0x10);

	READTRACK();

	// skip head and sub, and go to the root directory record
	dir = (struct iso_directory_record*) &buf[12+156]; 

	mmssdd(dir->extent, (char*)time);

	READDIR(mdir);

	if (GetCdromFile(mdir, time, (char*)exename) == -1) return -1;

	READTRACK();

	memcpy(head, buf+12, sizeof(EXE_HEADER));
	size = head->t_size;
	addr = head->t_addr;

	while (size) {
		incTime();
		READTRACK();

		memcpy((u8*)(psxMemRLUT[(addr) >> 16] + ((addr) & 0xffff)), (char*)buf+12, 2048);

		size -= 2048;
		addr += 2048;
	}

	return 0;
}

int CheckCdrom() {
	struct iso_directory_record *dir;
	unsigned char time[4],*buf;
	unsigned char mdir[4096];
	char exename[256];
	int i, c;

	time[0] = itob(0); time[1] = itob(2); time[2] = itob(0x10);

	READTRACK();

	CdromLabel[32]=0;
	CdromId[9]=0;

	strncpy(CdromLabel, (char*)buf+52, 32);

	// skip head and sub, and go to the root directory record
	dir = (struct iso_directory_record*) &buf[12+156]; 

	mmssdd(dir->extent, (char*)time);

	READDIR(mdir);

	if (GetCdromFile(mdir, time, "SYSTEM.CNF;1") != -1) {
		READTRACK();

		sscanf((char*)buf+12, "BOOT = cdrom:\\%256s", exename);
		if (GetCdromFile(mdir, time, exename) == -1) {
			sscanf((char*)buf+12, "BOOT = cdrom:%256s", exename);
			if (GetCdromFile(mdir, time, exename) == -1) {
				char *ptr = strstr((char*)buf+12, "cdrom:");			// possibly the executable is in some subdir
				for (i=0; i<32; i++) {
					if (ptr[i] == ' ') continue;
					if (ptr[i] == '\\') continue;
				}
				if(ptr) {
					strncpy(exename, ptr, 256);
					if (GetCdromFile(mdir, time, exename) == -1)
				 	return -1;		// main executable not found
				}
			}
		}
	} else
		return -1;		// SYSTEM.CNF not found

	i = strlen(exename);
	if (i >= 2) {
		if (exename[i - 2] == ';') i-= 2;
		c = 8; i--;
		while (i >= 0 && c >= 0) {
			if (isalnum(exename[i])) CdromId[c--] = exename[i];
			i--;
		}
	}

	if (Config.PsxAuto) { // autodetect system (pal or ntsc)
		if (strstr(exename, "ES") != NULL)
			Config.PsxType = 1; // pal
		else Config.PsxType = 0; // ntsc
	}
	psxUpdateVSyncRate();
	if (CdromLabel[0] == ' ') {
		strncpy(CdromLabel, CdromId, 9);
	}
	SysPrintf("CD-ROM Label: %.32s\n", CdromLabel);
	SysPrintf("CD-ROM ID: %.9s\n", CdromId);
  for(i = 32; i>0; i--) {
    if(CdromLabel[i]==' ') {
      CdromLabel[i]=0;
    }
  }
  for(i = 9; i>0; i--) {
    if(CdromId[i]==' ') {
      CdromId[i]=0;
    }
  }
	return 0;
}

static int PSXGetFileType(FILE *f) {
    unsigned long current;
    u32 mybuf[2048];
    EXE_HEADER *exe_hdr;
    FILHDR *coff_hdr;

    current = ftell(f);
    fseek(f,0L,SEEK_SET);
    fread(mybuf,2048,1,f);
    fseek(f,current,SEEK_SET);

    exe_hdr = (EXE_HEADER *)mybuf;
    if (memcmp(exe_hdr->id,"PS-X EXE",8)==0)
        return PSX_EXE;

    if (mybuf[0]=='C' && mybuf[1]=='P' && mybuf[2]=='E')
        return CPE_EXE;

    coff_hdr = (FILHDR*)mybuf;
    if (coff_hdr->f_magic == 0x0162)
        return COFF_EXE;

    return INVALID_EXE;
}

/* TODO Error handling - return integer for each error case below, defined in an enum. Pass variable on return */
int Load(char *ExePath) {
	FILE *tmpFile;
	EXE_HEADER tmpHead;
	int type;
	int retval = 0;

	strncpy(CdromId, "SLUS99999", 9);
	strncpy(CdromLabel, "SLUS_999.99", 11);

    tmpFile = fopen(ExePath,"rb");
	if (tmpFile == NULL) {
		SysMessage(_("Error opening file: %s"), ExePath);
		retval = 0;
	} else {
		type = PSXGetFileType(tmpFile);
		switch (type) {
			case PSX_EXE:
				fread(&tmpHead,sizeof(EXE_HEADER),1,tmpFile);
				fseek(tmpFile, 0x800, SEEK_SET);		
				fread((void *)PSXM(SWAP32(tmpHead.t_addr)), SWAP32(tmpHead.t_size),1,tmpFile);
				fclose(tmpFile);
				psxRegs.pc = SWAP32(tmpHead.pc0);
				psxRegs.GPR.n.gp = SWAP32(tmpHead.gp0);
				psxRegs.GPR.n.sp = SWAP32(tmpHead.s_addr); 
				if (psxRegs.GPR.n.sp == 0)
					psxRegs.GPR.n.sp = 0x801fff00;
				retval = 0;
				break;
			case CPE_EXE:
				SysMessage(_("CPE files not supported."));
				retval = -1;
				break;
			case COFF_EXE:
				SysMessage(_("COFF files not supported."));
				retval = -1;
				break;
			case INVALID_EXE:
				SysMessage(_("This file does not appear to be a valid PSX file."));
				retval = -1;
				break;
		}
	}
	return retval;
}

// STATES

const char PcsxHeader[32] = "STv3 PCSX v";
char* statespath = "/wiisx/saves/";
static unsigned int savestates_slot = 0;

void savestates_select_slot(unsigned int s)
{
   if (s > 9) return;
   savestates_slot = s;
}

int SaveState() {
  gzFile f;
  GPUFreeze_t *gpufP;
	SPUFreeze_t *spufP;
	int Size;
	unsigned char *pMem;
	char *filename, buf[1024];
	
  /* fix the filename to %s.st%d format */
	filename = malloc(256);
#ifdef HW_RVL
  if(saveStateDevice==SAVESTATEDEVICE_USB)
    strcpy(filename,"usb:");
#endif
  if(saveStateDevice==SAVESTATEDEVICE_SD)
    strcpy(filename,"sd:"); //"sd:/" is any currently mounted SD on GC or Wii
	strcat(filename, statespath);
  strcat(filename, CdromId);
	strcat(filename, ".st");
	sprintf(buf, "%d", savestates_slot);
	strcat(filename, buf);

	f = gzopen(filename, "wb");
  free(filename);
   	
  if(!f)
  	return 0;

  pauseRemovalThread(); 
    
	gzwrite(f, (void*)PcsxHeader, 32);

	pMem = (unsigned char *) malloc(128*96*3);
	if (pMem == NULL) return -1;
	GPU_getScreenPic(pMem);
	gzwrite(f, pMem, 128*96*3);
	free(pMem);

	if (Config.HLE)
		psxBiosFreeze(1);

	gzwrite(f, psxM, 0x00200000);
	gzwrite(f, psxR, 0x00080000);
	gzwrite(f, psxH, 0x00010000);
	gzwrite(f, (void*)&psxRegs, sizeof(psxRegs));

	// gpu
	gpufP = (GPUFreeze_t *) malloc(sizeof(GPUFreeze_t));
	gpufP->ulFreezeVersion = 1;
	GPU_freeze(1, gpufP);
	gzwrite(f, gpufP, sizeof(GPUFreeze_t));
	free(gpufP);

	// spu
	spufP = (SPUFreeze_t *) malloc(16);
	SPU_freeze(2, spufP);
	Size = spufP->Size; gzwrite(f, &Size, 4);
	free(spufP);
	spufP = (SPUFreeze_t *) malloc(Size);
	SPU_freeze(1, spufP);
	gzwrite(f, spufP, Size);
	free(spufP);

	sioFreeze(f, 1);
	cdrFreeze(f, 1);
	psxHwFreeze(f, 1);
	psxRcntFreeze(f, 1);
	mdecFreeze(f, 1);

	gzclose(f);
	continueRemovalThread();

	return 1; //ok
}

int LoadState() {
	gzFile f;
	GPUFreeze_t *gpufP;
	SPUFreeze_t *spufP;
	int Size;
	char header[32];
	char *filename, buf[1024];
	
  /* fix the filename to %s.st%d format */
	filename = malloc(256);
#ifdef HW_RVL
  if(saveStateDevice==SAVESTATEDEVICE_USB)
    strcpy(filename,"usb:");
#endif
  if(saveStateDevice==SAVESTATEDEVICE_SD)
    strcpy(filename,"sd:"); //"sd:/" is any currently mounted SD on GC or Wii
	strcat(filename, statespath);
  strcat(filename, CdromId);
	strcat(filename, ".st");
	sprintf(buf, "%d", savestates_slot);
	strcat(filename, buf);

	f = gzopen(filename, "rb");
  free(filename);
   	
  if(!f)
  	return 0;

	pauseRemovalThread();
	
	psxCpu->Reset();

	gzread(f, header, 32);

	if (strncmp("STv3 PCSX", header, 9)) { gzclose(f); return -1; }

	gzseek(f, 128*96*3, SEEK_CUR);

	gzread(f, psxM, 0x00200000);
	gzread(f, psxR, 0x00080000);
	gzread(f, psxH, 0x00010000);
	gzread(f, (void*)&psxRegs, sizeof(psxRegs));

	if (Config.HLE)
		psxBiosFreeze(0);

	// gpu
	gpufP = (GPUFreeze_t *) malloc (sizeof(GPUFreeze_t));
	gzread(f, gpufP, sizeof(GPUFreeze_t));
	GPU_freeze(0, gpufP);
	free(gpufP);

	// spu
	gzread(f, &Size, 4);
	spufP = (SPUFreeze_t *) malloc (Size);
	gzread(f, spufP, Size);
	SPU_freeze(0, spufP);
	free(spufP);

	sioFreeze(f, 0);
	cdrFreeze(f, 0);
	psxHwFreeze(f, 0);
	psxRcntFreeze(f, 0);
	mdecFreeze(f, 0);

	gzclose(f);
  continueRemovalThread();
  
	return 1;
}

int CheckState(char *file) {
	gzFile f;
	char header[32];

	f = gzopen(file, "rb");
	if (f == NULL) return -1;

	psxCpu->Reset();

	gzread(f, header, 32);

	gzclose(f);

	if (strncmp("STv3 PCSX", header, 9)) return -1;

	return 0;
}

// NET Function Helpers

int SendPcsxInfo() {
	if (NET_recvData == NULL || NET_sendData == NULL)
		return 0;

//	SysPrintf("SendPcsxInfo\n");

	NET_sendData(&Config.Xa, sizeof(Config.Xa), PSE_NET_BLOCKING);
	NET_sendData(&Config.Sio, sizeof(Config.Sio), PSE_NET_BLOCKING);
	NET_sendData(&Config.SpuIrq, sizeof(Config.SpuIrq), PSE_NET_BLOCKING);
	NET_sendData(&Config.RCntFix, sizeof(Config.RCntFix), PSE_NET_BLOCKING);
	NET_sendData(&Config.PsxType, sizeof(Config.PsxType), PSE_NET_BLOCKING);
	NET_sendData(&Config.Cpu, sizeof(Config.Cpu), PSE_NET_BLOCKING);

//	SysPrintf("Send OK\n");

	return 0;
}

int RecvPcsxInfo() {
	int tmp;

	if (NET_recvData == NULL || NET_sendData == NULL)
		return 0;

//	SysPrintf("RecvPcsxInfo\n");

	NET_recvData(&Config.Xa, sizeof(Config.Xa), PSE_NET_BLOCKING);
	NET_recvData(&Config.Sio, sizeof(Config.Sio), PSE_NET_BLOCKING);
	NET_recvData(&Config.SpuIrq, sizeof(Config.SpuIrq), PSE_NET_BLOCKING);
	NET_recvData(&Config.RCntFix, sizeof(Config.RCntFix), PSE_NET_BLOCKING);
	NET_recvData(&Config.PsxType, sizeof(Config.PsxType), PSE_NET_BLOCKING);
	psxUpdateVSyncRate();

	SysUpdate();

	tmp = Config.Cpu;
	NET_recvData(&Config.Cpu, sizeof(Config.Cpu), PSE_NET_BLOCKING);
	if (tmp != Config.Cpu) {
		psxCpu->Shutdown();
#ifdef PSXREC
		if (Config.Cpu)	
			 psxCpu = &psxInt;
		else psxCpu = &psxRec;
#else
		psxCpu = &psxInt;
#endif
		if (psxCpu->Init() == -1) {
			SysClose(); return -1;
		}
		psxCpu->Reset();
	}

//	SysPrintf("Recv OK\n");

	return 0;
}


void __Log(char *fmt, ...) {
	va_list list;
#ifdef LOG_STDOUT
	char tmp[1024];
#endif

	va_start(list, fmt);
#ifndef LOG_STDOUT
#if defined (CPU_LOG) || defined(DMA_LOG) || defined(CDR_LOG) || defined(HW_LOG) || \
	defined(BIOS_LOG) || defined(GTE_LOG) || defined(PAD_LOG)
	vfprintf(emuLog, fmt, list);
#endif
#else
	vsprintf(tmp, fmt, list);
	SysPrintf(tmp);
#endif
	va_end(list);
}

