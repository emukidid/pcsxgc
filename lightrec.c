#include <lightrec.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "CdRom.h"
#include "PsxGpu.h"
#include "Gte.h"
#include "Mdec.h"
#include "PsxCounters.h"
#include "PsxDma.h"
#include "PsxHw.h"
#include "PsxMem.h"
#include "R3000A.h"

#define ARRAY_SIZE(x) (sizeof(x) ? sizeof(x) / sizeof((x)[0]) : 0)

#ifdef __GNUC__
#	define likely(x)       __builtin_expect(!!(x),1)
#	define unlikely(x)     __builtin_expect(!!(x),0)
#else
#	define likely(x)       (x)
#	define unlikely(x)     (x)
#endif

#define FORCE_STEP_BY_STEP 1

struct lightrec_state *lightrec_state;

static char *name = "pcsx";

static u32 event_cycles[PSXINT_COUNT];
static u32 next_interrupt;

static bool ram_disabled;
static bool use_lightrec_interpreter = true;
static bool booting;

enum my_cp2_opcodes {
	OP_CP2_RTPS		= 0x01,
	OP_CP2_NCLIP		= 0x06,
	OP_CP2_OP		= 0x0c,
	OP_CP2_DPCS		= 0x10,
	OP_CP2_INTPL		= 0x11,
	OP_CP2_MVMVA		= 0x12,
	OP_CP2_NCDS		= 0x13,
	OP_CP2_CDP		= 0x14,
	OP_CP2_NCDT		= 0x16,
	OP_CP2_NCCS		= 0x1b,
	OP_CP2_CC		= 0x1c,
	OP_CP2_NCS		= 0x1e,
	OP_CP2_NCT		= 0x20,
	OP_CP2_SQR		= 0x28,
	OP_CP2_DCPL		= 0x29,
	OP_CP2_DPCT		= 0x2a,
	OP_CP2_AVSZ3		= 0x2d,
	OP_CP2_AVSZ4		= 0x2e,
	OP_CP2_RTPT		= 0x30,
	OP_CP2_GPF		= 0x3d,
	OP_CP2_GPL		= 0x3e,
	OP_CP2_NCCT		= 0x3f,
};

static void (*cp2_ops[])(struct psxCP2Regs *) = {
	[OP_CP2_RTPS] = gteRTPS,
	[OP_CP2_NCLIP] = gteNCLIP,
	[OP_CP2_OP] = gteOP,
	[OP_CP2_DPCS] = gteDPCS,
	[OP_CP2_INTPL] = gteINTPL,
	[OP_CP2_MVMVA] = gteMVMVA,
	[OP_CP2_NCDS] = gteNCDS,
	[OP_CP2_CDP] = gteCDP,
	[OP_CP2_NCDT] = gteNCDT,
	[OP_CP2_NCCS] = gteNCCS,
	[OP_CP2_CC] = gteCC,
	[OP_CP2_NCS] = gteNCS,
	[OP_CP2_NCT] = gteNCT,
	[OP_CP2_SQR] = gteSQR,
	[OP_CP2_DCPL] = gteDCPL,
	[OP_CP2_DPCT] = gteDPCT,
	[OP_CP2_AVSZ3] = gteAVSZ3,
	[OP_CP2_AVSZ4] = gteAVSZ4,
	[OP_CP2_RTPT] = gteRTPT,
	[OP_CP2_GPF] = gteGPF,
	[OP_CP2_GPL] = gteGPL,
	[OP_CP2_NCCT] = gteNCCT,
};

static char cache_buf[64 * 1024];

static void cop2_op(struct lightrec_state *state, u32 func)
{
	struct lightrec_registers *regs = lightrec_get_registers(state);

	psxCore.code = func;

	if (unlikely(!cp2_ops[func & 0x3f])) {
		fprintf(stderr, "Invalid CP2 function %u\n", func);
	} else {
		/* This works because regs->cp2c comes right after regs->cp2d,
		 * so it can be cast to a pcsxCP2Regs pointer. */
		cp2_ops[func & 0x3f]((psxCP2Regs *) regs->cp2d);
	}
}

static bool has_interrupt(void)
{
	struct lightrec_registers *regs = lightrec_get_registers(lightrec_state);

	return ((psxHu32(0x1070) & psxHu32(0x1074)) &&
		(regs->cp0[12] & 0x401) == 0x401) ||
		(regs->cp0[12] & regs->cp0[13] & 0x0300);
}

static void lightrec_restore_state(struct lightrec_state *state)
{
	lightrec_reset_cycle_count(state, psxCore.cycle);

	if (FORCE_STEP_BY_STEP || booting || has_interrupt())
		lightrec_set_exit_flags(state, LIGHTREC_EXIT_CHECK_INTERRUPT);
	else
		lightrec_set_target_cycle_count(state, next_interrupt);
}

static void hw_write_byte(struct lightrec_state *state,
			  u32 op, void *host, u32 mem, u8 val)
{

	psxCore.cycle = lightrec_current_cycle_count(state);

	psxHwWrite8(mem, val);

	lightrec_restore_state(state);
}

static void hw_write_half(struct lightrec_state *state,
			  u32 op, void *host, u32 mem, u16 val)
{
	psxCore.cycle = lightrec_current_cycle_count(state);

	psxHwWrite16(mem, val);

	lightrec_restore_state(state);
}

static void hw_write_word(struct lightrec_state *state,
			  u32 op, void *host, u32 mem, u32 val)
{
	psxCore.cycle = lightrec_current_cycle_count(state);

	psxHwWrite32(mem, val);

	lightrec_restore_state(state);
}

static u8 hw_read_byte(struct lightrec_state *state, u32 op, void *host, u32 mem)
{
	u8 val;

	psxCore.cycle = lightrec_current_cycle_count(state);

	val = psxHwRead8(mem);

	lightrec_restore_state(state);

	return val;
}

static u16 hw_read_half(struct lightrec_state *state,
			u32 op, void *host, u32 mem)
{
	u16 val;

	psxCore.cycle = lightrec_current_cycle_count(state);

	val = psxHwRead16(mem);

	lightrec_restore_state(state);

	return val;
}

static u32 hw_read_word(struct lightrec_state *state,
			u32 op, void *host, u32 mem)
{
	u32 val;

	psxCore.cycle = lightrec_current_cycle_count(state);

	val = psxHwRead32(mem);

	lightrec_restore_state(state);

	return val;
}

static struct lightrec_mem_map_ops hw_regs_ops = {
	.sb = hw_write_byte,
	.sh = hw_write_half,
	.sw = hw_write_word,
	.lb = hw_read_byte,
	.lh = hw_read_half,
	.lw = hw_read_word,
};

static u32 cache_ctrl;

static void cache_ctrl_write_word(struct lightrec_state *state,
				  u32 op, void *host, u32 mem, u32 val)
{
	cache_ctrl = val;
}

static u32 cache_ctrl_read_word(struct lightrec_state *state,
				u32 op, void *host, u32 mem)
{
	return cache_ctrl;
}

static struct lightrec_mem_map_ops cache_ctrl_ops = {
	.sw = cache_ctrl_write_word,
	.lw = cache_ctrl_read_word,
};

static struct lightrec_mem_map lightrec_map[] = {
	[PSX_MAP_KERNEL_USER_RAM] = {
		/* Kernel and user memory */
		.pc = 0x00000000,
		.length = 0x200000,
	},
	[PSX_MAP_BIOS] = {
		/* BIOS */
		.pc = 0x1fc00000,
		.length = 0x80000,
	},
	[PSX_MAP_SCRATCH_PAD] = {
		/* Scratch pad */
		.pc = 0x1f800000,
		.length = 0x400,
	},
	[PSX_MAP_PARALLEL_PORT] = {
		/* Parallel port */
		.pc = 0x1f000000,
		.length = 0x10000,
	},
	[PSX_MAP_HW_REGISTERS] = {
		/* Hardware registers */
		.pc = 0x1f801000,
		.length = 0x2000,
		.ops = &hw_regs_ops,
	},
	[PSX_MAP_CACHE_CONTROL] = {
		/* Cache control */
		.pc = 0x5ffe0130,
		.length = 4,
		.ops = &cache_ctrl_ops,
	},

	/* Mirrors of the kernel/user memory */
	[PSX_MAP_MIRROR1] = {
		.pc = 0x00200000,
		.length = 0x200000,
		.mirror_of = &lightrec_map[PSX_MAP_KERNEL_USER_RAM],
	},
	[PSX_MAP_MIRROR2] = {
		.pc = 0x00400000,
		.length = 0x200000,
		.mirror_of = &lightrec_map[PSX_MAP_KERNEL_USER_RAM],
	},
	[PSX_MAP_MIRROR3] = {
		.pc = 0x00600000,
		.length = 0x200000,
		.mirror_of = &lightrec_map[PSX_MAP_KERNEL_USER_RAM],
	},
	[PSX_MAP_CODE_BUFFER] = {
		.length = sizeof(psxCore.code_buffer),
	},
};

static void lightrec_enable_ram(struct lightrec_state *state, bool enable)
{
	if (enable)
		memcpy(psxCore.psxM, cache_buf, sizeof(cache_buf));
	else
		memcpy(cache_buf, psxCore.psxM, sizeof(cache_buf));

	ram_disabled = !enable;
}

static bool lightrec_can_hw_direct(u32 kaddr, bool is_write, u8 size)
{
	switch (size) {
	case 8:
		switch (kaddr) {
		case 0x1f801040:
		case 0x1f801050:
		case 0x1f801800:
		case 0x1f801801:
		case 0x1f801802:
		case 0x1f801803:
			return false;
		default:
			return true;
		}
	case 16:
		switch (kaddr) {
		case 0x1f801040:
		case 0x1f801044:
		case 0x1f801048:
		case 0x1f80104a:
		case 0x1f80104e:
		case 0x1f801050:
		case 0x1f801054:
		case 0x1f80105a:
		case 0x1f80105e:
		case 0x1f801100:
		case 0x1f801104:
		case 0x1f801108:
		case 0x1f801110:
		case 0x1f801114:
		case 0x1f801118:
		case 0x1f801120:
		case 0x1f801124:
		case 0x1f801128:
			return false;
		case 0x1f801070:
		case 0x1f801074:
			return !is_write;
		default:
			return kaddr < 0x1f801c00 || kaddr >= 0x1f801e00;
		}
	default:
		switch (kaddr) {
		case 0x1f801040:
		case 0x1f801050:
		case 0x1f801100:
		case 0x1f801104:
		case 0x1f801108:
		case 0x1f801110:
		case 0x1f801114:
		case 0x1f801118:
		case 0x1f801120:
		case 0x1f801124:
		case 0x1f801128:
		case 0x1f801810:
		case 0x1f801814:
		case 0x1f801820:
		case 0x1f801824:
			return false;
		case 0x1f801070:
		case 0x1f801074:
		case 0x1f801088:
		case 0x1f801098:
		case 0x1f8010a8:
		case 0x1f8010b8:
		case 0x1f8010c8:
		case 0x1f8010e8:
		case 0x1f8010f4:
			return !is_write;
		default:
			return !is_write || kaddr < 0x1f801c00 || kaddr >= 0x1f801e00;
		}
	}
}

static const struct lightrec_ops lightrec_ops = {
	.cop2_op = cop2_op,
	.enable_ram = lightrec_enable_ram,
	.hw_direct = lightrec_can_hw_direct,
};

static int lightrec_plugin_init(void)
{
	lightrec_map[PSX_MAP_KERNEL_USER_RAM].address = psxCore.psxM;
	lightrec_map[PSX_MAP_BIOS].address = psxCore.psxR;
	lightrec_map[PSX_MAP_SCRATCH_PAD].address = psxCore.psxM + 0x210000;
	lightrec_map[PSX_MAP_PARALLEL_PORT].address = psxCore.psxM + 0x200000;
	lightrec_map[PSX_MAP_HW_REGISTERS].address = psxCore.psxM + 0x211000;
	lightrec_map[PSX_MAP_CODE_BUFFER].address = psxCore.code_buffer;
	/*
	lightrec_map[PSX_MAP_MIRROR1].address = psxM + 0x200000;
	lightrec_map[PSX_MAP_MIRROR2].address = psxM + 0x400000;
	lightrec_map[PSX_MAP_MIRROR3].address = psxM + 0x600000;
	*/

	lightrec_state = lightrec_init(name,
			lightrec_map, ARRAY_SIZE(lightrec_map),
			&lightrec_ops);

	signal(SIGPIPE, exit);
	return 0;
}

static void lightrec_dump_regs(struct lightrec_state *state)
{
	struct lightrec_registers *regs = lightrec_get_registers(state);

	if (booting)
		memcpy(&psxCore.GPR, regs->gpr, sizeof(regs->gpr));
	psxCore.CP0.n.Status = regs->cp0[12];
	psxCore.CP0.n.Cause = regs->cp0[13];
}

static void lightrec_restore_regs(struct lightrec_state *state)
{
	struct lightrec_registers *regs = lightrec_get_registers(state);

	if (booting)
		memcpy(regs->gpr, &psxCore.GPR, sizeof(regs->gpr));
	regs->cp0[12] = psxCore.CP0.n.Status;
	regs->cp0[13] = psxCore.CP0.n.Cause;
	regs->cp0[14] = psxCore.CP0.n.EPC;
}

static void schedule_timeslice(void)
{
	u32 i, c = psxCore.cycle;
	u32 irqs = psxCore.interrupt;
	s32 min, dif;

	min = PSXCLK;
	for (i = 0; irqs != 0; i++, irqs >>= 1) {
		if (!(irqs & 1))
			continue;
		dif = event_cycles[i] - c;
		if (0 < dif && dif < min)
			min = dif;
	}
	next_interrupt = c + min;
}

typedef void (irq_func)();

static irq_func * const irq_funcs[] = {
	[PSXINT_SIO]	= sioInterrupt,
	[PSXINT_CDR]	= cdrInterrupt,
	[PSXINT_CDREAD]	= cdrReadInterrupt,
	[PSXINT_GPUDMA]	= gpuInterrupt,
	[PSXINT_MDECOUTDMA] = mdec1Interrupt,
	[PSXINT_SPUDMA]	= spuInterrupt,
	[PSXINT_MDECINDMA] = mdec0Interrupt,
	[PSXINT_GPUOTCDMA] = gpuotcInterrupt,
	[PSXINT_CDRDMA] = cdrDmaInterrupt,
	[PSXINT_CDRLID] = cdrLidSeekInterrupt,
	[PSXINT_CDRPLAY] = cdrPlayInterrupt,
	[PSXINT_CDRDBUF] = cdrDecodedBufferInterrupt,
	//[PSXINT_SPU_UPDATE] = spuUpdate,
	//[PSXINT_RCNT] = psxRcntUpdate,
};

/* local dupe of psxBranchTest, using event_cycles */
static void irq_test(void)
{
	u32 irqs = psxCore.interrupt;
	u32 cycle = psxCore.cycle;
	u32 irq, irq_bits;

	// irq_funcs() may queue more irqs
	psxCore.interrupt = 0;

	for (irq = 0, irq_bits = irqs; irq_bits != 0; irq++, irq_bits >>= 1) {
		if (!(irq_bits & 1))
			continue;
		if ((s32)(cycle - event_cycles[irq]) >= 0) {
			irqs &= ~(1 << irq);
			irq_funcs[irq]();
		}
	}
	psxCore.interrupt |= irqs;

	if ((psxHu32(0x1070) & psxHu32(0x1074)) && (psxCore.CP0.n.Status & 0x401) == 0x401) {
		psxException(0x400, 0);
	}
}

void gen_interupt()
{
	irq_test();
	schedule_timeslice();
}

static void lightrec_plugin_execute_block(void)
{
	u32 flags;

	if (!FORCE_STEP_BY_STEP)
		gen_interupt();

	if (FORCE_STEP_BY_STEP || booting)
		next_interrupt = psxCore.cycle;

	lightrec_reset_cycle_count(lightrec_state, psxCore.cycle);
	lightrec_restore_regs(lightrec_state);

	if (use_lightrec_interpreter) {
		psxCore.pc = lightrec_run_interpreter(lightrec_state,
						      psxCore.pc, next_interrupt);
	} else {
		psxCore.pc = lightrec_execute(lightrec_state,
					      psxCore.pc, next_interrupt);
	}

	psxCore.cycle = lightrec_current_cycle_count(lightrec_state);

	lightrec_dump_regs(lightrec_state);
	flags = lightrec_exit_flags(lightrec_state);

	if (flags & LIGHTREC_EXIT_SEGFAULT) {
		fprintf(stderr, "Exiting at cycle 0x%08x\n",
			psxCore.cycle);
		exit(1);
	}

	if (flags & LIGHTREC_EXIT_NOMEM) {
		fprintf(stderr, "Out of memory!\n");
		exit(1);
	}

	if (flags & LIGHTREC_EXIT_SYSCALL)
		psxException(0x20, 0);

	if (booting && (psxCore.pc & 0xff800000) == 0x80000000)
		booting = false;

	if (FORCE_STEP_BY_STEP)
		psxBranchTest();

	if ((psxCore.CP0.n.Cause & psxCore.CP0.n.Status & 0x300) &&
			(psxCore.CP0.n.Status & 0x1)) {
		/* Handle software interrupts */
		psxCore.CP0.n.Cause &= ~0x7c;
		psxException(psxCore.CP0.n.Cause, 0);
	}
}

static void lightrec_plugin_execute(void)
{
	extern int stop;

	while (!stop)
		lightrec_plugin_execute_block();
}

static void lightrec_plugin_clear(u32 addr, u32 size)
{
	/* size * 4: PCSX uses DMA units */
	lightrec_invalidate(lightrec_state, addr, size * 4);
}

static void lightrec_plugin_shutdown(void)
{
	lightrec_destroy(lightrec_state);
}

static void lightrec_plugin_reset(void)
{
	struct lightrec_registers *regs;

	regs = lightrec_get_registers(lightrec_state);

	/* Invalidate all blocks */
	lightrec_invalidate_all(lightrec_state);

	/* Reset registers */
	memset(regs, 0, sizeof(*regs));

	regs->cp0[12] = 0x10900000; // COP0 enabled | BEV = 1 | TS = 1
	regs->cp0[15] = 0x00000002; // PRevID = Revision ID, same as R3000A

	booting = true;
}

void lightrec_plugin_prepare_load_state(void)
{
	struct lightrec_registers *regs;

	regs = lightrec_get_registers(lightrec_state);
	memcpy(regs->cp2d, &psxCore.CP2, sizeof(regs->cp2d) + sizeof(regs->cp2c));
	memcpy(regs->cp0, &psxCore.CP0, sizeof(regs->cp0));
	memcpy(regs->gpr, &psxCore.GPR, sizeof(regs->gpr));

	lightrec_invalidate_all(lightrec_state);
}

void lightrec_plugin_prepare_save_state(void)
{
	struct lightrec_registers *regs;

	regs = lightrec_get_registers(lightrec_state);
	memcpy(&psxCore.CP2, regs->cp2d, sizeof(regs->cp2d) + sizeof(regs->cp2c));
	memcpy(&psxCore.CP0, regs->cp0, sizeof(regs->cp0));
	memcpy(&psxCore.GPR, regs->gpr, sizeof(regs->gpr));
}

R3000Acpu psxRec =
{
	lightrec_plugin_init,
	lightrec_plugin_reset,
	lightrec_plugin_execute,
	lightrec_plugin_execute_block,
	lightrec_plugin_clear,
	lightrec_plugin_shutdown,
};

/* Implement the sysconf() symbol which is needed by GNU Lightning */
long sysconf(int name)
{
	switch (name) {
	case _SC_PAGE_SIZE:
		return 4096;
	default:
		return -EINVAL;
	}
}
