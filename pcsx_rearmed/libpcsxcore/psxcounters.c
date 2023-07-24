/***************************************************************************
 *   Copyright (C) 2010 by Blade_Arma                                      *
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
 * Internal PSX counters.
 */

#include "psxcounters.h"
#include "gpu.h"
//#include "debug.h"
#define DebugVSync()

/******************************************************************************/

enum
{
    Rc0Gate           = 0x0001, // 0    not implemented
    Rc1Gate           = 0x0001, // 0    not implemented
    Rc2Disable        = 0x0001, // 0    partially implemented
    RcUnknown1        = 0x0002, // 1    ?
    RcUnknown2        = 0x0004, // 2    ?
    RcCountToTarget   = 0x0008, // 3
    RcIrqOnTarget     = 0x0010, // 4
    RcIrqOnOverflow   = 0x0020, // 5
    RcIrqRegenerate   = 0x0040, // 6
    RcUnknown7        = 0x0080, // 7    ?
    Rc0PixelClock     = 0x0100, // 8    fake implementation
    Rc1HSyncClock     = 0x0100, // 8
    Rc2Unknown8       = 0x0100, // 8    ?
    Rc0Unknown9       = 0x0200, // 9    ?
    Rc1Unknown9       = 0x0200, // 9    ?
    Rc2OneEighthClock = 0x0200, // 9
    RcUnknown10       = 0x0400, // 10   ?
    RcCountEqTarget   = 0x0800, // 11
    RcOverflow        = 0x1000, // 12
    RcUnknown13       = 0x2000, // 13   ? (always zero)
    RcUnknown14       = 0x4000, // 14   ? (always zero)
    RcUnknown15       = 0x8000, // 15   ? (always zero)
};

#define CounterQuantity           ( 4 )
//static const u32 CounterQuantity  = 4;

static const u32 CountToOverflow  = 0;
static const u32 CountToTarget    = 1;

static const u32 FrameRate[]      = { 60, 50 };
static const u32 HSyncTotal[]     = { 263, 314 }; // actually one more on odd lines for PAL
#define VBlankStart 240

#define VERBOSE_LEVEL 0

/******************************************************************************/
#ifdef DRC_DISABLE
Rcnt rcnts[ CounterQuantity ];
#endif
u32 hSyncCount = 0;
u32 frame_counter = 0;
static u32 hsync_steps = 0;

u32 psxNextCounter = 0, psxNextsCounter = 0;

/******************************************************************************/

static inline
void setIrq( u32 irq )
{
    psxHu32ref(0x1070) |= SWAPu32(irq);
}

static
void verboseLog( u32 level, const char *str, ... )
{
#if VERBOSE_LEVEL > 0
    if( level <= VERBOSE_LEVEL )
    {
        va_list va;
        char buf[ 4096 ];

        va_start( va, str );
        vsprintf( buf, str, va );
        va_end( va );

        printf( "%s", buf );
        fflush( stdout );
    }
#endif
}

/******************************************************************************/

static inline
void _psxRcntWcount( u32 index, u32 value )
{
    if( value > 0xffff )
    {
        verboseLog( 1, "[RCNT %i] wcount > 0xffff: %x\n", index, value );
        value &= 0xffff;
    }

    rcnts[index].cycleStart  = psxRegs.cycle;
    rcnts[index].cycleStart -= value * rcnts[index].rate;

    // TODO: <=.
    if( value < rcnts[index].target )
    {
        rcnts[index].cycle = rcnts[index].target * rcnts[index].rate;
        rcnts[index].counterState = CountToTarget;
    }
    else
    {
        rcnts[index].cycle = 0x10000 * rcnts[index].rate;
        rcnts[index].counterState = CountToOverflow;
    }
}

static inline
u32 _psxRcntRcount( u32 index )
{
    u32 count;

    count  = psxRegs.cycle;
    count -= rcnts[index].cycleStart;
    if (rcnts[index].rate > 1)
        count /= rcnts[index].rate;

    if( count > 0x10000 )
    {
        verboseLog( 1, "[RCNT %i] rcount > 0x10000: %x\n", index, count );
    }
    count &= 0xffff;

    return count;
}

static
void _psxRcntWmode( u32 index, u32 value )
{
    rcnts[index].mode = value;

    switch( index )
    {
        case 0:
            if( value & Rc0PixelClock )
            {
                rcnts[index].rate = 5;
            }
            else
            {
                rcnts[index].rate = 1;
            }
        break;
        case 1:
            if( value & Rc1HSyncClock )
            {
                rcnts[index].rate = (PSXCLK / (FrameRate[Config.PsxType] * HSyncTotal[Config.PsxType]));
            }
            else
            {
                rcnts[index].rate = 1;
            }
        break;
        case 2:
            if( value & Rc2OneEighthClock )
            {
                rcnts[index].rate = 8;
            }
            else
            {
                rcnts[index].rate = 1;
            }

            // TODO: wcount must work.
            if( value & Rc2Disable )
            {
                rcnts[index].rate = 0xffffffff;
            }
        break;
    }
}

/******************************************************************************/

static
void psxRcntSet()
{
    s32 countToUpdate;
    u32 i;

    psxNextsCounter = psxRegs.cycle;
    psxNextCounter  = 0x7fffffff;

    for( i = 0; i < CounterQuantity; ++i )
    {
        countToUpdate = rcnts[i].cycle - (psxNextsCounter - rcnts[i].cycleStart);

        if( countToUpdate < 0 )
        {
            psxNextCounter = 0;
            break;
        }

        if( countToUpdate < (s32)psxNextCounter )
        {
            psxNextCounter = countToUpdate;
        }
    }

    psxRegs.interrupt |= (1 << PSXINT_RCNT);
    new_dyna_set_event(PSXINT_RCNT, psxNextCounter);
}

/******************************************************************************/

static
void psxRcntReset( u32 index )
{
    u32 rcycles;

    rcnts[index].mode |= RcUnknown10;

    if( rcnts[index].counterState == CountToTarget )
    {
        rcycles = psxRegs.cycle - rcnts[index].cycleStart;
        if( rcnts[index].mode & RcCountToTarget )
        {
            rcycles -= rcnts[index].target * rcnts[index].rate;
            rcnts[index].cycleStart = psxRegs.cycle - rcycles;
        }
        else
        {
            rcnts[index].cycle = 0x10000 * rcnts[index].rate;
            rcnts[index].counterState = CountToOverflow;
        }

        if( rcnts[index].mode & RcIrqOnTarget )
        {
            if( (rcnts[index].mode & RcIrqRegenerate) || (!rcnts[index].irqState) )
            {
                verboseLog( 3, "[RCNT %i] irq\n", index );
                setIrq( rcnts[index].irq );
                rcnts[index].irqState = 1;
            }
        }

        rcnts[index].mode |= RcCountEqTarget;

        if( rcycles < 0x10000 * rcnts[index].rate )
            return;
    }

    if( rcnts[index].counterState == CountToOverflow )
    {
        rcycles = psxRegs.cycle - rcnts[index].cycleStart;
        rcycles -= 0x10000 * rcnts[index].rate;

        rcnts[index].cycleStart = psxRegs.cycle - rcycles;

        if( rcycles < rcnts[index].target * rcnts[index].rate )
        {
            rcnts[index].cycle = rcnts[index].target * rcnts[index].rate;
            rcnts[index].counterState = CountToTarget;
        }

        if( rcnts[index].mode & RcIrqOnOverflow )
        {
            if( (rcnts[index].mode & RcIrqRegenerate) || (!rcnts[index].irqState) )
            {
                verboseLog( 3, "[RCNT %i] irq\n", index );
                setIrq( rcnts[index].irq );
                rcnts[index].irqState = 1;
            }
        }

        rcnts[index].mode |= RcOverflow;
    }
}

static void scheduleRcntBase(void)
{
    // Schedule next call, in hsyncs
    if (hSyncCount < VBlankStart)
        hsync_steps = VBlankStart - hSyncCount;
    else
        hsync_steps = HSyncTotal[Config.PsxType] - hSyncCount;

    if (hSyncCount + hsync_steps == HSyncTotal[Config.PsxType])
    {
        rcnts[3].cycle = Config.PsxType ? PSXCLK / 50 : PSXCLK / 60;
    }
    else
    {
        // clk / 50 / 314 ~= 2157.25
        // clk / 60 / 263 ~= 2146.31
        u32 mult = Config.PsxType ? 8836089 : 8791293;
        rcnts[3].cycle = hsync_steps * mult >> 12;
    }
}

void psxRcntUpdate()
{
    u32 cycle;

    cycle = psxRegs.cycle;

    // rcnt 0.
    while( cycle - rcnts[0].cycleStart >= rcnts[0].cycle )
    {
        psxRcntReset( 0 );
    }

    // rcnt 1.
    while( cycle - rcnts[1].cycleStart >= rcnts[1].cycle )
    {
        psxRcntReset( 1 );
    }

    // rcnt 2.
    while( cycle - rcnts[2].cycleStart >= rcnts[2].cycle )
    {
        psxRcntReset( 2 );
    }

    // rcnt base.
    if( cycle - rcnts[3].cycleStart >= rcnts[3].cycle )
    {
        hSyncCount += hsync_steps;

        // VSync irq.
        if( hSyncCount == VBlankStart )
        {
            HW_GPU_STATUS &= SWAP32(~PSXGPU_LCF);
            GPU_vBlank( 1, 0 );
            setIrq( 0x01 );

            EmuUpdate();
            GPU_updateLace();

            if( SPU_async )
            {
                SPU_async( cycle, 1 );
            }
        }
        
        // Update lace.
        if( hSyncCount >= HSyncTotal[Config.PsxType] )
        {
            u32 status, field = 0;
            rcnts[3].cycleStart += Config.PsxType ? PSXCLK / 50 : PSXCLK / 60;
            hSyncCount = 0;
            frame_counter++;

            gpuSyncPluginSR();
            status = SWAP32(HW_GPU_STATUS) | PSXGPU_FIELD;
            if ((status & PSXGPU_ILACE_BITS) == PSXGPU_ILACE_BITS) {
                field = frame_counter & 1;
                status |= field << 31;
                status ^= field << 13;
            }
            HW_GPU_STATUS = SWAP32(status);
            GPU_vBlank(0, field);
        }

        scheduleRcntBase();
    }

    psxRcntSet();

#if 0 //ndef NDEBUG
    DebugVSync();
#endif
}

/******************************************************************************/

void psxRcntWcount( u32 index, u32 value )
{
    verboseLog( 2, "[RCNT %i] wcount: %x\n", index, value );

    _psxRcntWcount( index, value );
    psxRcntSet();
}

void psxRcntWmode( u32 index, u32 value )
{
    verboseLog( 1, "[RCNT %i] wmode: %x\n", index, value );

    _psxRcntWmode( index, value );
    _psxRcntWcount( index, 0 );

    rcnts[index].irqState = 0;
    psxRcntSet();
}

void psxRcntWtarget( u32 index, u32 value )
{
    verboseLog( 1, "[RCNT %i] wtarget: %x\n", index, value );

    rcnts[index].target = value;

    _psxRcntWcount( index, _psxRcntRcount( index ) );
    psxRcntSet();
}

/******************************************************************************/

u32 psxRcntRcount( u32 index )
{
    u32 count;

    count = _psxRcntRcount( index );

    verboseLog( 2, "[RCNT %i] rcount: %x\n", index, count );

    return count;
}

u32 psxRcntRmode( u32 index )
{
    u16 mode;

    mode = rcnts[index].mode;
    rcnts[index].mode &= 0xe7ff;

    verboseLog( 2, "[RCNT %i] rmode: %x\n", index, mode );

    return mode;
}

u32 psxRcntRtarget( u32 index )
{
    verboseLog( 2, "[RCNT %i] rtarget: %x\n", index, rcnts[index].target );

    return rcnts[index].target;
}

/******************************************************************************/

void psxRcntInit()
{
    s32 i;

    // rcnt 0.
    rcnts[0].rate   = 1;
    rcnts[0].irq    = 0x10;

    // rcnt 1.
    rcnts[1].rate   = 1;
    rcnts[1].irq    = 0x20;

    // rcnt 2.
    rcnts[2].rate   = 1;
    rcnts[2].irq    = 0x40;

    // rcnt base.
    rcnts[3].rate   = 1;
    rcnts[3].mode   = RcCountToTarget;
    rcnts[3].target = (PSXCLK / (FrameRate[Config.PsxType] * HSyncTotal[Config.PsxType]));

    for( i = 0; i < CounterQuantity; ++i )
    {
        _psxRcntWcount( i, 0 );
    }

    hSyncCount = 0;
    hsync_steps = 1;

    psxRcntSet();
}

/******************************************************************************/

s32 psxRcntFreeze( void *f, s32 Mode )
{
    u32 spuSyncCount = 0;
    u32 count;
    s32 i;

    gzfreeze( &rcnts, sizeof(Rcnt) * CounterQuantity );
    gzfreeze( &hSyncCount, sizeof(hSyncCount) );
    gzfreeze( &spuSyncCount, sizeof(spuSyncCount) );
    gzfreeze( &psxNextCounter, sizeof(psxNextCounter) );
    gzfreeze( &psxNextsCounter, sizeof(psxNextsCounter) );

    if (Mode == 0)
    {
        // don't trust things from a savestate
        rcnts[3].rate = 1;
        for( i = 0; i < CounterQuantity; ++i )
        {
            _psxRcntWmode( i, rcnts[i].mode );
            count = (psxRegs.cycle - rcnts[i].cycleStart) / rcnts[i].rate;
            _psxRcntWcount( i, count );
        }
        scheduleRcntBase();
        psxRcntSet();
    }

    return 0;
}

/******************************************************************************/
// vim:ts=4:shiftwidth=4:expandtab
