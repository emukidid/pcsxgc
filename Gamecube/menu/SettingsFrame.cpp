/**
 * WiiSX - SettingsFrame.cpp
 * Copyright (C) 2009, 2010 sepp256
 *
 * WiiSX homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/

#include "MenuContext.h"
#include "SettingsFrame.h"
#include "../libgui/Button.h"
#include "../libgui/TextBox.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"
#include "../wiiSXconfig.h"

#include <libpcsxcore/psxcommon.h>

extern "C" {
#include "../../pcsx_rearmed/plugins/dfsound/spu_config.h"
#include "../gc_input/controller.h"
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "../fileBrowser/fileBrowser-CARD.h"
}

extern void Func_SetPlayGame();

void Func_TabGeneral();
void Func_TabVideo();
void Func_TabInput();
void Func_TabAudio();
void Func_TabSaves();

void Func_CpuInterp();
void Func_CpuDynarec();
void Func_BiosSelectHLE();
void Func_BiosSelectSD();
void Func_BiosSelectUSB();
void Func_BiosSelectDVD();
void Func_BootBiosYes();
void Func_BootBiosNo();
void Func_ExecuteBios();
void Func_SaveSettingsSD();
void Func_SaveSettingsUSB();

void Func_ShowFpsOn();
void Func_ShowFpsOff();
void Func_FpsLimitAuto();
void Func_FpsLimitOff();
void Func_FrameSkipOn();
void Func_FrameSkipOff();
void Func_ScreenMode4_3();
void Func_ScreenMode16_9();
void Func_ScreenForce16_9();
void Func_DitheringNone();
void Func_DitheringDefault();
void Func_DitheringAlways();

void Func_ConfigureInput();
void Func_ConfigureButtons();
void Func_PsxTypeStandard();
void Func_PsxTypeAnalog();
void Func_PsxTypeLightgun();
void Func_DisableRumbleYes();
void Func_DisableRumbleNo();
void Func_SaveButtonsSD();
void Func_SaveButtonsUSB();
void Func_ToggleButtonLoad();

void Func_DisableAudioYes();
void Func_DisableAudioNo();
void Func_DisableReverbYes();
void Func_DisableReverbNo();
void Func_VolumeToggle();

void Func_MemcardSaveSD();
void Func_MemcardSaveUSB();
void Func_SaveStateSD();
void Func_SaveStateUSB();

void Func_ReturnFromSettingsFrame();

extern BOOL hasLoadedISO;
extern int stop;
extern char menuActive;

extern "C" {
void SysReset();
int SysInit();
void SysClose();
void SysStartCPU();
void CheckCdrom();
void LoadCdrom();
void pauseAudio(void);  void pauseInput(void);
void resumeAudio(void); void resumeInput(void);
}

#define NUM_FRAME_BUTTONS 46
#define NUM_TAB_BUTTONS 5
#define NUM_FRAME_TEXTBOXES 18

/*
General Tab:
Select CPU Core: Interpreter; Dynarec
Select Bios: HLE; SD; USB
Boot Games Through Bios: Yes; No
Execute Bios
Save settings.cfg: SD; USB

Video Tab:
Show FPS: Yes; No
Limit FPS: Auto; Off; xxx
Frame Skip: On; Off
Screen Mode: 4:3; 16:9
Dithering: None; Game Dependent; Always

Input Tab:
Assign Controllers (assign player->pad)
Configure Button Mappings
PSX Controller Type: Standard/Analog/Gun
//Number of Multitaps: 0, 1, 2

Audio Tab:
Disable Audio: Yes; No
Volume Level: ("0: low", "1: medium", "2: loud", "3: loudest")
	Note: volume=4-ComboBox_GetCurSel(hWC);, so 1 is loudest and 4 is low; default is medium
Disable Reverb: Yes; No

Saves Tab:
Memcard Save Device: SD; USB
Save States Device: SD; USB
*/

enum BUTTON_IDS {
	BTN_TAB_GENERAL,
	BTN_TAB_VIDEO,
	BTN_TAB_INPUT,
	BTN_TAB_AUDIO,
	BTN_TAB_SAVES,
	
	BTN_INTERP,
	BTN_DYNAREC,
	BTN_HLE,
	BTN_SD,
	BTN_USB,
	BTN_BIOS_YES,
	BTN_BIOS_NO,
	BTN_EXEC_BIOS,
	BTN_SAVE_SETTING_SD,
	BTN_SAVE_SETTING_USB,
	
	BTN_SHOW_FPS_ON,
	BTN_SHOW_FPS_OFF,
	BTN_LIMIT_FPS_ON,
	BTN_LIMIT_FPS_OFF,
	BTN_FSKIP_ON,
	BTN_FSKIP_OFF,
	BTN_SM_4_3,
	BTN_SM_16_9,
	BTN_SM_F_16_9,
	BTN_DITHER_NONE,
	BTN_DITHER_DEFAULT,
	BTN_DITHER_ALWAYS,
	
	BTN_CONF_INPUT,
	BTN_CONF_BTN_MAP,
	BTN_PAD_STANDARD,
	BTN_PAD_ANALOG,
	BTN_PAD_GUN,
	BTN_RUMBLE_YES,
	BTN_RUMBLE_NO,
	BTN_SAVE_BTN_MAP_SD,
	BTN_SAVE_BTN_MAP_USB,
	BTN_AUTOLOAD_BTN_CONF,
	
	BTN_DISABLE_AUDIO_YES,
	BTN_DISABLE_AUDIO_NO,
	BTN_DISABLE_REVERB_YES,
	BTN_DISABLE_REVERB_NO,
	BTN_VOLUME,
	
	BTN_MEMCARD_SD,
	BTN_MEMCARD_USB,
	BTN_SSTATE_SD,
	BTN_SSTATE_USB,
	
	BTNS_END
};

struct LabelResources
{
	int	key;
	char* def_val;
} RES_BTN_LBL[BTNS_END] =
{
	{BTN_TAB_GENERAL, "General"},
	{BTN_TAB_VIDEO, "Video"},
	{BTN_TAB_INPUT, "Input"},
	{BTN_TAB_AUDIO, "Audio"},
	{BTN_TAB_SAVES, "Saves"},
	
	{BTN_INTERP, "Interpreter"},
	{BTN_DYNAREC, "Dynarec"},
	{BTN_HLE, "HLE"},
	{BTN_SD, "SD"},
	{BTN_USB, "USB"},
	{BTN_BIOS_YES, "Yes"},
	{BTN_BIOS_NO, "No"},
	{BTN_EXEC_BIOS, "Execute BIOS"},
	{BTN_SAVE_SETTING_SD, "SD"},
	{BTN_SAVE_SETTING_USB, "USB"},

	
	{BTN_SHOW_FPS_ON, "On"},
	{BTN_SHOW_FPS_OFF, "Off"},
	{BTN_LIMIT_FPS_ON, "Auto"},
	{BTN_LIMIT_FPS_OFF, "Off"},
	{BTN_FSKIP_ON, "On"},
	{BTN_FSKIP_OFF, "Off"},
	{BTN_SM_4_3, "4:3"},
	{BTN_SM_16_9, "16:9"},
	{BTN_SM_F_16_9, "Force 16:9"},
	{BTN_DITHER_NONE, "None"},
	{BTN_DITHER_DEFAULT, "Default"},
	{BTN_DITHER_ALWAYS, "Always"},
	
	{BTN_CONF_INPUT, "Configure Input Assignment"},
	{BTN_CONF_BTN_MAP, "Configure Button Mappings"},
	{BTN_PAD_STANDARD, "Standard"},
	{BTN_PAD_ANALOG, "Analog"},
	{BTN_PAD_GUN, "Gun"},
	{BTN_RUMBLE_YES, "Yes"},
	{BTN_RUMBLE_NO, "No"},
	{BTN_SAVE_BTN_MAP_SD, "SD"},
	{BTN_SAVE_BTN_MAP_USB, "USB"},
	{BTN_AUTOLOAD_BTN_CONF, "Auto Load Slot:"},
	
	{BTN_DISABLE_AUDIO_YES, "Yes"},
	{BTN_DISABLE_AUDIO_NO, "No"},
	{BTN_DISABLE_REVERB_YES, "Yes"},
	{BTN_DISABLE_REVERB_NO, "No"},
	{BTN_VOLUME, "Volume"},
	
	{BTN_MEMCARD_SD, "SD"},
	{BTN_MEMCARD_USB, "USB"},
	{BTN_SSTATE_SD, "SD"},
	{BTN_SSTATE_USB, "USB"}
};

enum TAB_IDS {
	TAB_NONE,
	TAB_GENERAL,
	TAB_VIDEO,
	TAB_INPUT,
	TAB_AUDIO,
	TAB_SAVES
};

enum LABEL_IDS {
	// Tabs
	LBL_TAB,
	// General
	LBL_GENERAL_CPU,
	LBL_GENERAL_BIOS,
	LBL_GENERAL_BOOT_BIOS,
	LBL_GENERAL_EXEC_BIOS,
	LBL_GENERAL_SAVE,
	// Video
	LBL_VIDEO_FPS_SHOW,
	LBL_VIDEO_FPS_LIMIT,
	LBL_VIDEO_FRAMESKIP,
	LBL_VIDEO_SCREENMODE,
	LBL_VIDEO_DITHER,
	// Input
	LBL_INPUT_CONF_INPUT,
	LBL_INPUT_CONF_BTNS,
	LBL_INPUT_CONT_TYPE,
	LBL_INPUT_RUMBLE,
	LBL_INPUT_SAVE,
	LBL_INPUT_AUTOLOAD,
	// Audio
	LBL_AUDIO_DISABLE,
	LBL_AUDIO_REVERB,
	LBL_AUDIO_VOLUME,
	// Saves tab
	LBL_SAVES_NATIVE_LOC,
	LBL_SAVES_STATE_LOC,
	LABEL_GROUPS_END
};

LabelResources RES_LBL[LABEL_GROUPS_END] =
{
	{LBL_TAB, "Tab"},
	// General
	{LBL_GENERAL_CPU, "Select CPU Core"},
	{LBL_GENERAL_BIOS, "Select BIOS"},
	{LBL_GENERAL_BOOT_BIOS, "Boot Through BIOS"},
	{LBL_GENERAL_EXEC_BIOS, "Execute BIOS"},
	{LBL_GENERAL_SAVE, "Save settings.cfg"},
	// Video
	{LBL_VIDEO_FPS_SHOW, "Show FPS"},
	{LBL_VIDEO_FPS_LIMIT, "Limit FPS"},
	{LBL_VIDEO_FRAMESKIP, "Frame Skip"},
	{LBL_VIDEO_SCREENMODE, "Screen Mode"},
	{LBL_VIDEO_DITHER, "Dithering"},
	// Input
	{LBL_INPUT_CONF_INPUT, "Configure Input"},
	{LBL_INPUT_CONF_BTNS, "Configure Buttons"},
	{LBL_INPUT_CONT_TYPE, "PSX Controller Type"},
	{LBL_INPUT_RUMBLE, "Disable Rumble"},
	{LBL_INPUT_SAVE, "Save Button Configs"},
	{LBL_INPUT_AUTOLOAD, "Auto Load Slot:"},
	// Audio
	{LBL_AUDIO_DISABLE, "Disable Audio"},
	{LBL_AUDIO_REVERB, "Disable Reverb"},
	{LBL_AUDIO_VOLUME, "Volume"},
	// Saves tab
	{LBL_SAVES_NATIVE_LOC, "Memcard Save Device"},
	{LBL_SAVES_STATE_LOC, "Save States Device"}
};

struct SettingsButtonInfo
{
	menu::Button	*button;
	int				buttonStyle;
	float			x;
	float			y;
	float			width;
	float			height;
	int				focusUp;
	int				focusDown;
	int				focusLeft;
	int				focusRight;
	ButtonFunc		clickedFunc;
	int				tabGrpId;
	int				lblGrpId;	// Label/button group. Used for labelling but also for button selections.
	int				btnId;
} FRAME_BUTTONS[NUM_FRAME_BUTTONS] =
{ //	button	buttonStyle x		y		width	height	Up	Dwn	Lft	Rt	clickFunc				
	//Buttons for Tabs
	{	NULL,	BTN_A_SEL,	 25.0,	 30.0,	110.0,	56.0,	-1,	-1,	 4,	 1,	Func_TabGeneral,		TAB_NONE, LBL_TAB, BTN_TAB_GENERAL }, // General tab
	{	NULL,	BTN_A_SEL,	155.0,	 30.0,	100.0,	56.0,	-1,	-1,	 0,	 2,	Func_TabVideo,			TAB_NONE, LBL_TAB, BTN_TAB_VIDEO }, // Video tab
	{	NULL,	BTN_A_SEL,	275.0,	 30.0,	100.0,	56.0,	-1,	-1,	 1,	 3,	Func_TabInput,			TAB_NONE, LBL_TAB, BTN_TAB_INPUT }, // Input tab
	{	NULL,	BTN_A_SEL,	395.0,	 30.0,	100.0,	56.0,	-1,	-1,	 2,	 4,	Func_TabAudio,			TAB_NONE, LBL_TAB, BTN_TAB_AUDIO }, // Audio tab
	{	NULL,	BTN_A_SEL,	515.0,	 30.0,	100.0,	56.0,	-1,	-1,	 3,	 0,	Func_TabSaves,			TAB_NONE, LBL_TAB, BTN_TAB_SAVES }, // Saves tab
	//Buttons for General Tab
	{	NULL,	BTN_A_SEL,	295.0,	100.0,	160.0,	56.0,	 0,	 7,	 6,	 6,	Func_CpuInterp,			TAB_GENERAL, LBL_GENERAL_CPU, BTN_INTERP }, // CPU: Interp
	{	NULL,	BTN_A_SEL,	465.0,	100.0,	130.0,	56.0,	 0,	 9,	 5,	 5,	Func_CpuDynarec,		TAB_GENERAL, LBL_GENERAL_CPU, BTN_DYNAREC }, // CPU: Dynarec
	{	NULL,	BTN_A_SEL,	295.0,	170.0,	 70.0,	56.0,	 5,	11,	10,	 8,	Func_BiosSelectHLE,		TAB_GENERAL, LBL_GENERAL_BIOS, BTN_HLE }, // Bios: HLE
	{	NULL,	BTN_A_SEL,	375.0,	170.0,	 55.0,	56.0,	 5,	12,	 7,	 9,	Func_BiosSelectSD,		TAB_GENERAL, LBL_GENERAL_BIOS, BTN_SD }, // Bios: SD
	{	NULL,	BTN_A_SEL,	440.0,	170.0,	 70.0,	56.0,	 6,	12,	 8,	10,	Func_BiosSelectUSB,		TAB_GENERAL, LBL_GENERAL_BIOS, BTN_USB }, // Bios: USB
	{	NULL,	BTN_A_SEL,	295.0,	240.0,	 75.0,	56.0,	 7,	13,	12,	12,	Func_BootBiosYes,		TAB_GENERAL, LBL_GENERAL_BOOT_BIOS, BTN_BIOS_YES }, // Boot Thru Bios: Yes
	{	NULL,	BTN_A_SEL,	380.0,	240.0,	 75.0,	56.0,	 8,	13,	11,	11,	Func_BootBiosNo,		TAB_GENERAL, LBL_GENERAL_BOOT_BIOS, BTN_BIOS_NO }, // Boot Thru Bios: No
	{	NULL,	BTN_A_NRM,	295.0,	310.0,	200.0,	56.0,	11,	14,	-1,	-1,	Func_ExecuteBios,		TAB_GENERAL, LBL_GENERAL_EXEC_BIOS, BTN_EXEC_BIOS }, // Execute Bios
	{	NULL,	BTN_A_NRM,	295.0,	380.0,	 55.0,	56.0,	13,	 0,	15,	15,	Func_SaveSettingsSD,	TAB_GENERAL, LBL_GENERAL_SAVE, BTN_SAVE_SETTING_SD }, // Save Settings: SD
	{	NULL,	BTN_A_NRM,	360.0,	380.0,	 70.0,	56.0,	13,	 0,	14,	14,	Func_SaveSettingsUSB,	TAB_GENERAL, LBL_GENERAL_SAVE, BTN_SAVE_SETTING_USB }, // Save Settings: USB
	//Buttons for Video Tab
	{	NULL,	BTN_A_SEL,	325.0,	100.0,	 75.0,	56.0,	 1,	18,	17,	17,	Func_ShowFpsOn,			TAB_VIDEO, LBL_VIDEO_FPS_SHOW, BTN_SHOW_FPS_ON }, // Show FPS: On
	{	NULL,	BTN_A_SEL,	420.0,	100.0,	 75.0,	56.0,	 1,	19,	16,	16,	Func_ShowFpsOff,		TAB_VIDEO, LBL_VIDEO_FPS_SHOW, BTN_SHOW_FPS_OFF }, // Show FPS: Off
	{	NULL,	BTN_A_SEL,	325.0,	170.0,	 75.0,	56.0,	16,	20,	19,	19,	Func_FpsLimitAuto,		TAB_VIDEO, LBL_VIDEO_FPS_LIMIT, BTN_LIMIT_FPS_ON }, // FPS Limit: Auto
	{	NULL,	BTN_A_SEL,	420.0,	170.0,	 75.0,	56.0,	17,	21,	18,	18,	Func_FpsLimitOff,		TAB_VIDEO, LBL_VIDEO_FPS_LIMIT, BTN_LIMIT_FPS_OFF }, // FPS Limit: Off
	{	NULL,	BTN_A_SEL,	325.0,	240.0,	 75.0,	56.0,	18,	23,	21,	21,	Func_FrameSkipOn,		TAB_VIDEO, LBL_VIDEO_FRAMESKIP, BTN_FSKIP_ON }, // Frame Skip: On
	{	NULL,	BTN_A_SEL,	420.0,	240.0,	 75.0,	56.0,	19,	24,	20,	20,	Func_FrameSkipOff,		TAB_VIDEO, LBL_VIDEO_FRAMESKIP, BTN_FSKIP_OFF }, // Frame Skip: Off
	{	NULL,	BTN_A_SEL,	230.0,	310.0,	 75.0,	56.0,	20,	25,	24,	23,	Func_ScreenMode4_3,		TAB_VIDEO, LBL_VIDEO_SCREENMODE, BTN_SM_4_3 }, // ScreenMode: 4:3
	{	NULL,	BTN_A_SEL,	325.0,	310.0,	 75.0,	56.0,	20,	26,	22,	24,	Func_ScreenMode16_9,	TAB_VIDEO, LBL_VIDEO_SCREENMODE, BTN_SM_16_9 }, // ScreenMode: 16:9
	{	NULL,	BTN_A_SEL,	420.0,	310.0,	155.0,	56.0,	21,	27,	23,	22,	Func_ScreenForce16_9,	TAB_VIDEO, LBL_VIDEO_SCREENMODE, BTN_SM_F_16_9 }, // ScreenMode: Force 16:9
	{	NULL,	BTN_A_SEL,	230.0,	380.0,	 75.0,	56.0,	22,	 1,	27,	26,	Func_DitheringNone,		TAB_VIDEO, LBL_VIDEO_DITHER, BTN_DITHER_NONE }, // Dithering: None
	{	NULL,	BTN_A_SEL,	325.0,	380.0,	110.0,	56.0,	23,	 1,	25,	27,	Func_DitheringDefault,	TAB_VIDEO, LBL_VIDEO_DITHER, BTN_DITHER_DEFAULT }, // Dithering: Game Dependent
	{	NULL,	BTN_A_SEL,	455.0,	380.0,	110.0,	56.0,	24,	 1,	26,	25,	Func_DitheringAlways,	TAB_VIDEO, LBL_VIDEO_DITHER, BTN_DITHER_NONE }, // Dithering: Always
	//Buttons for Input Tab
	{	NULL,	BTN_A_NRM,	 90.0,	100.0,	220.0,	56.0,	 2,	32,	31,	31,	Func_ConfigureInput,	TAB_INPUT, LBL_INPUT_CONF_INPUT, BTN_CONF_INPUT }, // Configure Input Assignment
	{	NULL,	BTN_A_NRM,	325.0,	100.0,	235.0,	56.0,	 2,	32,	30,	30,	Func_ConfigureButtons,	TAB_INPUT, LBL_INPUT_CONF_BTNS, BTN_CONF_BTN_MAP }, // Configure Button Mappings
	{	NULL,	BTN_A_SEL,	285.0,	170.0,	130.0,	56.0,	30,	34,	56,	33,	Func_PsxTypeStandard,	TAB_INPUT, LBL_INPUT_CONT_TYPE, BTN_PAD_STANDARD }, // PSX Controller Type: Standard
	{	NULL,	BTN_A_SEL,	425.0,	170.0,	110.0,	56.0,	31,	35,	32,	56,	Func_PsxTypeAnalog,		TAB_INPUT, LBL_INPUT_CONT_TYPE, BTN_PAD_ANALOG }, // PSX Controller Type: Analog
	{	NULL,	BTN_A_SEL,	550.0,	170.0,	 75.0,	56.0,	31,	35,	33,	32,	Func_PsxTypeLightgun,	TAB_INPUT, LBL_INPUT_CONT_TYPE, BTN_PAD_GUN }, // PSX Controller Type: Gun
	{	NULL,	BTN_A_SEL,	285.0,	240.0,	 75.0,	56.0,	32,	36,	35,	35,	Func_DisableRumbleYes,	TAB_INPUT, LBL_INPUT_RUMBLE, BTN_RUMBLE_YES }, // Disable Rumble: Yes
	{	NULL,	BTN_A_SEL,	380.0,	240.0,	 75.0,	56.0,	33,	37,	34,	34,	Func_DisableRumbleNo,	TAB_INPUT, LBL_INPUT_RUMBLE, BTN_RUMBLE_NO }, // Disable Rumble: No
	{	NULL,	BTN_A_NRM,	285.0,	310.0,	 55.0,	56.0,	34,	38,	37,	37,	Func_SaveButtonsSD,		TAB_INPUT, LBL_INPUT_SAVE, BTN_SAVE_BTN_MAP_SD }, // Save Button Mappings: SD
	{	NULL,	BTN_A_NRM,	350.0,	310.0,	 70.0,	56.0,	35,	38,	36,	36,	Func_SaveButtonsUSB,	TAB_INPUT, LBL_INPUT_SAVE, BTN_SAVE_BTN_MAP_USB }, // Save Button Mappings: USB
	{	NULL,	BTN_A_NRM,	285.0,	380.0,	135.0,	56.0,	36,	 2,	-1,	-1,	Func_ToggleButtonLoad,	TAB_INPUT, LBL_INPUT_AUTOLOAD, BTN_AUTOLOAD_BTN_CONF }, // Auto Load Button Config Slot: Default,1,2,3,4
	//Buttons for Audio Tab
	{	NULL,	BTN_A_SEL,	345.0,	100.0,	 75.0,	56.0,	 3,	41,	40,	40,	Func_DisableAudioYes,	TAB_AUDIO, LBL_AUDIO_DISABLE, BTN_DISABLE_AUDIO_YES }, // Disable Audio: Yes
	{	NULL,	BTN_A_SEL,	440.0,	100.0,	 75.0,	56.0,	 3,	42,	39,	39,	Func_DisableAudioNo,	TAB_AUDIO, LBL_AUDIO_DISABLE, BTN_DISABLE_AUDIO_NO }, // Disable Audio: No
	{	NULL,	BTN_A_SEL,	345.0,	380.0,	 75.0,	56.0,	45,	3,	55,	55,	Func_DisableReverbYes,	TAB_AUDIO, LBL_AUDIO_REVERB, BTN_DISABLE_REVERB_YES }, // Disable Reverb: Yes
	{	NULL,	BTN_A_SEL,	440.0,	380.0,	 75.0,	56.0,	45,	3,	54,	54,	Func_DisableReverbNo,	TAB_AUDIO, LBL_AUDIO_REVERB, BTN_DISABLE_REVERB_NO }, // Disable Reverb: No
	{	NULL,	BTN_A_NRM,	345.0,	310.0,	170.0,	56.0,	43,	54,	-1,	-1,	Func_VolumeToggle,		TAB_AUDIO, LBL_AUDIO_VOLUME, BTN_VOLUME }, // Volume: low/medium/loud/loudest
	//Buttons for Saves Tab
	{	NULL,	BTN_A_SEL,	295.0,	100.0,	 55.0,	56.0,	 4,	50,	49,	47,	Func_MemcardSaveSD,		TAB_SAVES, LBL_SAVES_NATIVE_LOC, BTN_MEMCARD_SD }, // Memcard Save: SD
	{	NULL,	BTN_A_SEL,	360.0,	100.0,	 70.0,	56.0,	 4,	51,	46,	48,	Func_MemcardSaveUSB,	TAB_SAVES, LBL_SAVES_NATIVE_LOC, BTN_MEMCARD_USB }, // Memcard Save: USB
	{	NULL,	BTN_A_SEL,	295.0,	240.0,	 55.0,	56.0,	50,	 4,	53,	53,	Func_SaveStateSD,		TAB_SAVES, LBL_SAVES_STATE_LOC, BTN_SSTATE_SD }, // Save State: SD
	{	NULL,	BTN_A_SEL,	360.0,	240.0,	 70.0,	56.0,	51,	 4,	52,	52,	Func_SaveStateUSB,		TAB_SAVES, LBL_SAVES_STATE_LOC, BTN_SSTATE_USB }, // Save State: USB
};

struct SettingsTextBoxInfo
{
	menu::TextBox	*textBox;
	float			x;
	float			y;
	int				tabGrpId;
	int				lblGrpId;
} FRAME_TEXTBOXES[NUM_FRAME_TEXTBOXES] =
{ //	textBox	x		y		tabGrpId		lblGrpId
	//TextBoxes for General Tab
	{	NULL,	155.0,	128.0,	TAB_GENERAL,	LBL_GENERAL_CPU }, // CPU Core: Pure Interp/Dynarec
	{	NULL,	155.0,	198.0,	TAB_GENERAL,	LBL_GENERAL_BIOS }, // Bios: HLE/SD/USB/DVD
	{	NULL,	155.0,	268.0,	TAB_GENERAL,	LBL_GENERAL_BOOT_BIOS }, // Boot Thru Bios: Yes/No
	{	NULL,	155.0,	408.0,	TAB_GENERAL,	LBL_GENERAL_SAVE }, // Save settings.cfg: SD/USB
	//TextBoxes for Video Tab 	
	{	NULL,	190.0,	128.0,	TAB_VIDEO,		LBL_VIDEO_FPS_SHOW }, // Show FPS: On/Off
	{	NULL,	190.0,	198.0,	TAB_VIDEO,		LBL_VIDEO_FPS_LIMIT }, // Limit FPS: Auto/Off
	{	NULL,	190.0,	268.0,	TAB_VIDEO,		LBL_VIDEO_FRAMESKIP }, // Frame Skip: On/Off
	{	NULL,	130.0,	338.0,	TAB_VIDEO,		LBL_VIDEO_SCREENMODE }, // ScreenMode: 4x3/16x9/Force16x9
	{	NULL,	130.0,	408.0,	TAB_VIDEO,		LBL_VIDEO_DITHER }, // Dithering: None/Game Dependent/Always
	//TextBoxes for Input Tab 	
	{	NULL,	145.0,	198.0,	TAB_INPUT,		LBL_INPUT_CONT_TYPE }, // PSX Controller Type: Analog/Digital/Gun
	{	NULL,	145.0,	268.0,	TAB_INPUT,		LBL_INPUT_RUMBLE }, // Disable Rumble: Yes/No
	{	NULL,	145.0,	338.0,	TAB_INPUT,		LBL_INPUT_SAVE }, // Save Button Configs: SD/USB
	{	NULL,	145.0,	408.0,	TAB_INPUT,		LBL_INPUT_AUTOLOAD }, // Auto Load Slot: Default/1/2/3/4
	//TextBoxes for Audio Tab 	
	{	NULL,	210.0,	128.0,	TAB_AUDIO,		LBL_AUDIO_DISABLE }, // Disable Audio: Yes/No
	{	NULL,	210.0,	338.0,	TAB_AUDIO,		LBL_AUDIO_VOLUME }, // Volume: low/medium/loud/loudest
	{	NULL,	210.0,	408.0,	TAB_AUDIO,		LBL_AUDIO_REVERB }, // Disable Reverb: Yes/No
	//TextBoxes for Saves Tab 	
	{	NULL,	150.0,	128.0,	TAB_SAVES,		LBL_SAVES_NATIVE_LOC }, // Memcard Save Device: SD/USB
	{	NULL,	150.0,	268.0,	TAB_SAVES,		LBL_SAVES_STATE_LOC }, // Save State Device: SD/USB
};

SettingsFrame::SettingsFrame()
		: activeSubmenu(SUBMENU_GENERAL)
{
	// Create buttons from the struct above.
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++) {
		// Look up the label from the bundle
		for (int j = 0; j < BTNS_END; j++) {
			if(RES_BTN_LBL[j].key == FRAME_BUTTONS[i].btnId) {
				FRAME_BUTTONS[i].button = new menu::Button(FRAME_BUTTONS[i].buttonStyle, &RES_BTN_LBL[j].def_val, 
										FRAME_BUTTONS[i].x, FRAME_BUTTONS[i].y, 
										FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].height);
				break;
			}
		}
	
		
	}
	
	// Configure buttons (focus, functions)
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		if (FRAME_BUTTONS[i].focusUp != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[FRAME_BUTTONS[i].focusUp].button);
		if (FRAME_BUTTONS[i].focusDown != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[FRAME_BUTTONS[i].focusDown].button);
		if (FRAME_BUTTONS[i].focusLeft != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_LEFT, FRAME_BUTTONS[FRAME_BUTTONS[i].focusLeft].button);
		if (FRAME_BUTTONS[i].focusRight != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_RIGHT, FRAME_BUTTONS[FRAME_BUTTONS[i].focusRight].button);
		FRAME_BUTTONS[i].button->setActive(true);
		if (FRAME_BUTTONS[i].clickedFunc) FRAME_BUTTONS[i].button->setClicked(FRAME_BUTTONS[i].clickedFunc);
		FRAME_BUTTONS[i].button->setReturn(Func_ReturnFromSettingsFrame);
		add(FRAME_BUTTONS[i].button);
		menu::Cursor::getInstance().addComponent(this, FRAME_BUTTONS[i].button, FRAME_BUTTONS[i].x, 
												FRAME_BUTTONS[i].x+FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].y, 
												FRAME_BUTTONS[i].y+FRAME_BUTTONS[i].height);
	}

	// Create labels from the struct above.
	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
	{
		// Look up the label from the bundle
		for (int j = 0; j < LABEL_GROUPS_END; j++) {
			if(RES_LBL[j].key == FRAME_TEXTBOXES[i].lblGrpId) {
				FRAME_TEXTBOXES[i].textBox = new menu::TextBox(&RES_LBL[j].def_val, FRAME_TEXTBOXES[i].x, FRAME_TEXTBOXES[i].y, 1.0, true);
				add(FRAME_TEXTBOXES[i].textBox);
				break;
			}
		}		
	}

	setDefaultFocus(FRAME_BUTTONS[0].button);
	setBackFunc(Func_ReturnFromSettingsFrame);
	setEnabled(true);
	activateSubmenu(SUBMENU_GENERAL);
}

SettingsFrame::~SettingsFrame()
{
	// Destroy all button/label instances.
	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
		delete FRAME_TEXTBOXES[i].textBox;
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}
}

// Selects a specific button in a group of buttons sharing a label/group, deselects the rest in that group.
void SelectBtnInGroup(int curBtnId, int lblGrpId) {
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++) {
		if(FRAME_BUTTONS[i].lblGrpId == lblGrpId) {
			FRAME_BUTTONS[i].button->setSelected(FRAME_BUTTONS[i].btnId == curBtnId);
		}
	}
}

// Selects a button
void SelectBtn(int curBtnId) {
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++) {
		if(FRAME_BUTTONS[i].btnId == curBtnId) {
			FRAME_BUTTONS[i].button->setSelected(true);
			return;
		}
	}
}

// Sets the labels to visible
void SetVisibleLabelsForTab(int tabGrpId) {
	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++) {
		if(FRAME_TEXTBOXES[i].tabGrpId == tabGrpId) {
			FRAME_TEXTBOXES[i].textBox->setVisible(true);
		}
	}
}

// Sets the buttons to visible
void SetVisibleButtonsForTab(int tabGrpId) {
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++) {
		if(FRAME_BUTTONS[i].tabGrpId == tabGrpId) {
			FRAME_BUTTONS[i].button->setVisible(true);
			FRAME_BUTTONS[i].button->setActive(true);
		}
	}
}

void SettingsFrame::activateSubmenu(int submenu)
{
	activeSubmenu = submenu;

	// All buttons: hide; unselect
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		FRAME_BUTTONS[i].button->setVisible(false);
		FRAME_BUTTONS[i].button->setSelected(false);
		FRAME_BUTTONS[i].button->setActive(false);
	}
	// All textBoxes: hide
	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
	{
		FRAME_TEXTBOXES[i].textBox->setVisible(false);
	}
	
	switch (activeSubmenu)	//Tab buttons: set visible; set focus up/down; set selected
	{						//Config buttons: set visible; set selected
		case SUBMENU_GENERAL:
			setDefaultFocus(FRAME_BUTTONS[0].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[5].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[14].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			SetVisibleLabelsForTab(TAB_GENERAL);
			SetVisibleButtonsForTab(TAB_GENERAL);
			// Display button selections based on settings
			SelectBtn(BTN_TAB_GENERAL);
			SelectBtnInGroup((dynacore == DYNACORE_INTERPRETER) ? BTN_DYNAREC : BTN_INTERP, LBL_GENERAL_CPU);
			SelectBtnInGroup((biosDevice == BIOSDEVICE_HLE) ? BTN_HLE : ((biosDevice == BIOSDEVICE_SD) ? BTN_SD : BTN_USB), LBL_GENERAL_BIOS);
			SelectBtnInGroup((LoadCdBios == BOOTTHRUBIOS_YES) ? BTN_BIOS_YES : BTN_BIOS_NO, LBL_GENERAL_BOOT_BIOS);
			break;
		case SUBMENU_VIDEO:
			setDefaultFocus(FRAME_BUTTONS[1].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[16].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[26].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			SetVisibleLabelsForTab(TAB_VIDEO);
			SetVisibleButtonsForTab(TAB_VIDEO);
			// Display button selections based on settings
			SelectBtn(BTN_TAB_VIDEO);
			SelectBtnInGroup((showFPSonScreen == FPS_SHOW) ? BTN_SHOW_FPS_ON : BTN_SHOW_FPS_OFF, LBL_VIDEO_FPS_SHOW);
			SelectBtnInGroup((frameLimit == FRAMELIMIT_AUTO) ? BTN_LIMIT_FPS_ON : BTN_LIMIT_FPS_OFF, LBL_VIDEO_FPS_LIMIT);
			SelectBtnInGroup((frameSkip == FRAMESKIP_ENABLE) ? BTN_FSKIP_ON : BTN_FSKIP_OFF, LBL_VIDEO_FRAMESKIP);
			SelectBtnInGroup((screenMode == SCREENMODE_4x3) ? BTN_SM_4_3 : ((screenMode == SCREENMODE_16x9) ? BTN_SM_16_9 : BTN_SM_F_16_9), LBL_VIDEO_SCREENMODE);
			SelectBtnInGroup((useDithering == USEDITHER_NONE) ? BTN_DITHER_NONE : ((useDithering == USEDITHER_DEFAULT) ? BTN_DITHER_DEFAULT : BTN_DITHER_ALWAYS), LBL_VIDEO_DITHER);
			break;
		case SUBMENU_INPUT:
			setDefaultFocus(FRAME_BUTTONS[2].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[30].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[38].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			SetVisibleLabelsForTab(TAB_INPUT);
			SetVisibleButtonsForTab(TAB_INPUT);
			// Display button selections based on settings
			SelectBtn(BTN_TAB_INPUT);
			SelectBtnInGroup((controllerType == CONTROLLERTYPE_LIGHTGUN) ? BTN_PAD_GUN : ((controllerType == CONTROLLERTYPE_ANALOG) ? BTN_PAD_ANALOG : BTN_PAD_STANDARD), LBL_INPUT_CONT_TYPE);
			SelectBtnInGroup(rumbleEnabled ? BTN_RUMBLE_NO : BTN_RUMBLE_YES, LBL_INPUT_RUMBLE);
			break;
		case SUBMENU_AUDIO:
			setDefaultFocus(FRAME_BUTTONS[3].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[39].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[55].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			SetVisibleLabelsForTab(TAB_AUDIO);
			SetVisibleButtonsForTab(TAB_AUDIO);
			// Display button selections based on settings
			SelectBtn(BTN_TAB_AUDIO);
			SelectBtnInGroup((audioEnabled == AUDIO_DISABLE) ? BTN_DISABLE_AUDIO_YES : BTN_DISABLE_AUDIO_NO, LBL_AUDIO_DISABLE);
			SelectBtnInGroup((!spu_config.iUseReverb) ? BTN_DISABLE_REVERB_YES : BTN_DISABLE_REVERB_NO, LBL_AUDIO_REVERB);
			//FRAME_BUTTONS[45].buttonString = FRAME_STRINGS[46+volume];
			break;
		case SUBMENU_SAVES:
			setDefaultFocus(FRAME_BUTTONS[4].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[46].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[52].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			SetVisibleLabelsForTab(TAB_SAVES);
			SetVisibleButtonsForTab(TAB_SAVES);
			// Display button selections based on settings
			SelectBtn(BTN_TAB_SAVES);
			SelectBtnInGroup((nativeSaveDevice == NATIVESAVEDEVICE_SD) ? BTN_MEMCARD_SD : BTN_MEMCARD_USB, LBL_SAVES_NATIVE_LOC);
			SelectBtnInGroup((saveStateDevice == SAVESTATEDEVICE_SD) ? BTN_SSTATE_SD : BTN_SSTATE_USB, LBL_SAVES_STATE_LOC);
			break;
	}
}

void SettingsFrame::drawChildren(menu::Graphics &gfx)
{
	if(isVisible())
	{
#ifdef HW_RVL
		WPADData* wiiPad = menu::Input::getInstance().getWpad();
#endif
		for (int i=0; i<4; i++)
		{
			u16 currentButtonsGC = PAD_ButtonsHeld(i);
			if (currentButtonsGC ^ previousButtonsGC[i])
			{
				u16 currentButtonsDownGC = (currentButtonsGC ^ previousButtonsGC[i]) & currentButtonsGC;
				previousButtonsGC[i] = currentButtonsGC;
				if (currentButtonsDownGC & PAD_TRIGGER_R)
				{
					//move to next tab
					if(activeSubmenu < SUBMENU_SAVES) 
					{
						activateSubmenu(activeSubmenu+1);
						menu::Focus::getInstance().clearPrimaryFocus();
					}
					break;
				}
				else if (currentButtonsDownGC & PAD_TRIGGER_L)
				{
					//move to the previous tab
					if(activeSubmenu > SUBMENU_GENERAL) 
					{
						activateSubmenu(activeSubmenu-1);
						menu::Focus::getInstance().clearPrimaryFocus();
					}
					break;
				}
			}
#ifdef HW_RVL
			else if (wiiPad[i].btns_h ^ previousButtonsWii[i])
			{
				u32 currentButtonsDownWii = (wiiPad[i].btns_h ^ previousButtonsWii[i]) & wiiPad[i].btns_h;
				previousButtonsWii[i] = wiiPad[i].btns_h;
				if (wiiPad[i].exp.type == WPAD_EXP_CLASSIC)
				{
					if (currentButtonsDownWii & WPAD_CLASSIC_BUTTON_FULL_R)
					{
						//move to next tab
						if(activeSubmenu < SUBMENU_SAVES) 
						{
							activateSubmenu(activeSubmenu+1);
							menu::Focus::getInstance().clearPrimaryFocus();
						}
						break;
					}
					else if (currentButtonsDownWii & WPAD_CLASSIC_BUTTON_FULL_L)
					{
						//move to the previous tab
						if(activeSubmenu > SUBMENU_GENERAL) 
						{
							activateSubmenu(activeSubmenu-1);
							menu::Focus::getInstance().clearPrimaryFocus();
						}
						break;
					}
				}
				else
				{
					if (currentButtonsDownWii & WPAD_BUTTON_PLUS)
					{
						//move to next tab
						if(activeSubmenu < SUBMENU_SAVES) 
						{
							activateSubmenu(activeSubmenu+1);
							menu::Focus::getInstance().clearPrimaryFocus();
						}
						break;
					}
					else if (currentButtonsDownWii & WPAD_BUTTON_MINUS)
					{
						//move to the previous tab
						if(activeSubmenu > SUBMENU_GENERAL) 
						{
							activateSubmenu(activeSubmenu-1);
							menu::Focus::getInstance().clearPrimaryFocus();
						}
						break;
					}
				}
			}
#endif //HW_RVL
		}

		//Draw buttons
		menu::ComponentList::const_iterator iteration;
		for (iteration = componentList.begin(); iteration != componentList.end(); iteration++)
		{
			(*iteration)->draw(gfx);
		}
	}
}

extern MenuContext *pMenuContext;

void Func_TabGeneral()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_GENERAL);
}

void Func_TabVideo()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_VIDEO);
}

void Func_TabInput()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_INPUT);
}

void Func_TabAudio()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_AUDIO);
}

void Func_TabSaves()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_SAVES);
}

void Func_CpuInterp()
{
	SelectBtnInGroup(BTN_INTERP, LBL_GENERAL_CPU);

	int needInit = 0;
	if(hasLoadedISO && dynacore != DYNACORE_INTERPRETER){ SysClose(); needInit = 1; }
	dynacore = DYNACORE_INTERPRETER;
	if(hasLoadedISO && needInit) {
		SysInit();
		CheckCdrom();
		SysReset();
		LoadCdrom();
		Func_SetPlayGame();
		menu::MessageBox::getInstance().setMessage("Game Reset");
	}
}

void Func_CpuDynarec()
{
	SelectBtnInGroup(BTN_DYNAREC, LBL_GENERAL_CPU);

	int needInit = 0;
	if(hasLoadedISO && dynacore != DYNACORE_DYNAREC){ SysClose(); needInit = 1; }
	dynacore = DYNACORE_DYNAREC;
	if(hasLoadedISO && needInit) {
		SysInit ();
		CheckCdrom();
		SysReset();
		LoadCdrom();
		Func_SetPlayGame();
		menu::MessageBox::getInstance().setMessage("Game Reset");
	}
}

void Func_BiosSelectHLE()
{
	SelectBtnInGroup(BTN_HLE, LBL_GENERAL_BIOS);

	int needInit = 0;
	if(hasLoadedISO && biosDevice != BIOSDEVICE_HLE){ SysClose(); needInit = 1; }
	biosDevice = BIOSDEVICE_HLE;
	if(hasLoadedISO && needInit) {
		SysInit ();
		CheckCdrom();
		SysReset();
		LoadCdrom();
		Func_SetPlayGame();
		menu::MessageBox::getInstance().setMessage("Game Reset");
	}
}

int checkBiosExists(int testDevice) 
{
	fileBrowser_file testFile;
	memset(&testFile, 0, sizeof(fileBrowser_file));
	
	biosFile_dir = (testDevice == BIOSDEVICE_SD) ? &biosDir_libfat_Default : &biosDir_libfat_USB;
	sprintf(&testFile.name[0], "%s/SCPH1001.BIN", &biosFile_dir->name[0]);
	biosFile_readFile  = fileBrowser_libfat_readFile;
	biosFile_open      = fileBrowser_libfat_open;
	biosFile_init      = fileBrowser_libfat_init;
	biosFile_deinit    = fileBrowser_libfat_deinit;
	biosFile_init(&testFile);  //initialize the bios device (it might not be the same as ISO device)
	return biosFile_open(&testFile);
}

void Func_BiosSelectSD()
{

	int needInit = 0;
	if(checkBiosExists(BIOSDEVICE_SD) == FILE_BROWSER_ERROR_NO_FILE) {
		menu::MessageBox::getInstance().setMessage("BIOS not found on SD");
		if(hasLoadedISO && biosDevice != BIOSDEVICE_HLE){ SysClose(); needInit = 1; }
		biosDevice = BIOSDEVICE_HLE;
		SelectBtnInGroup(BTN_HLE, LBL_GENERAL_BIOS);
	}
	else {
		if(hasLoadedISO && biosDevice != BIOSDEVICE_SD){ SysClose(); needInit = 1; }
		biosDevice = BIOSDEVICE_SD;
		SelectBtnInGroup(BTN_SD, LBL_GENERAL_BIOS);
	}
	if(hasLoadedISO && needInit) {
		SysInit ();
		CheckCdrom();
		SysReset();
		LoadCdrom();
		Func_SetPlayGame();
		menu::MessageBox::getInstance().setMessage("Game Reset");
	}
}

void Func_BiosSelectUSB()
{

	int needInit = 0;
	if(checkBiosExists(BIOSDEVICE_USB) == FILE_BROWSER_ERROR_NO_FILE) {
		menu::MessageBox::getInstance().setMessage("BIOS not found on USB");
		if(hasLoadedISO && biosDevice != BIOSDEVICE_HLE){ SysClose(); needInit = 1; }
		biosDevice = BIOSDEVICE_HLE;
		SelectBtnInGroup(BTN_HLE, LBL_GENERAL_BIOS);
	}
	else {
		if(hasLoadedISO && biosDevice != BIOSDEVICE_USB){ SysClose(); needInit = 1; }
		biosDevice = BIOSDEVICE_USB;
		SelectBtnInGroup(BTN_USB, LBL_GENERAL_BIOS);
	}
	if(hasLoadedISO && needInit) {
		SysInit ();
		CheckCdrom();
		SysReset();
		LoadCdrom();
		Func_SetPlayGame();
		menu::MessageBox::getInstance().setMessage("Game Reset");
	}
}

void Func_BootBiosYes()
{
	/* If HLE bios selected, boot thru bios shouldn't make a difference. TODO: Check this.
	if(biosDevice == BIOSDEVICE_HLE) {
		menu::MessageBox::getInstance().setMessage("You must select a BIOS, not HLE");
		return;
	}*/
	
	SelectBtnInGroup(BTN_BIOS_YES, LBL_GENERAL_BOOT_BIOS);
	
	LoadCdBios = BOOTTHRUBIOS_YES;
}

void Func_BootBiosNo()
{
	SelectBtnInGroup(BTN_BIOS_NO, LBL_GENERAL_BOOT_BIOS);

	LoadCdBios = BOOTTHRUBIOS_NO;
}

void Func_ExecuteBios()
{
	if(biosDevice == BIOSDEVICE_HLE) {
		menu::MessageBox::getInstance().setMessage("You must select a BIOS, not HLE");
		return;
	}
	if(hasLoadedISO) {
		//TODO: Implement yes/no that current game will be reset
		SysClose();
	}

	//TODO: load/save memcards here
	if(SysInit() < 0) {
		menu::MessageBox::getInstance().setMessage("Failed to initialize system.\nTry loading an ISO.");
		return;
	}
	CheckCdrom();
	SysReset();
	resumeAudio();
	resumeInput();
	menuActive = 0;
	SysStartCPU();
	menuActive = 1;
	pauseInput();
	pauseAudio();
}

extern void writeConfig(FILE* f);

void Func_SaveSettingsSD()
{
	fileBrowser_file* configFile_file;
	int (*configFile_init)(fileBrowser_file*) = fileBrowser_libfat_init;
	configFile_file = &saveDir_libfat_Default;
	if(configFile_init(configFile_file)) {                //only if device initialized ok
		FILE* f = fopen( "sd:/wiisx/settings.cfg", "wb" );  //attempt to open file
		if(f) {
			writeConfig(f);                                   //write out the config
			fclose(f);
			menu::MessageBox::getInstance().setMessage("Saved settings.cfg to SD");
			return;
		}
	}
	menu::MessageBox::getInstance().setMessage("Error saving settings.cfg to SD");
}

void Func_SaveSettingsUSB()
{
	fileBrowser_file* configFile_file;
	int (*configFile_init)(fileBrowser_file*) = fileBrowser_libfat_init;
	configFile_file = &saveDir_libfat_USB;
	if(configFile_init(configFile_file)) {                //only if device initialized ok
		FILE* f = fopen( "usb:/wiisx/settings.cfg", "wb" ); //attempt to open file
		if(f) {
			writeConfig(f);                                   //write out the config
			fclose(f);
			menu::MessageBox::getInstance().setMessage("Saved settings.cfg to USB");
			return;
		}
	}
	menu::MessageBox::getInstance().setMessage("Error saving settings.cfg to USB");
}

void Func_ShowFpsOn()
{
	SelectBtnInGroup(BTN_SHOW_FPS_ON, LBL_VIDEO_FPS_SHOW);
	showFPSonScreen = FPS_SHOW;
}

void Func_ShowFpsOff()
{
	SelectBtnInGroup(BTN_SHOW_FPS_OFF, LBL_VIDEO_FPS_SHOW);
	showFPSonScreen = FPS_HIDE;
}

void Func_FpsLimitAuto()
{
	SelectBtnInGroup(BTN_LIMIT_FPS_ON, LBL_VIDEO_FPS_LIMIT);
	frameLimit = FRAMELIMIT_AUTO;
}

void Func_FpsLimitOff()
{
	SelectBtnInGroup(BTN_LIMIT_FPS_OFF, LBL_VIDEO_FPS_LIMIT);
	frameLimit = FRAMELIMIT_NONE;
}

void Func_FrameSkipOn()
{
	SelectBtnInGroup(BTN_FSKIP_ON, LBL_VIDEO_FRAMESKIP);
	frameSkip = FRAMESKIP_ENABLE;
}

void Func_FrameSkipOff()
{
	SelectBtnInGroup(BTN_FSKIP_OFF, LBL_VIDEO_FRAMESKIP);
	frameSkip = FRAMESKIP_DISABLE;
}

void Func_ScreenMode4_3()
{
	SelectBtnInGroup(BTN_SM_4_3, LBL_VIDEO_SCREENMODE);
	screenMode = SCREENMODE_4x3;
}

void Func_ScreenMode16_9()
{
	SelectBtnInGroup(BTN_SM_16_9, LBL_VIDEO_SCREENMODE);
	screenMode = SCREENMODE_16x9;
}

void Func_ScreenForce16_9()
{
	SelectBtnInGroup(BTN_SM_F_16_9, LBL_VIDEO_SCREENMODE);
	screenMode = SCREENMODE_16x9_PILLARBOX;
}

void Func_DitheringNone()
{
	SelectBtnInGroup(BTN_DITHER_NONE, LBL_VIDEO_DITHER);
	useDithering = USEDITHER_NONE;
}

void Func_DitheringDefault()
{
	SelectBtnInGroup(BTN_DITHER_DEFAULT, LBL_VIDEO_DITHER);
	useDithering = USEDITHER_DEFAULT;
}

void Func_DitheringAlways()
{
	SelectBtnInGroup(BTN_DITHER_ALWAYS, LBL_VIDEO_DITHER);
	useDithering = USEDITHER_ALWAYS;
}


void Func_ConfigureInput()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_CONFIGUREINPUT,ConfigureInputFrame::SUBMENU_REINIT);
}

void Func_ConfigureButtons()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_CONFIGUREBUTTONS,ConfigureButtonsFrame::SUBMENU_PSX_PADNONE);
}

void Func_PsxTypeStandard()
{
	SelectBtnInGroup(BTN_PAD_STANDARD, LBL_INPUT_CONT_TYPE);
	controllerType = CONTROLLERTYPE_STANDARD;
}

void Func_PsxTypeAnalog()
{
	SelectBtnInGroup(BTN_PAD_ANALOG, LBL_INPUT_CONT_TYPE);
	controllerType = CONTROLLERTYPE_ANALOG;
}

void Func_PsxTypeLightgun()
{
	SelectBtnInGroup(BTN_PAD_GUN, LBL_INPUT_CONT_TYPE);
	controllerType = CONTROLLERTYPE_LIGHTGUN;
}

void Func_DisableRumbleYes()
{
	SelectBtnInGroup(BTN_RUMBLE_YES, LBL_INPUT_RUMBLE);
	rumbleEnabled = RUMBLE_DISABLE;
}

void Func_DisableRumbleNo()
{
	SelectBtnInGroup(BTN_RUMBLE_NO, LBL_INPUT_RUMBLE);
	rumbleEnabled = RUMBLE_ENABLE;
}

void Func_SaveButtonsSD()
{
	fileBrowser_file* configFile_file;
	int (*configFile_init)(fileBrowser_file*) = fileBrowser_libfat_init;
	int num_written = 0;
	configFile_file = &saveDir_libfat_Default;
	if(configFile_init(configFile_file)) {                //only if device initialized ok
		FILE* f = fopen( "sd:/wiisx/controlG.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_GC);					//write out GC controller mappings
			fclose(f);
			num_written++;
		}
#ifdef HW_RVL
		f = fopen( "sd:/wiisx/controlC.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_Classic);			//write out Classic controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "sd:/wiisx/controlN.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_WiimoteNunchuk);	//write out WM+NC controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "sd:/wiisx/controlW.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_Wiimote);			//write out Wiimote controller mappings
			fclose(f);
			num_written++;
		}
#endif //HW_RVL
	}
	if (num_written == num_controller_t)
		menu::MessageBox::getInstance().setMessage("Saved Button Configs to SD");
	else
		menu::MessageBox::getInstance().setMessage("Error saving Button Configs to SD");
}

void Func_SaveButtonsUSB()
{
	fileBrowser_file* configFile_file;
	int (*configFile_init)(fileBrowser_file*) = fileBrowser_libfat_init;
	int num_written = 0;
	configFile_file = &saveDir_libfat_USB;
	if(configFile_init(configFile_file)) {                //only if device initialized ok
		FILE* f = fopen( "usb:/wiisx/controlG.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_GC);					//write out GC controller mappings
			fclose(f);
			num_written++;
		}
#ifdef HW_RVL
		f = fopen( "usb:/wiisx/controlC.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_Classic);			//write out Classic controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "usb:/wiisx/controlN.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_WiimoteNunchuk);	//write out WM+NC controller mappings
			fclose(f);
			num_written++;
		}
		f = fopen( "usb:/wiisx/controlW.cfg", "wb" );  //attempt to open file
		if(f) {
			save_configurations(f, &controller_Wiimote);			//write out Wiimote controller mappings
			fclose(f);
			num_written++;
		}
#endif //HW_RVL
	}
	if (num_written == num_controller_t)
		menu::MessageBox::getInstance().setMessage("Saved Button Configs to USB");
	else
		menu::MessageBox::getInstance().setMessage("Error saving Button Configs to USB");
}

void Func_ToggleButtonLoad()
{
	loadButtonSlot = (loadButtonSlot + 1) % 5;
	//if (loadButtonSlot == LOADBUTTON_DEFAULT)
	//	strcpy(FRAME_STRINGS[42], "Default");
	//else
	//	sprintf(FRAME_STRINGS[42], "Slot %d", loadButtonSlot+1);
}

void Func_DisableAudioYes()
{
	SelectBtnInGroup(BTN_DISABLE_AUDIO_YES, LBL_AUDIO_DISABLE);
	audioEnabled = AUDIO_DISABLE;
}

void Func_DisableAudioNo()
{
	SelectBtnInGroup(BTN_DISABLE_AUDIO_NO, LBL_AUDIO_DISABLE);
	audioEnabled = AUDIO_ENABLE;
}

void Func_DisableReverbYes()
{
	SelectBtnInGroup(BTN_DISABLE_REVERB_YES, LBL_AUDIO_REVERB);
	spu_config.iUseReverb = 0;
}

void Func_DisableReverbNo()
{
	SelectBtnInGroup(BTN_DISABLE_REVERB_NO, LBL_AUDIO_REVERB);
	spu_config.iUseReverb = 1;
}

extern "C" void SetVolume(void);

void Func_VolumeToggle()
{
	volume--;
	if (volume<1)
		volume = 4;
	spu_config.iVolume = 1024 - (volume * 192);
	//FRAME_BUTTONS[45].buttonString = FRAME_STRINGS[46+volume];
	SetVolume();
}

void Func_MemcardSaveSD()
{
	SelectBtnInGroup(BTN_MEMCARD_SD, LBL_SAVES_NATIVE_LOC);
	nativeSaveDevice = NATIVESAVEDEVICE_SD;
}

void Func_MemcardSaveUSB()
{
	SelectBtnInGroup(BTN_MEMCARD_USB, LBL_SAVES_NATIVE_LOC);
	nativeSaveDevice = NATIVESAVEDEVICE_USB;
}

void Func_SaveStateSD()
{
	SelectBtnInGroup(BTN_SSTATE_SD, LBL_SAVES_STATE_LOC);
	saveStateDevice = SAVESTATEDEVICE_SD;
}

void Func_SaveStateUSB()
{
	SelectBtnInGroup(BTN_SSTATE_USB, LBL_SAVES_STATE_LOC);
	saveStateDevice = SAVESTATEDEVICE_USB;
}

void Func_ReturnFromSettingsFrame()
{
	menu::Gui::getInstance().menuLogo->setLocation(580.0, 70.0, -50.0);
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}

