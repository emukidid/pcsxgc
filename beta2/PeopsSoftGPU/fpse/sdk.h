#ifndef _SDK_H_
#define	_SDK_H_

typedef void (*FPSE_CallBack_Type)(void);

// CDROM section
//------------------------------------------------------------

UINT8 *CD_Read(UINT8 *param);
int    CD_Play(UINT8 *param);
int    CD_Stop(void);
int    CD_GetTD(UINT8 *result,int track);
int    CD_GetTN(UINT8 *result);
int    CD_Open(UINT32 *par);
void   CD_Close(void);
int    CD_Wait(void);

// High level functions
int    CD_Configure(UINT32 *par);
void   CD_About(UINT32 *par);


// GPU section
//------------------------------------------------------------

typedef struct {
    int dummy;
} GPU_State;

int    GPU_Open(UINT32 *gpu);
void   GPU_Close(void);

UINT32 GP0_Read(void);
UINT32 GP1_Read(void);
void   GP0_Write(UINT32 data);
void   GP1_Write(UINT32 code);

void   GPU_Update(void);

void   GPU_DmaExec(UINT32 adr,UINT32 bcr,UINT32 chcr);

void   GPU_ScreenShot(char *path);

int    GPU_Configure(UINT32 *par);
void   GPU_About(UINT32 *par);

void   GPU_LoadState(GPU_State *state);
void   GPU_SaveState(GPU_State *state);

// SPU section
//------------------------------------------------------------

typedef struct {
    int dummy;
} SPU_State;

int  SPU_Open(UINT32 *spu);
void SPU_Close(void);

int  SPU_Read(UINT32 adr);
void SPU_Write(UINT32 adr, unsigned int data);

void SPU_DmaExec(UINT32 adr,UINT32 bcr,UINT32 chcr);

int  SPU_Configure(UINT32 *par);
void SPU_About(UINT32 *par);

void SPU_PlayStream(INT16 *XAsampleBuf, int freq, int chns);

void SPU_LoadState(SPU_State *state);
void SPU_SaveState(SPU_State *state);

FPSE_CallBack_Type SPU_GetCallBack(void);

// CONTROLLERS section
//------------------------------------------------------------

typedef struct {
    int dummy;
} JOY_State;

#define ACK_OK	1
#define ACK_ERR 0

int  JOY0_Open(UINT32 *joy);
void JOY0_Close(void);
void JOY0_SetOutputBuffer(UINT8 *buf);
int  JOY0_StartPoll(void);
int  JOY0_Poll(int outbyte);
void JOY0_LoadState(JOY_State *state);
void JOY0_SaveState(JOY_State *state);
int  JOY0_Configure(UINT32 *par);
void JOY0_About(UINT32 *par);

int  JOY1_Open(UINT32 *joy);
void JOY1_Close(void);
void JOY1_SetOutputBuffer(UINT8 *buf);
int  JOY1_StartPoll(void);
int  JOY1_Poll(int outbyte);
void JOY1_LoadState(JOY_State *state);
void JOY1_SaveState(JOY_State *state);
int  JOY1_Configure(UINT32 *par);
void JOY1_About(UINT32 *par);


// PARALLEL port section
//------------------------------------------------------------

typedef struct {
    int dummy;
} PAR_State;

int  PAR_Open(UINT32 *par);
void PAR_Close(void);

int  PAR_Read(UINT32 adr);
void PAR_Write(UINT32 adr, unsigned int data);

void PAR_DmaExec(UINT32 adr,UINT32 bcr,UINT32 chcr);

int  PAR_Configure(UINT32 *par);
void PAR_About(UINT32 *par);

void PAR_LoadState(PAR_State *state);
void PAR_SaveState(PAR_State *state);

FPSE_CallBack_Type PAR_GetCallBack(void);

#endif
