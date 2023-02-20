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
* PSX memory functions.
*/

// TODO: Implement caches & cycle penalty.

#include "PsxMem.h"
#include "psxmem_map.h"
#include "R3000A.h"
#include "PsxHw.h"
//#include "debug.h"
#define DebugCheckBP(...)

//#include "lightrec/mem.h"
//#include "memmap.h"

#ifdef USE_LIBRETRO_VFS
#include <streams/file_stream_transforms.h>
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#include "Gamecube/fileBrowser/fileBrowser.h"
#include "Gamecube/fileBrowser/fileBrowser-libfat.h"
#include "Gamecube/fileBrowser/fileBrowser-CARD.h"
#include "Gamecube/wiiSXconfig.h"

static s8 psxM_buf[0x220000] __attribute__((aligned(4096)));
static s8 psxR_buf[0x80000] __attribute__((aligned(4096)));

s8 *psxM = psxM_buf; // Kernel & User Memory (2 Meg)
s8 *psxR = psxR_buf; // BIOS ROM (512K)
s8 *psxP = NULL; // Parallel Port (64K)
s8 *psxH = NULL; // Scratch Pad (1K) & Hardware Registers (8K)

#if 0
void *(*psxMapHook)(unsigned long addr, size_t size, int is_fixed,
		enum psxMapTag tag);
void (*psxUnmapHook)(void *ptr, size_t size, enum psxMapTag tag);

void *psxMap(unsigned long addr, size_t size, int is_fixed,
		enum psxMapTag tag)
{
	int flags = MAP_PRIVATE | MAP_ANONYMOUS;
	int try_ = 0;
	unsigned long mask;
	void *req, *ret;

retry:
	if (psxMapHook != NULL) {
		ret = psxMapHook(addr, size, 0, tag);
		if (ret == NULL)
			return MAP_FAILED;
	}
	else {
		/* avoid MAP_FIXED, it overrides existing mappings.. */
		/* if (is_fixed)
			flags |= MAP_FIXED; */

		req = (void *)(uintptr_t)addr;
		ret = mmap(req, size, PROT_READ | PROT_WRITE, flags, -1, 0);
		if (ret == MAP_FAILED)
			return ret;
	}

	if (addr != 0 && ret != (void *)(uintptr_t)addr) {
		SysMessage("psxMap: warning: wanted to map @%08x, got %p\n",
			addr, ret);

		if (is_fixed) {
			psxUnmap(ret, size, tag);
			return MAP_FAILED;
		}

		if (((addr ^ (unsigned long)(uintptr_t)ret) & ~0xff000000l) && try_ < 2)
		{
			psxUnmap(ret, size, tag);

			// try to use similarly aligned memory instead
			// (recompiler needs this)
			mask = try_ ? 0xffff : 0xffffff;
			addr = ((uintptr_t)ret + mask) & ~mask;
			try_++;
			goto retry;
		}
	}

	return ret;
}

void psxUnmap(void *ptr, size_t size, enum psxMapTag tag)
{
	if (psxUnmapHook != NULL) {
		psxUnmapHook(ptr, size, tag);
		return;
	}

	if (ptr)
		munmap(ptr, size);
}


/*  Playstation Memory Map (from Playstation doc by Joshua Walker)
0x0000_0000-0x0000_ffff		Kernel (64K)
0x0001_0000-0x001f_ffff		User Memory (1.9 Meg)

0x1f00_0000-0x1f00_ffff		Parallel Port (64K)

0x1f80_0000-0x1f80_03ff		Scratch Pad (1024 bytes)

0x1f80_1000-0x1f80_2fff		Hardware Registers (8K)

0x1fc0_0000-0x1fc7_ffff		BIOS (512K)

0x8000_0000-0x801f_ffff		Kernel and User Memory Mirror (2 Meg) Cached
0x9fc0_0000-0x9fc7_ffff		BIOS Mirror (512K) Cached

0xa000_0000-0xa01f_ffff		Kernel and User Memory Mirror (2 Meg) Uncached
0xbfc0_0000-0xbfc7_ffff		BIOS Mirror (512K) Uncached
*/

static int psxMemInitMap(void)
{
	psxM = psxMap(0x80000000, 0x00210000, 1, MAP_TAG_RAM);
	if (psxM == MAP_FAILED)
		psxM = psxMap(0x77000000, 0x00210000, 0, MAP_TAG_RAM);
	if (psxM == MAP_FAILED) {
		SysMessage(_("mapping main RAM failed"));
		psxM = NULL;
		return -1;
	}
	psxP = &psxM[0x200000];

	psxH = psxMap(0x1f800000, 0x10000, 0, MAP_TAG_OTHER);
	if (psxH == MAP_FAILED) {
		SysMessage(_("Error allocating memory!"));
		psxMemShutdown();
		return -1;
	}

	psxR = psxMap(0x1fc00000, 0x80000, 0, MAP_TAG_OTHER);
	if (psxR == MAP_FAILED) {
		SysMessage(_("Error allocating memory!"));
		psxMemShutdown();
		return -1;
	}

	return 0;
}

static void psxMemFreeMap(void)
{
	if (psxM) psxUnmap(psxM, 0x00210000, MAP_TAG_RAM);
	if (psxH) psxUnmap(psxH, 0x10000, MAP_TAG_OTHER);
	if (psxR) psxUnmap(psxR, 0x80000, MAP_TAG_OTHER);
	psxM = psxH = psxR = NULL;
	psxP = NULL;
}
#endif

int psxMemInit(void)
{
	unsigned int i;
	int ret;

	/*if (LIGHTREC_CUSTOM_MAP)
		ret = lightrec_init_mmap();
	else
		ret = psxMemInitMap();
	if (ret) {
		SysMessage(_("Error allocating memory!"));
		psxMemShutdown();
		return -1;
	}

	psxMemRLUT = (u8 **)malloc(0x10000 * sizeof(void *));
	psxMemWLUT = (u8 **)malloc(0x10000 * sizeof(void *));

	if (psxMemRLUT == NULL || psxMemWLUT == NULL) {
		SysMessage(_("Error allocating memory!"));
		psxMemShutdown();
		return -1;
	}*/
	psxP = &psxM[0x200000];
	psxH = &psxM[0x210000];

	memset(psxCore.psxMemRLUT, (uintptr_t)INVALID_PTR, 0x10000 * sizeof(void *));
	memset(psxCore.psxMemWLUT, (uintptr_t)INVALID_PTR, 0x10000 * sizeof(void *));

// MemR
	for (i = 0; i < 0x80; i++) psxCore.psxMemRLUT[i + 0x0000] = (u8 *)&psxM[(i & 0x1f) << 16];

	memcpy(psxCore.psxMemRLUT + 0x8000, psxCore.psxMemRLUT, 0x80 * sizeof(void *));
	memcpy(psxCore.psxMemRLUT + 0xa000, psxCore.psxMemRLUT, 0x80 * sizeof(void *));

	psxCore.psxMemRLUT[0x1f00] = (u8 *)psxP;
	psxCore.psxMemRLUT[0x1f80] = (u8 *)psxH;

	for (i = 0; i < 0x08; i++) psxCore.psxMemRLUT[i + 0x1fc0] = (u8 *)&psxR[i << 16];

	memcpy(psxCore.psxMemRLUT + 0x9fc0, psxCore.psxMemRLUT + 0x1fc0, 0x08 * sizeof(void *));
	memcpy(psxCore.psxMemRLUT + 0xbfc0, psxCore.psxMemRLUT + 0x1fc0, 0x08 * sizeof(void *));

// MemW
	for (i = 0; i < 0x80; i++) psxCore.psxMemWLUT[i + 0x0000] = (u8 *)&psxM[(i & 0x1f) << 16];

	memcpy(psxCore.psxMemWLUT + 0x8000, psxCore.psxMemWLUT, 0x80 * sizeof(void *));
	memcpy(psxCore.psxMemWLUT + 0xa000, psxCore.psxMemWLUT, 0x80 * sizeof(void *));

	// Don't allow writes to PIO Expansion region (psxP) to take effect.
	// NOTE: Not sure if this is needed to fix any games but seems wise,
	//       seeing as some games do read from PIO as part of copy-protection
	//       check. (See fix in psxMemReset() regarding psxP region reads).
	psxCore.psxMemWLUT[0x1f00] = INVALID_PTR;
	psxCore.psxMemWLUT[0x1f80] = (u8 *)psxH;

	return 0;
}

void psxMemReset() {
	int temp;
	memset(psxM, 0, 0x00200000);
	memset(psxP, 0xff, 0x00010000);
	memset(psxR, 0, 0x80000);
  if(!biosFile || (biosDevice == BIOSDEVICE_HLE)) {
    Config.HLE = BIOS_HLE;
    return;
  }
  else {
    biosFile->offset = 0; //must reset otherwise the if statement will fail!
  	if(biosFile_readFile(biosFile, &temp, 4) == 4) {  //bios file exists
  	  biosFile->offset = 0;
  		if(biosFile_readFile(biosFile, psxR, 0x80000) != 0x80000) { //failed size
  		  //printf("Using HLE\n");
  		  Config.HLE = BIOS_HLE;
  	  }
  		else {
    		//printf("Using BIOS file %s\n",biosFile->name);
  		  Config.HLE = BIOS_USER_DEFINED;
  	  }
  	}
  	else {  //bios fails to open
    	Config.HLE = BIOS_HLE;
  	}
	}
}

void psxMemShutdown() {
	/*if (LIGHTREC_CUSTOM_MAP)
		lightrec_free_mmap();
	else
		psxMemFreeMap();

	free(psxMemRLUT); psxMemRLUT = NULL;
	free(psxMemWLUT); psxMemWLUT = NULL;*/
}

static int writeok = 1;

u8 psxMemRead8(u32 mem) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if ((mem & 0xffff) < 0x400)
			return psxHu8(mem);
		else
			return psxHwRead8(mem);
	} else {
		p = (char *)(psxCore.psxMemRLUT[t]);
		if (p != INVALID_PTR) {
			if (Config.Debug)
				DebugCheckBP((mem & 0xffffff) | 0x80000000, R1);
			return *(u8 *)(p + (mem & 0xffff));
		} else {
#ifdef PSXMEM_LOG
			PSXMEM_LOG("err lb %8.8lx\n", mem);
#endif
			return 0xFF;
		}
	}
}

u16 psxMemRead16(u32 mem) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if ((mem & 0xffff) < 0x400)
			return psxHu16(mem);
		else
			return psxHwRead16(mem);
	} else {
		p = (char *)(psxCore.psxMemRLUT[t]);
		if (p != INVALID_PTR) {
			if (Config.Debug)
				DebugCheckBP((mem & 0xffffff) | 0x80000000, R2);
			return SWAPu16(*(u16 *)(p + (mem & 0xffff)));
		} else {
#ifdef PSXMEM_LOG
			PSXMEM_LOG("err lh %8.8lx\n", mem);
#endif
			return 0xFFFF;
		}
	}
}

u32 psxMemRead32(u32 mem) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if ((mem & 0xffff) < 0x400)
			return psxHu32(mem);
		else
			return psxHwRead32(mem);
	} else {
		p = (char *)(psxCore.psxMemRLUT[t]);
		if (p != INVALID_PTR) {
			if (Config.Debug)
				DebugCheckBP((mem & 0xffffff) | 0x80000000, R4);
			return SWAPu32(*(u32 *)(p + (mem & 0xffff)));
		} else {
#ifdef PSXMEM_LOG
			if (writeok) { PSXMEM_LOG("err lw %8.8lx\n", mem); }
#endif
			return 0xFFFFFFFF;
		}
	}
}

void psxMemWrite8(u32 mem, u8 value) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if ((mem & 0xffff) < 0x400)
			psxHu8(mem) = value;
		else
			psxHwWrite8(mem, value);
	} else {
		p = (char *)(psxCore.psxMemWLUT[t]);
		if (p != INVALID_PTR) {
			if (Config.Debug)
				DebugCheckBP((mem & 0xffffff) | 0x80000000, W1);
			*(u8 *)(p + (mem & 0xffff)) = value;
#ifndef DRC_DISABLE
			psxCpu->Clear((mem & (~3)), 1);
#endif
		} else {
#ifdef PSXMEM_LOG
			PSXMEM_LOG("err sb %8.8lx\n", mem);
#endif
		}
	}
}

void psxMemWrite16(u32 mem, u16 value) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if ((mem & 0xffff) < 0x400)
			psxHu16ref(mem) = SWAPu16(value);
		else
			psxHwWrite16(mem, value);
	} else {
		p = (char *)(psxCore.psxMemWLUT[t]);
		if (p != INVALID_PTR) {
			if (Config.Debug)
				DebugCheckBP((mem & 0xffffff) | 0x80000000, W2);
			*(u16 *)(p + (mem & 0xffff)) = SWAPu16(value);
#ifndef DRC_DISABLE
			psxCpu->Clear((mem & (~3)), 1);
#endif
		} else {
#ifdef PSXMEM_LOG
			PSXMEM_LOG("err sh %8.8lx\n", mem);
#endif
		}
	}
}

void psxMemWrite32(u32 mem, u32 value) {
	char *p;
	u32 t;

//	if ((mem&0x1fffff) == 0x71E18 || value == 0x48088800) SysPrintf("t2fix!!\n");
	t = mem >> 16;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if ((mem & 0xffff) < 0x400)
			psxHu32ref(mem) = SWAPu32(value);
		else
			psxHwWrite32(mem, value);
	} else {
		p = (char *)(psxCore.psxMemWLUT[t]);
		if (p != INVALID_PTR) {
			if (Config.Debug)
				DebugCheckBP((mem & 0xffffff) | 0x80000000, W4);
			*(u32 *)(p + (mem & 0xffff)) = SWAPu32(value);
#ifndef DRC_DISABLE
			psxCpu->Clear(mem, 1);
#endif
		} else {
			if (mem != 0xfffe0130) {
#ifndef DRC_DISABLE
				if (!writeok)
					psxCpu->Clear(mem, 1);
#endif

#ifdef PSXMEM_LOG
				if (writeok) { PSXMEM_LOG("err sw %8.8lx\n", mem); }
#endif
			} else {
				int i;

				switch (value) {
					case 0x800: case 0x804:
						if (writeok == 0) break;
						writeok = 0;
						memset(psxCore.psxMemWLUT + 0x0000, (uintptr_t)INVALID_PTR, 0x80 * sizeof(void *));
						memset(psxCore.psxMemWLUT + 0x8000, (uintptr_t)INVALID_PTR, 0x80 * sizeof(void *));
						memset(psxCore.psxMemWLUT + 0xa000, (uintptr_t)INVALID_PTR, 0x80 * sizeof(void *));
						/* Required for icache interpreter otherwise Armored Core won't boot on icache interpreter */
						psxCpu->Notify(R3000ACPU_NOTIFY_CACHE_ISOLATED, NULL);
						break;
					case 0x00: case 0x1e988:
						if (writeok == 1) break;
						writeok = 1;
						for (i = 0; i < 0x80; i++) psxCore.psxMemWLUT[i + 0x0000] = (void *)&psxM[(i & 0x1f) << 16];
						memcpy(psxCore.psxMemWLUT + 0x8000, psxCore.psxMemWLUT, 0x80 * sizeof(void *));
						memcpy(psxCore.psxMemWLUT + 0xa000, psxCore.psxMemWLUT, 0x80 * sizeof(void *));
						/* Dynarecs might take this opportunity to flush their code cache */
						psxCpu->Notify(R3000ACPU_NOTIFY_CACHE_UNISOLATED, NULL);
						break;
					default:
#ifdef PSXMEM_LOG
						PSXMEM_LOG("unk %8.8lx = %x\n", mem, value);
#endif
						break;
				}
			}
		}
	}
}

void *psxMemPointer(u32 mem) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if ((mem & 0xffff) < 0x400)
			return (void *)&psxH[mem];
		else
			return NULL;
	} else {
		p = (char *)(psxCore.psxMemWLUT[t]);
		if (p != INVALID_PTR) {
			return (void *)(p + (mem & 0xffff));
		}
		return NULL;
	}
}
