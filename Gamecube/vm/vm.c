/**
 * vm.c - Implements Virtual Memory for GC/Wii
 * Copyright (C) 2012  tueidj
 * ISFS code replaced with ARAM code by emu_kidid
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
**/

#include <gccore.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <ogc/machine/processor.h>
#include <ogc/aram.h>
#include "vm.h"

#include <stdio.h>

typedef u8 vm_page[PAGE_SIZE];

static p_map phys_map[2048+(PTE_SIZE/PAGE_SIZE)];
static vm_map virt_map[65536];
static u16 pmap_max, pmap_head;

static PTE* HTABORG;
static vm_page* VM_Base;
static vm_page* MEM_Base = NULL;

static mutex_t vm_mutex = LWP_MUTEX_NULL;
static bool vm_initialized = 0;

static __inline__ void tlbie(void* p)
{
	asm volatile("tlbie %0" :: "r"(p));
}

static u16 locate_oldest(void)
{
	u16 head = pmap_head;

	for(;;++head)
	{
		PTE *p;

		if (head >= pmap_max)
			head = 0;

		if (!phys_map[head].valid || phys_map[head].locked)
			continue;

		p = HTABORG+phys_map[head].pte_index;
		tlbie(VM_Base+phys_map[head].page_index);

		if (p->C)
		{
			p->C = 0;
			phys_map[head].dirty = 1;
			continue;
		}

		if (p->R)
		{
			p->R = 0;
			continue;
		}

		p->data[0] = 0;

		pmap_head = head+1;
		return head;
	}
}

static PTE* StorePTE(PTEG pteg, u32 virtual, u32 physical, u8 WIMG, u8 PP, int secondary, u32 vsid)
{
	int i;
	PTE p = {{0}};

	p.valid = 1;
	p.VSID = vsid;
	p.hash = secondary ? 1:0;
	p.API = virtual >> 22;
	p.RPN = physical >> 12;
	p.WIMG = WIMG;
	p.PP = PP;

	for (i=0; i < 8; i++)
	{
		if (pteg[i].valid)
			continue;

		asm volatile("tlbie %0" : : "r"(virtual));
		pteg[i].data[1] = p.data[1];
		pteg[i].data[0] = p.data[0];
		return pteg+i;
	}

	return NULL;
}

static PTEG CalcPTEG(u32 virtual, int secondary, u32 vsid)
{
	uint32_t segment_index = (virtual >> 12) & 0xFFFF;
	u32 ptr = MEM_VIRTUAL_TO_PHYSICAL(HTABORG);
	u32 hash = segment_index ^ vsid;

	if (secondary) hash = ~hash;

	hash &= (HTABMASK << 10) | 0x3FF;
	ptr |= hash << 6;

	return (PTEG)MEM_PHYSICAL_TO_K0(ptr);
}

static PTE* insert_pte(u32 virtual, u32 physical, u8 WIMG, u8 PP)
{
	u32 vsid = virtual >> 28;
	PTE *pte;
	int i;

	for (i=0; i < 2; i++)
	{
		PTEG pteg = CalcPTEG(virtual, i, vsid);
		pte = StorePTE(pteg, virtual, physical, WIMG, PP, i, vsid);
		if (pte)
			return pte;
	}

	return NULL;
}

static void tlbia(void)
{
	int i;
	for (i=0; i < 64; i++)
		asm volatile("tlbie %0" :: "r" (i*PAGE_SIZE));
}

/* This definition is wrong, pHndl does not take frame_context* as a parameter,
 * it has to adjust the stack pointer and finish filling frame_context itself
 */
void __exception_sethandler(u32 nExcept, void (*pHndl)(frame_context*));
extern void default_exceptionhandler();
// use our own exception stub because libogc stupidly requires it
extern void dsi_handler();

void* VM_Init(u32 VMSize, u32 MEMSize)
{
	u32 i;
	u16 index, v_index;

	if (vm_initialized)
		return VM_Base;

	// parameter checking
	if (VMSize>MAX_VM_SIZE || MEMSize<MIN_MEM_SIZE || MEMSize>MAX_MEM_SIZE)
	{
		errno = EINVAL;
		return NULL;
	}

	VMSize = (VMSize+PAGE_SIZE-1)&PAGE_MASK;
	MEMSize = (MEMSize+PAGE_SIZE-1)&PAGE_MASK;
	VM_Base = (vm_page*)(0x7F000000);
	pmap_max = MEMSize / PAGE_SIZE + 16;

	if (VMSize <= MEMSize)
	{
		errno = EINVAL;
		return NULL;
	}

	if (LWP_MutexInit(&vm_mutex, 0) != 0)
	{
		errno = ENOLCK;
		return NULL;
	}

	MEMSize += PTE_SIZE;
	MEM_Base = (vm_page*)memalign(PAGE_SIZE, MEMSize);

	if (MEM_Base==NULL)
	{
		errno = ENOMEM;
		return NULL;
	}

	tlbia();
	DCZeroRange(MEM_Base, MEMSize);
	HTABORG = (PTE*)(((u32)MEM_Base+0xFFFF)&~0xFFFF);

	// initial commit: map pmap_max pages to fill PTEs with valid RPNs
	for (index=0,v_index=0; index<pmap_max; ++index,++v_index)
	{
		if ((PTE*)(MEM_Base+index) == HTABORG)
		{
			for (i=0; i<(PTE_SIZE/PAGE_SIZE); ++i,++index)
				phys_map[index].valid = 0;

			--index;
			--v_index;
			continue;
		}

		phys_map[index].valid = 1;
		phys_map[index].locked = 0;
		phys_map[index].dirty = 0;
		phys_map[index].page_index = v_index;
		phys_map[index].pte_index = insert_pte((u32)(VM_Base+v_index),
						       MEM_VIRTUAL_TO_PHYSICAL(MEM_Base+index),
						       0, 0b10) - HTABORG;
		virt_map[v_index].committed = 0;
		virt_map[v_index].p_map_index = index;
	}

	// all indexes up to 65536
	for (; v_index; ++v_index)
	{
		virt_map[v_index].committed = 0;
		virt_map[v_index].p_map_index = pmap_max;
	}

	pmap_head = 0;

	// set SDR1
	mtspr(25, MEM_VIRTUAL_TO_PHYSICAL(HTABORG)|HTABMASK);
	// enable SR
	asm volatile("mtsrin %0,%1" :: "r"((u32)VM_Base >> 28), "r"(VM_Base));
	// hook DSI
	__exception_sethandler(EX_DSI, dsi_handler);

	atexit(VM_Deinit);

	vm_initialized = 1;

	return VM_Base;
}

void VM_Deinit(void)
{
	if (!vm_initialized)
		return;

	// disable SR
	asm volatile("mtsrin %0,%1" :: "r"(0x80000000), "r"(VM_Base));
	// restore default DSI handler
	__exception_sethandler(EX_DSI, default_exceptionhandler);

	free(MEM_Base);
	MEM_Base = NULL;

	if (vm_mutex != LWP_MUTEX_NULL)
	{
		LWP_MutexDestroy(vm_mutex);
		vm_mutex = LWP_MUTEX_NULL;
	}

	vm_initialized = 0;
}

int vm_dsi_handler(u32 DSISR, u32 DAR)
{
	u16 v_index;
	u16 p_index;

	if (DAR<(u32)VM_Base || DAR>=0x80000000)
		return 0;
	if ((DSISR&~0x02000000)!=0x40000000)
		return 0;
	if (!vm_initialized)
		return 0;

	LWP_MutexLock(vm_mutex);

	DAR &= ~0xFFF;
	v_index = (vm_page*)DAR - VM_Base;

	p_index = locate_oldest();

	// purge p_index if it's dirty
	if (phys_map[p_index].dirty)
	{
		DCFlushRange(MEM_Base+p_index, PAGE_SIZE);
		AR_StartDMA(AR_MRAMTOARAM,(u32)(MEM_Base+p_index),phys_map[p_index].page_index*PAGE_SIZE,PAGE_SIZE);
		while (AR_GetDMAStatus());
		virt_map[phys_map[p_index].page_index].committed = 1;
		virt_map[phys_map[p_index].page_index].p_map_index = pmap_max;
		phys_map[p_index].dirty = 0;
	}

	// fetch v_index if it has been previously committed
	if (virt_map[v_index].committed)
	{
		DCInvalidateRange(MEM_Base+p_index, PAGE_SIZE);
		AR_StartDMA(AR_ARAMTOMRAM,(u32)(MEM_Base+p_index),v_index*PAGE_SIZE,PAGE_SIZE);
		while (AR_GetDMAStatus());
	}
	else
		DCZeroRange(MEM_Base+p_index, PAGE_SIZE);

	virt_map[v_index].p_map_index = p_index;
	phys_map[p_index].page_index = v_index;
	phys_map[p_index].pte_index = insert_pte((u32)(VM_Base+v_index),
						 MEM_VIRTUAL_TO_PHYSICAL(MEM_Base+p_index),
						 0, 0b10) - HTABORG;

	LWP_MutexUnlock(vm_mutex);

	return 1;
}
