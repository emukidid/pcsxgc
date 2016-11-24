#ifndef _PSEMU_PHI
#define _PSEMU_PHI

/*
  ------------------------------

  please export following functions

  must fill all required fields
  PSECALLBACK void psepIdentify(HWND parent, LPPLUGININFO info);

  // this routine should not allocate memory or do sth stupid... just fill the fields in PHI
  PSECALLBACK long psepInitialize(LPPHI phi);

*/  

//#define PSECALLBACK __fastcall
#define PSECALLBACK __stdcall

typedef struct
{
	char	dirPSEmu[_MAX_PATH];	// PSEmu base dir
	char	dirSkins[_MAX_DIR];		// skins
	char	dirSnaps[_MAX_DIR];		// snapshots
	char	dirThumbs[_MAX_DIR];	// thumbnails
	char	dirBios[_MAX_DIR];		// bioses
	char	dirDevices[_MAX_DIR];	// devices
	
} PHICONFIG;



// those flags refers to display mode in PHI.GPU.display_type
#define PSE_GPU_DT_WINDOWED		0x1
// for WINDOWED use window_x and window_y
#define	PSE_GPU_DT_FULLSCREEN	0x0



typedef struct
{
	// this field is set by psemu, to force plugin to use specified display mode
	unsigned long display_type;

	// client are to draw
	unsigned short window_x, window_y, window_w, window_h;

	// psemu will call this routine whenever window size or windowed/fullscreen mode change
	long (PSECALLBACK *display_type_changed)(void);

	// this field is modified by plugin (then plugin calls display_mode_changed)
	volatile unsigned long display_mode;
	// this field is set by psemu, if you want to display, dont count fps
	volatile float fps;

	// plugin calls it when mode changed
	// *** this routine is set by psemu (no need to check if NULL) it will be set before init
	void (PSECALLBACK *display_mode_changed)(void);

	// on GPU interrupt... it happens sometime
	// *** this routine is set by psemu (no need to check if NULL) it will be set before init
	void (PSECALLBACK *interrupt)(void);

	// those following routines pointers must be set by plugin

	// initialize is to be called only once when psemu starts
	long (PSECALLBACK *initialize)(void);
	// shutdown is called when psemu crashes or exits
	// please keep in mind to close window and free resources, in case of crash, close() is not called
	void (PSECALLBACK *shutdown)(void);

	// used to open/close window... pliz, dont invalidate frame buffer or so
	long (PSECALLBACK *open)(void);
	void (PSECALLBACK *close)(void);

	// as a PSX reset :) 
	void (PSECALLBACK *reset)(void);

	// no need to explain now
	
	
	// note that in dma_xxxx functions, addr is a relative to PSX memory pointer (PHI.lpvPSMem)
	unsigned long (PSECALLBACK *dma_slice_in)(unsigned long addr, unsigned long size);
	unsigned long (PSECALLBACK *dma_slice_out)(unsigned long addr, unsigned long size);
	unsigned long (PSECALLBACK *dma_chain_in)(unsigned long addr);
	
	void (PSECALLBACK *data_in)(unsigned long data);
	unsigned long (PSECALLBACK *data_out)(void);
	void (PSECALLBACK *status_in)(unsigned long status);
	unsigned long (PSECALLBACK *status_out)(void);
	// vsync will be called on every vsync
	// if type is 0 ... just update screen if needed etc
	// if type is 1 .... wait for VSync of VGA card or whatever... 
	void (PSECALLBACK *vsync)(unsigned long type);


	
} PHIGPU;


typedef struct
{
	// no idea yet... but this will be used as a storage for different configs for different games...
	// we have to chat with Pete.. and his PSSwitch

	// more to be added soon
} PLUGINCONFIG;

typedef PLUGINCONFIG *LPPLUGINCONFIG;


// plugin information structure
// plugin must set all of the fields
typedef struct
{
	char			*name;
	char			*author;
	unsigned char	type;
	unsigned char	version;
	unsigned char	revision;
	unsigned char	psemu_version;
	unsigned long (PSECALLBACK *about)(void);
	unsigned long (PSECALLBACK *configure)(LPPLUGINCONFIG config);
} PLUGININFO;


typedef PLUGININFO *LPPLUGININFO;


// plugin types (types can be merged if plugin is multi type!)
#define PSE_LT_CDR					1
#define PSE_LT_GPU					2
#define PSE_LT_SPU					4
#define PSE_LT_PAD					8

typedef struct
{
} PHISPU;

typedef struct
{
} PHIPAD;

typedef struct
{
} PHICDR;


// predefined types for PHIGPU.displaymode;
// this field is to be set always by gpu, when display
// mode will change
#define PG_DM_LACE	0x1
#define PG_DM_NOLACE	0x0
#define PG_DM_PAL	0x2
#define PG_DM_NTSC	0x0
#define PG_DM_HIRES	0x4
#define PG_DM_LORES	0x0


typedef PHIGPU *LPPHIGPU;
typedef PHISPU *LPPHISPU;
typedef PHIPAD *LPPHIPAD;
typedef PHICDR *LPPHICDR;



typedef struct
{
	// pointer to PSX RAM
	void			*lpvPSMem;
	// mem size in bytes
	unsigned long	memsize;
		
	HWND psemu_hwnd;
	
	// use this to report errors, instead of calling AfxMessageBox()
	long (PSECALLBACK *pseMessageBox)(unsigned long type, char *fmt, ...);
	
	PHIGPU 	gpu;
	PHISPU 	spu;
	PHICDR 	cdr;
	PHIPAD 	pad;
} PHI;


typedef PHI *LPPHI;


#endif // _PSEMU_PHI
