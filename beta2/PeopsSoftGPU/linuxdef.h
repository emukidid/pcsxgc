#ifndef _GNUDEF_H_
#define _GNUDEF_H_

// Type of Plugins returned by *_About functions.
#define FPSE_GPU    1
#define FPSE_SPU    2
#define FPSE_JOY0   3
#define FPSE_JOY1   4
#define FPSE_CDROM  5
#define FPSE_PAR    6

// Bit masks for Flags field
#define GPU_USE_DIB_UPDATE      0x00000001
#define GPU_USE_NEW_MDEC        0x00000002

// New MDEC from GPU plugin.
typedef struct {
    int     (*MDEC0_Read)();
    int     (*MDEC0_Write)();
    int     (*MDEC1_Read)();
    int     (*MDEC1_Write)();
    int     (*MDEC0_DmaExec)();
    int     (*MDEC1_DmaExec)();
} MDEC_Export;

// Main Struct for initialization
typedef struct {
    UINT8        *SystemRam;   // Pointer to the PSX system ram
    UINT32        Flags;       // Flags to plugins
    UINT32       *IrqPulsePtr; // Pointer to interrupt pending reg
    MDEC_Export   MDecAltern;  // Use another MDEC engine
    int         (*ReadCfg)();  // Read an item from INI
    int         (*WriteCfg)(); // Write an item to INI
    void        (*FlushRec)(); // Tell where the RAM is changed
} FPSElinux;

// Info about a plugin
typedef struct {
    UINT8   PlType;             // Plugin type: GPU, SPU or Controllers
    UINT8   VerLo;              // Version High
    UINT8   VerHi;              // Version Low
    UINT8   TestResult;         // Returns if it'll work or not
    char    Author[64];         // Name of the author
    char    Name[64];           // Name of plugin
    char    Description[1024];  // Description to put in the edit box
} FPSElinuxAbout;

#endif
