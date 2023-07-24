/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

/*
* Miscellaneous functions, including savestates and CD-ROM loading.
*/

#include <stddef.h>
#include <assert.h>
#include "misc.h"
#include "cdrom.h"
#include "mdec.h"
#include "gpu.h"
#include "ppf.h"
#include "database.h"
#include <zlib.h>

char CdromId[10] = "";
char CdromLabel[33] = "";

// PSX Executable types
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

static void mmssdd( char *b, char *p )
{
	int m, s, d;
	unsigned char *ub = (void *)b;
	int block = (ub[3] << 24) | (ub[2] << 16) | (ub[1] << 8) | ub[0];

	block += 150;
	m = block / 4500;			// minutes
	block = block - m * 4500;	// minutes rest
	s = block / 75;				// seconds
	d = block - s * 75;			// seconds rest

	m = ((m / 10) << 4) | m % 10;
	s = ((s / 10) << 4) | s % 10;
	d = ((d / 10) << 4) | d % 10;

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
	if (!CDR_readTrack(time)) return -1; \
	buf = (void *)CDR_getBuffer(); \
	if (buf == NULL) return -1; \
	else CheckPPFCache((u8 *)buf, time[0], time[1], time[2]);

#define READDIR(_dir) \
	READTRACK(); \
	memcpy(_dir, buf + 12, 2048); \
 \
	incTime(); \
	READTRACK(); \
	memcpy(_dir + 2048, buf + 12, 2048);

int GetCdromFile(u8 *mdir, u8 *time, char *filename) {
	struct iso_directory_record *dir;
	int retval = -1;
	u8 ddir[4096];
	u8 *buf;
	int i;

	// only try to scan if a filename is given
	if (!strlen(filename)) return -1;

	i = 0;
	while (i < 4096) {
		dir = (struct iso_directory_record*) &mdir[i];
		if (dir->length[0] == 0) {
			return -1;
		}
		i += (u8)dir->length[0];

		if (dir->flags[0] & 0x2) { // it's a dir
			if (!strnicmp((char *)&dir->name[0], filename, dir->name_len[0])) {
				if (filename[dir->name_len[0]] != '\\') continue;

				filename += dir->name_len[0] + 1;

				mmssdd(dir->extent, (char *)time);
				READDIR(ddir);
				i = 0;
				mdir = ddir;
			}
		} else {
			if (!strnicmp((char *)&dir->name[0], filename, strlen(filename))) {
				mmssdd(dir->extent, (char *)time);
				retval = 0;
				break;
			}
		}
	}
	return retval;
}

static const unsigned int gpu_ctl_def[] = {
	0x00000000, 0x01000000, 0x03000000, 0x04000000,
	0x05000800, 0x06c60260, 0x0703fc10, 0x08000027,
};

static const unsigned int gpu_data_def[] = {
	0xe100360b, 0xe2000000, 0xe3000800, 0xe4077e7f,
	0xe5001000, 0xe6000000,
	0x02000000, 0x00000000, 0x01ff03ff,
};

void BiosLikeGPUSetup()
{
	int i;

	for (i = 0; i < sizeof(gpu_ctl_def) / sizeof(gpu_ctl_def[0]); i++)
		GPU_writeStatus(gpu_ctl_def[i]);

	for (i = 0; i < sizeof(gpu_data_def) / sizeof(gpu_data_def[0]); i++)
		GPU_writeData(gpu_data_def[i]);

	HW_GPU_STATUS |= SWAP32(PSXGPU_nBUSY);
}

static void SetBootRegs(u32 pc, u32 gp, u32 sp)
{
	//printf("%s %08x %08x %08x\n", __func__, pc, gp, sp);
	psxCpu->Notify(R3000ACPU_NOTIFY_BEFORE_SAVE, NULL);

	psxRegs.pc = pc;
	psxRegs.GPR.n.gp = gp;
	psxRegs.GPR.n.sp = sp ? sp : 0x801fff00;

	psxCpu->Notify(R3000ACPU_NOTIFY_AFTER_LOAD, NULL);
}

void BiosBootBypass() {
	assert(psxRegs.pc == 0x80030000);

	// skip BIOS logos and region check
	psxCpu->Notify(R3000ACPU_NOTIFY_BEFORE_SAVE, NULL);
	psxRegs.pc = psxRegs.GPR.n.ra;
}

int LoadCdrom() {
	EXE_HEADER tmpHead;
	struct iso_directory_record *dir;
	u8 time[4], *buf;
	u8 mdir[4096];
	char exename[256];

	if (!Config.HLE) {
		if (psxRegs.pc != 0x80030000) // BiosBootBypass'ed or custom BIOS?
			return 0;
		if (Config.SlowBoot)
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
		strcpy(exename, "PSX.EXE;1");

		READTRACK();
	}
	else {
		// read the SYSTEM.CNF
		READTRACK();

		sscanf((char *)buf + 12, "BOOT = cdrom:\\%255s", exename);
		if (GetCdromFile(mdir, time, exename) == -1) {
			sscanf((char *)buf + 12, "BOOT = cdrom:%255s", exename);
			if (GetCdromFile(mdir, time, exename) == -1) {
				char *ptr = strstr((char *)buf + 12, "cdrom:");
				if (ptr != NULL) {
					ptr += 6;
					while (*ptr == '\\' || *ptr == '/') ptr++;
					strncpy(exename, ptr, 255);
					exename[255] = '\0';
					ptr = exename;
					while (*ptr != '\0' && *ptr != '\r' && *ptr != '\n') ptr++;
					*ptr = '\0';
					if (GetCdromFile(mdir, time, exename) == -1)
						return -1;
				} else
					return -1;
			}
		}

		// Read the EXE-Header
		READTRACK();
	}

	memcpy(&tmpHead, buf + 12, sizeof(EXE_HEADER));

	SysPrintf("manual booting '%s'\n", exename);
	SetBootRegs(SWAP32(tmpHead.pc0), SWAP32(tmpHead.gp0), SWAP32(tmpHead.s_addr));

	tmpHead.t_size = SWAP32(tmpHead.t_size);
	tmpHead.t_addr = SWAP32(tmpHead.t_addr);

	psxCpu->Clear(tmpHead.t_addr, tmpHead.t_size / 4);
	psxCpu->Reset();

	// Read the rest of the main executable
	while (tmpHead.t_size & ~2047) {
		void *ptr = (void *)PSXM(tmpHead.t_addr);

		incTime();
		READTRACK();

		if (ptr != INVALID_PTR) memcpy(ptr, buf+12, 2048);

		tmpHead.t_size -= 2048;
		tmpHead.t_addr += 2048;
	}

	return 0;
}

int LoadCdromFile(const char *filename, EXE_HEADER *head) {
	struct iso_directory_record *dir;
	u8 time[4],*buf;
	u8 mdir[4096];
	char exename[256];
	u32 size, addr;
	void *mem;

	sscanf(filename, "cdrom:\\%255s", exename);

	time[0] = itob(0); time[1] = itob(2); time[2] = itob(0x10);

	READTRACK();

	// skip head and sub, and go to the root directory record
	dir = (struct iso_directory_record *)&buf[12 + 156];

	mmssdd(dir->extent, (char*)time);

	READDIR(mdir);

	if (GetCdromFile(mdir, time, exename) == -1) return -1;

	READTRACK();

	memcpy(head, buf + 12, sizeof(EXE_HEADER));
	size = head->t_size;
	addr = head->t_addr;

	psxCpu->Clear(addr, size / 4);
	psxCpu->Reset();

	while (size & ~2047) {
		incTime();
		READTRACK();

		mem = PSXM(addr);
		if (mem != INVALID_PTR)
			memcpy(mem, buf + 12, 2048);

		size -= 2048;
		addr += 2048;
	}

	return 0;
}

int CheckCdrom() {
	struct iso_directory_record *dir;
	unsigned char time[4];
	char *buf;
	unsigned char mdir[4096];
	char exename[256];
	int i, len, c;

	FreePPFCache();

	time[0] = itob(0);
	time[1] = itob(2);
	time[2] = itob(0x10);

	READTRACK();

	memset(CdromLabel, 0, sizeof(CdromLabel));
	memset(CdromId, 0, sizeof(CdromId));
	memset(exename, 0, sizeof(exename));

	strncpy(CdromLabel, buf + 52, 32);

	// skip head and sub, and go to the root directory record
	dir = (struct iso_directory_record *)&buf[12 + 156];

	mmssdd(dir->extent, (char *)time);

	READDIR(mdir);

	if (GetCdromFile(mdir, time, "SYSTEM.CNF;1") != -1) {
		READTRACK();

		sscanf(buf + 12, "BOOT = cdrom:\\%255s", exename);
		if (GetCdromFile(mdir, time, exename) == -1) {
			sscanf(buf + 12, "BOOT = cdrom:%255s", exename);
			if (GetCdromFile(mdir, time, exename) == -1) {
				char *ptr = strstr(buf + 12, "cdrom:");			// possibly the executable is in some subdir
				if (ptr != NULL) {
					ptr += 6;
					while (*ptr == '\\' || *ptr == '/') ptr++;
					strncpy(exename, ptr, 255);
					exename[255] = '\0';
					ptr = exename;
					while (*ptr != '\0' && *ptr != '\r' && *ptr != '\n') ptr++;
					*ptr = '\0';
					if (GetCdromFile(mdir, time, exename) == -1)
					 	return -1;		// main executable not found
				} else
					return -1;
			}
		}
		/* Workaround for Wild Arms EU/US which has non-standard string causing incorrect region detection */
		if (exename[0] == 'E' && exename[1] == 'X' && exename[2] == 'E' && exename[3] == '\\') {
			size_t offset = 4;
			size_t i, len = strlen(exename) - offset;
			for (i = 0; i < len; i++)
				exename[i] = exename[i + offset];
			exename[i] = '\0';
		}
	} else if (GetCdromFile(mdir, time, "PSX.EXE;1") != -1) {
		strcpy(exename, "PSX.EXE;1");
		strcpy(CdromId, "SLUS99999");
	} else
		return -1;		// SYSTEM.CNF and PSX.EXE not found

	if (CdromId[0] == '\0') {
		len = strlen(exename);
		c = 0;
		for (i = 0; i < len; ++i) {
			if (exename[i] == ';' || c >= sizeof(CdromId) - 1)
				break;
			if (isalnum(exename[i]))
				CdromId[c++] = exename[i];
		}
	}

	if (CdromId[0] == '\0')
		strcpy(CdromId, "SLUS99999");

	if (Config.PsxAuto) { // autodetect system (pal or ntsc)
		if (
			/* Make sure Wild Arms SCUS-94608 is not detected as a PAL game. */
			((CdromId[0] == 's' || CdromId[0] == 'S') && (CdromId[2] == 'e' || CdromId[2] == 'E')) ||
			!strncmp(CdromId, "DTLS3035", 8) ||
			!strncmp(CdromId, "PBPX95001", 9) || // according to redump.org, these PAL
			!strncmp(CdromId, "PBPX95007", 9) || // discs have a non-standard ID;
			!strncmp(CdromId, "PBPX95008", 9))   // add more serials if they are discovered.
			Config.PsxType = PSX_TYPE_PAL; // pal
		else Config.PsxType = PSX_TYPE_NTSC; // ntsc
	}

	if (CdromLabel[0] == ' ') {
		strncpy(CdromLabel, CdromId, 9);
	}
	SysPrintf(_("CD-ROM Label: %.32s\n"), CdromLabel);
	SysPrintf(_("CD-ROM ID: %.9s\n"), CdromId);
	SysPrintf(_("CD-ROM EXE Name: %.255s\n"), exename);
	
	Apply_Hacks_Cdrom();

	BuildPPFCache();

	return 0;
}

static int PSXGetFileType(FILE *f) {
	unsigned long current;
	u8 mybuf[2048];
	EXE_HEADER *exe_hdr;
	FILHDR *coff_hdr;

	current = ftell(f);
	fseek(f, 0L, SEEK_SET);
	if (fread(&mybuf, 1, sizeof(mybuf), f) != sizeof(mybuf))
		goto io_fail;
	
	fseek(f, current, SEEK_SET);

	exe_hdr = (EXE_HEADER *)mybuf;
	if (memcmp(exe_hdr->id, "PS-X EXE", 8) == 0)
		return PSX_EXE;

	if (mybuf[0] == 'C' && mybuf[1] == 'P' && mybuf[2] == 'E')
		return CPE_EXE;

	coff_hdr = (FILHDR *)mybuf;
	if (SWAPu16(coff_hdr->f_magic) == 0x0162)
		return COFF_EXE;

	return INVALID_EXE;

io_fail:
#ifndef NDEBUG
	SysPrintf(_("File IO error in <%s:%s>.\n"), __FILE__, __func__);
#endif
	return INVALID_EXE;
}

// temporary pandora workaround..
// FIXME: remove
size_t fread_to_ram(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	void *tmp;
	size_t ret = 0;

	tmp = malloc(size * nmemb);
	if (tmp) {
		ret = fread(tmp, size, nmemb, stream);
		memcpy(ptr, tmp, size * nmemb);
		free(tmp);
	}
	return ret;
}

int Load(const char *ExePath) {
	FILE *tmpFile;
	EXE_HEADER tmpHead;
	int type;
	int retval = 0;
	u8 opcode;
	u32 section_address, section_size;
	void *mem;

	strcpy(CdromId, "SLUS99999");
	strcpy(CdromLabel, "SLUS_999.99");

	tmpFile = fopen(ExePath, "rb");
	if (tmpFile == NULL) {
		SysPrintf(_("Error opening file: %s.\n"), ExePath);
		retval = -1;
	} else {
		type = PSXGetFileType(tmpFile);
		switch (type) {
			case PSX_EXE:
				if (fread(&tmpHead, 1, sizeof(EXE_HEADER), tmpFile) != sizeof(EXE_HEADER))
					goto fail_io;
				section_address = SWAP32(tmpHead.t_addr);
				section_size = SWAP32(tmpHead.t_size);
				mem = PSXM(section_address);
				if (mem != INVALID_PTR) {
					fseek(tmpFile, 0x800, SEEK_SET);
					fread_to_ram(mem, section_size, 1, tmpFile);
					psxCpu->Clear(section_address, section_size / 4);
				}
				SetBootRegs(SWAP32(tmpHead.pc0), SWAP32(tmpHead.gp0),
					SWAP32(tmpHead.s_addr));
				retval = 0;
				break;
			case CPE_EXE:
				fseek(tmpFile, 6, SEEK_SET); /* Something tells me we should go to 4 and read the "08 00" here... */
				do {
					if (fread(&opcode, 1, sizeof(opcode), tmpFile) != sizeof(opcode))
						goto fail_io;
					switch (opcode) {
						case 1: /* Section loading */
							if (fread(&section_address, 1, sizeof(section_address), tmpFile) != sizeof(section_address))
								goto fail_io;
							if (fread(&section_size, 1, sizeof(section_size), tmpFile) != sizeof(section_size))
								goto fail_io;
							section_address = SWAPu32(section_address);
							section_size = SWAPu32(section_size);
#ifdef EMU_LOG
							EMU_LOG("Loading %08X bytes from %08X to %08X\n", section_size, ftell(tmpFile), section_address);
#endif
							mem = PSXM(section_address);
							if (mem != INVALID_PTR) {
								fread_to_ram(mem, section_size, 1, tmpFile);
								psxCpu->Clear(section_address, section_size / 4);
							}
							break;
						case 3: /* register loading (PC only?) */
							fseek(tmpFile, 2, SEEK_CUR); /* unknown field */
							if (fread(&psxRegs.pc, 1, sizeof(psxRegs.pc), tmpFile) != sizeof(psxRegs.pc))
								goto fail_io;
							psxRegs.pc = SWAPu32(psxRegs.pc);
							break;
						case 0: /* End of file */
							break;
						default:
							SysPrintf(_("Unknown CPE opcode %02x at position %08x.\n"), opcode, ftell(tmpFile) - 1);
							retval = -1;
							break;
					}
				} while (opcode != 0 && retval == 0);
				break;
			case COFF_EXE:
				SysPrintf(_("COFF files not supported.\n"));
				retval = -1;
				break;
			case INVALID_EXE:
				SysPrintf(_("This file does not appear to be a valid PSX EXE file.\n"));
				SysPrintf(_("(did you forget -cdfile ?)\n"));
				retval = -1;
				break;
		}
	}

	if (retval != 0) {
		CdromId[0] = '\0';
		CdromLabel[0] = '\0';
	}

	if (tmpFile)
		fclose(tmpFile);
	return retval;

fail_io:
#ifndef NDEBUG
	SysPrintf(_("File IO error in <%s:%s>.\n"), __FILE__, __func__);
#endif
	fclose(tmpFile);
	return -1;
}

// STATES

static void *zlib_open(const char *name, const char *mode)
{
	return gzopen(name, mode);
}

static int zlib_read(void *file, void *buf, u32 len)
{
	return gzread(file, buf, len);
}

static int zlib_write(void *file, const void *buf, u32 len)
{
	return gzwrite(file, buf, len);
}

static long zlib_seek(void *file, long offs, int whence)
{
	return gzseek(file, offs, whence);
}

static void zlib_close(void *file)
{
	gzclose(file);
}

struct PcsxSaveFuncs SaveFuncs = {
	zlib_open, zlib_read, zlib_write, zlib_seek, zlib_close
};

static const char PcsxHeader[32] = "STv4 PCSX v" PCSX_VERSION;

// Savestate Versioning!
// If you make changes to the savestate version, please increment the value below.
static const u32 SaveVersion = 0x8b410006;

int SaveState(const char *file) {
	void *f;
	GPUFreeze_t *gpufP;
	SPUFreezeHdr_t *spufH;
	SPUFreeze_t *spufP;
	int Size;
	unsigned char *pMem;

	f = SaveFuncs.open(file, "wb");
	if (f == NULL) return -1;

	psxCpu->Notify(R3000ACPU_NOTIFY_BEFORE_SAVE, NULL);

	SaveFuncs.write(f, (void *)PcsxHeader, 32);
	SaveFuncs.write(f, (void *)&SaveVersion, sizeof(u32));
	SaveFuncs.write(f, (void *)&Config.HLE, sizeof(boolean));

	pMem = (unsigned char *)malloc(128 * 96 * 3);
	if (pMem == NULL) return -1;
	GPU_getScreenPic(pMem);
	SaveFuncs.write(f, pMem, 128 * 96 * 3);
	free(pMem);

	if (Config.HLE)
		psxBiosFreeze(1);

	SaveFuncs.write(f, psxM, 0x00200000);
	SaveFuncs.write(f, psxR, 0x00080000);
	SaveFuncs.write(f, psxH, 0x00010000);
	// only partial save of psxRegisters to maintain savestate compat
	SaveFuncs.write(f, &psxRegs, offsetof(psxRegisters, gteBusyCycle));

	// gpu
	gpufP = (GPUFreeze_t *)malloc(sizeof(GPUFreeze_t));
	gpufP->ulFreezeVersion = 1;
	GPU_freeze(1, gpufP);
	SaveFuncs.write(f, gpufP, sizeof(GPUFreeze_t));
	free(gpufP);

	// spu
	spufH = malloc(sizeof(*spufH));
	SPU_freeze(2, (SPUFreeze_t *)spufH, psxRegs.cycle);
	Size = spufH->Size; SaveFuncs.write(f, &Size, 4);
	free(spufH);
	spufP = (SPUFreeze_t *) malloc(Size);
	SPU_freeze(1, spufP, psxRegs.cycle);
	SaveFuncs.write(f, spufP, Size);
	free(spufP);

	sioFreeze(f, 1);
	cdrFreeze(f, 1);
	psxHwFreeze(f, 1);
	psxRcntFreeze(f, 1);
	mdecFreeze(f, 1);
	new_dyna_freeze(f, 1);

	SaveFuncs.close(f);

	return 0;
}

int LoadState(const char *file) {
	void *f;
	GPUFreeze_t *gpufP;
	SPUFreeze_t *spufP;
	int Size;
	char header[32];
	u32 version;
	boolean hle;

	f = SaveFuncs.open(file, "rb");
	if (f == NULL) return -1;

	SaveFuncs.read(f, header, sizeof(header));
	SaveFuncs.read(f, &version, sizeof(u32));
	SaveFuncs.read(f, &hle, sizeof(boolean));

	if (strncmp("STv4 PCSX", header, 9) != 0 || version != SaveVersion) {
		SaveFuncs.close(f);
		return -1;
	}
	Config.HLE = hle;

	if (Config.HLE)
		psxBiosInit();

	SaveFuncs.seek(f, 128 * 96 * 3, SEEK_CUR);
	SaveFuncs.read(f, psxM, 0x00200000);
	SaveFuncs.read(f, psxR, 0x00080000);
	SaveFuncs.read(f, psxH, 0x00010000);
	SaveFuncs.read(f, &psxRegs, offsetof(psxRegisters, gteBusyCycle));
	psxRegs.gteBusyCycle = psxRegs.cycle;

	psxCpu->Notify(R3000ACPU_NOTIFY_AFTER_LOAD, NULL);

	if (Config.HLE)
		psxBiosFreeze(0);

	// gpu
	gpufP = (GPUFreeze_t *)malloc(sizeof(GPUFreeze_t));
	SaveFuncs.read(f, gpufP, sizeof(GPUFreeze_t));
	GPU_freeze(0, gpufP);
	free(gpufP);
	if (HW_GPU_STATUS == 0)
		HW_GPU_STATUS = SWAP32(GPU_readStatus());

	// spu
	SaveFuncs.read(f, &Size, 4);
	spufP = (SPUFreeze_t *)malloc(Size);
	SaveFuncs.read(f, spufP, Size);
	SPU_freeze(0, spufP, psxRegs.cycle);
	free(spufP);

	sioFreeze(f, 0);
	cdrFreeze(f, 0);
	psxHwFreeze(f, 0);
	psxRcntFreeze(f, 0);
	mdecFreeze(f, 0);
	new_dyna_freeze(f, 0);

	SaveFuncs.close(f);

	return 0;
}

int CheckState(const char *file) {
	void *f;
	char header[32];
	u32 version;
	boolean hle;

	f = SaveFuncs.open(file, "rb");
	if (f == NULL) return -1;

	SaveFuncs.read(f, header, sizeof(header));
	SaveFuncs.read(f, &version, sizeof(u32));
	SaveFuncs.read(f, &hle, sizeof(boolean));

	SaveFuncs.close(f);

	if (strncmp("STv4 PCSX", header, 9) != 0 || version != SaveVersion)
		return -1;

	return 0;
}

// NET Function Helpers

int SendPcsxInfo() {
	if (NET_recvData == NULL || NET_sendData == NULL)
		return 0;

	boolean Sio_old = 0;
	boolean SpuIrq_old = 0;
	boolean RCntFix_old = 0;
	NET_sendData(&Config.Xa, sizeof(Config.Xa), PSE_NET_BLOCKING);
	NET_sendData(&Sio_old, sizeof(Sio_old), PSE_NET_BLOCKING);
	NET_sendData(&SpuIrq_old, sizeof(SpuIrq_old), PSE_NET_BLOCKING);
	NET_sendData(&RCntFix_old, sizeof(RCntFix_old), PSE_NET_BLOCKING);
	NET_sendData(&Config.PsxType, sizeof(Config.PsxType), PSE_NET_BLOCKING);
	NET_sendData(&Config.Cpu, sizeof(Config.Cpu), PSE_NET_BLOCKING);

	return 0;
}

int RecvPcsxInfo() {
	int tmp;

	if (NET_recvData == NULL || NET_sendData == NULL)
		return 0;

	boolean Sio_old = 0;
	boolean SpuIrq_old = 0;
	boolean RCntFix_old = 0;
	NET_recvData(&Config.Xa, sizeof(Config.Xa), PSE_NET_BLOCKING);
	NET_recvData(&Sio_old, sizeof(Sio_old), PSE_NET_BLOCKING);
	NET_recvData(&SpuIrq_old, sizeof(SpuIrq_old), PSE_NET_BLOCKING);
	NET_recvData(&RCntFix_old, sizeof(RCntFix_old), PSE_NET_BLOCKING);
	NET_recvData(&Config.PsxType, sizeof(Config.PsxType), PSE_NET_BLOCKING);

	SysUpdate();

	tmp = Config.Cpu;
	NET_recvData(&Config.Cpu, sizeof(Config.Cpu), PSE_NET_BLOCKING);
	if (tmp != Config.Cpu) {
		psxCpu->Shutdown();
#ifndef DRC_DISABLE
		if (Config.Cpu == CPU_INTERPRETER) psxCpu = &psxInt;
		else psxCpu = &psxRec;
#else
		psxCpu = &psxInt;
#endif
		if (psxCpu->Init() == -1) {
			SysClose(); return -1;
		}
		psxCpu->Reset();
		psxCpu->Notify(R3000ACPU_NOTIFY_AFTER_LOAD, NULL);
	}

	return 0;
}

// remove the leading and trailing spaces in a string
void trim(char *str) {
	int pos = 0;
	char *dest = str;

	// skip leading blanks
	while (str[pos] <= ' ' && str[pos] > 0)
		pos++;

	while (str[pos]) {
		*(dest++) = str[pos];
		pos++;
	}

	*(dest--) = '\0'; // store the null

	// remove trailing blanks
	while (dest >= str && *dest <= ' ' && *dest > 0)
		*(dest--) = '\0';
}

// lookup table for crc calculation
static unsigned short crctab[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108,
	0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF, 0x1231, 0x0210,
	0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 0x9339, 0x8318, 0xB37B,
	0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE, 0x2462, 0x3443, 0x0420, 0x1401,
	0x64E6, 0x74C7, 0x44A4, 0x5485, 0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE,
	0xF5CF, 0xC5AC, 0xD58D, 0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6,
	0x5695, 0x46B4, 0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D,
	0xC7BC, 0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B, 0x5AF5,
	0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 0xDBFD, 0xCBDC,
	0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A, 0x6CA6, 0x7C87, 0x4CE4,
	0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD,
	0xAD2A, 0xBD0B, 0x8D68, 0x9D49, 0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13,
	0x2E32, 0x1E51, 0x0E70, 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A,
	0x9F59, 0x8F78, 0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E,
	0xE16F, 0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 0x02B1,
	0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256, 0xB5EA, 0xA5CB,
	0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 0x34E2, 0x24C3, 0x14A0,
	0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xA7DB, 0xB7FA, 0x8799, 0x97B8,
	0xE75F, 0xF77E, 0xC71D, 0xD73C, 0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657,
	0x7676, 0x4615, 0x5634, 0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9,
	0xB98A, 0xA9AB, 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882,
	0x28A3, 0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92, 0xFD2E,
	0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 0x7C26, 0x6C07,
	0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1, 0xEF1F, 0xFF3E, 0xCF5D,
	0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 0x6E17, 0x7E36, 0x4E55, 0x5E74,
	0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

u16 calcCrc(u8 *d, int len) {
	u16 crc = 0;
	int i;

	for (i = 0; i < len; i++) {
		crc = crctab[(crc >> 8) ^ d[i]] ^ (crc << 8);
	}

	return ~crc;
}
