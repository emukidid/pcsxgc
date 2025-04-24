/* MEM2.h - MEM2 boundaries for different chunks of memory
   by Mike Slegeir for Mupen64-Wii adapted for WiiSX by emu_kidid
 */

#ifndef MEM2_H
#define MEM2_H

// Define a MegaByte
#define KB (1024)
#define MB (1024*1024)

// MEM2 begins at MEM2_LO, the Starlet's Dedicated Memory begins at MEM2_HI
#define MEM2_LO   ((char*)0x90080000)
#define MEM2_HI   ((char*)0x933E0000)
#define MEM2_SIZE (MEM2_HI - MEM2_LO)

// We want 128KB for our MEMCARD 1
#define MCD1_SIZE     (128*KB)
#define MCD1_LO       (MEM2_LO)
#define MCD1_HI       (MCD1_LO + MCD1_SIZE)

// We want 128KB for our MEMCARD 2
#define MCD2_SIZE     (128*KB)
#define MCD2_LO       (MCD1_HI)
#define MCD2_HI       (MCD2_LO + MCD2_SIZE)

// We want 256KB for fontFont
#define FONT_SIZE (256*KB)
#define FONT_LO   (MCD2_HI)
#define FONT_HI   (FONT_LO + FONT_SIZE)

// We want 256KB for fontFont
#define FONTWORK_SIZE (128*KB)
#define FONTWORK_LO   (FONT_HI)
#define FONTWORK_HI   (FONTWORK_LO + FONTWORK_SIZE)

// 256KB for SIO Dongle data
#define SIODONGLE_SIZE (256*KB)
#define SIODONGLE_LO   (FONTWORK_HI)
#define SIODONGLE_HI   (SIODONGLE_LO + SIODONGLE_SIZE)

// 4MB for CD read-ahead data
#define CDREADAHEAD_SIZE (4*MB)
#define CDREADAHEAD_LO (SIODONGLE_HI)
#define CDREADAHEAD_HI (CDREADAHEAD_LO + CDREADAHEAD_SIZE)

// 32MB for Texture/texture meta
#define TEXCACHE_SIZE (32*MB)
#define TEXCACHE_LO   (CDREADAHEAD_HI)
#define TEXCACHE_HI   (TEXCACHE_LO + TEXCACHE_SIZE)

// 3MB for VSecure
#define VSECURE_SIZE (3*MB)
#define VSECURE_LO   (TEXCACHE_HI)
#define VSECURE_HI   (VSECURE_LO + VSECURE_SIZE)

#endif
