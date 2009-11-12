/**
 * WiiSX - SettingsFrame.cpp
 * Copyright (C) 2009 sepp256
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
#include "../../PsxCommon.h"

extern "C" {
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
void Func_DitheringNone();
void Func_DitheringDefault();
void Func_DitheringAlways();
void Func_ScalingNone();
void Func_Scaling2xSai();

void Func_AssignInput();
void Func_InputOptions();
void Func_MapButtons();

void Func_DisableAudioYes();
void Func_DisableAudioNo();
void Func_DisableXaYes();
void Func_DisableXaNo();
void Func_DisableCddaYes();
void Func_DisableCddaNo();
void Func_VolumeToggle();

void Func_MemcardSaveSD();
void Func_MemcardSaveUSB();
void Func_MemcardSaveCardA();
void Func_MemcardSaveCardB();
void Func_AutoSaveYes();
void Func_AutoSaveNo();
void Func_SaveStateSD();
void Func_SaveStateUSB();

void Func_ReturnFromSettingsFrame();

extern BOOL hasLoadedISO;
extern int stop;
extern char menuActive;

extern "C" {
void SysReset();
void SysInit();
void SysClose();
void SysStartCPU();
void CheckCdrom();
void LoadCdrom();
void pauseAudio(void);  void pauseInput(void);
void resumeAudio(void); void resumeInput(void);
}

#define NUM_FRAME_BUTTONS 46
#define NUM_TAB_BUTTONS 5
#define FRAME_BUTTONS settingsFrameButtons
#define FRAME_STRINGS settingsFrameStrings
#define NUM_FRAME_TEXTBOXES 17
#define FRAME_TEXTBOXES settingsFrameTextBoxes

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
Scaling: None; 2xSaI
Dithering: None; Game Dependent; Always

Input Tab:
Configure Input Mapping (assign player->pad)
Configure Input Options (rumble; analog/digital)
Configure Button Mappings

Audio Tab:
Disable Audio: Yes; No
Disable XA: Yes; No
Disable CDDA: Yes; No
Volume Level: ("0: low", "1: medium", "2: loud", "3: loudest")
	Note: iVolume=4-ComboBox_GetCurSel(hWC);, so 1 is loudest and 4 is low; default is medium

Saves Tab:
Memcard Save Device: SD; USB; CardA; CardB
Auto Save Memcards: Yes; No
Save States Device: SD; USB
*/

static char FRAME_STRINGS[48][24] =
	{ "General",
	  "Video",
	  "Input",
	  "Audio",
	  "Saves",
	//Strings for General tab (starting at FRAME_STRINGS[5])
	  "Select CPU Core",
	  "Select Bios",
	  "Boot Through Bios",
	  "Execute Bios",
	  "Save settings.cfg",
	  "Interpreter",
	  "Dynarec",
	  "HLE",
	  "SD",
	  "USB",
	  "Yes",
	  "No",
	//Strings for Video tab (starting at FRAME_STRINGS[17])
	  "Show FPS",
	  "Limit FPS",
	  "Frame Skip",
	  "Screen Mode",
	  "Scaling",
	  "Dithering",
	  "On",
	  "Off",
	  "Auto",
	  "4:3",
	  "16:9",
	  "None",
	  "2xSaI",
	  "Default",
	  "Always",
	//Strings for Input tab (starting at FRAME_STRINGS[32])
	  "Assign Input",
	  "Input Options",
	  "Button Mappings",
	//Strings for Audio tab (starting at FRAME_STRINGS[35])
	  "Disable Audio",
	  "Disable XA",
	  "Disable CDDA",
	  "Volume",
	  "loudest",	//iVolume=1
	  "loud",
	  "medium",
	  "low",		//iVolume=4
	//Strings for Saves tab (starting at FRAME_STRINGS[43])
	  "Memcard Save Device",
	  "Auto Save Memcards",
	  "Save States Device",
	  "CardA",
	  "CardB"};


struct ButtonInfo
{
	menu::Button	*button;
	int				buttonStyle;
	char*			buttonString;
	float			x;
	float			y;
	float			width;
	float			height;
	int				focusUp;
	int				focusDown;
	int				focusLeft;
	int				focusRight;
	ButtonFunc		clickedFunc;
	ButtonFunc		returnFunc;
} FRAME_BUTTONS[NUM_FRAME_BUTTONS] =
{ //	button	buttonStyle buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc				returnFunc
	//Buttons for Tabs (starts at button[0])
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[0],	 25.0,	 30.0,	110.0,	56.0,	-1,	-1,	 4,	 1,	Func_TabGeneral,		Func_ReturnFromSettingsFrame }, // General tab
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[1],	155.0,	 30.0,	100.0,	56.0,	-1,	-1,	 0,	 2,	Func_TabVideo,			Func_ReturnFromSettingsFrame }, // Video tab
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[2],	275.0,	 30.0,	100.0,	56.0,	-1,	-1,	 1,	 3,	Func_TabInput,			Func_ReturnFromSettingsFrame }, // Input tab
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[3],	395.0,	 30.0,	100.0,	56.0,	-1,	-1,	 2,	 4,	Func_TabAudio,			Func_ReturnFromSettingsFrame }, // Audio tab
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[4],	515.0,	 30.0,	100.0,	56.0,	-1,	-1,	 3,	 0,	Func_TabSaves,			Func_ReturnFromSettingsFrame }, // Saves tab
	//Buttons for General Tab (starts at button[5])
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[10],	295.0,	100.0,	160.0,	56.0,	 0,	 7,	 6,	 6,	Func_CpuInterp,			Func_ReturnFromSettingsFrame }, // CPU: Interp
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[11],	465.0,	100.0,	130.0,	56.0,	 0,	 9,	 5,	 5,	Func_CpuDynarec,		Func_ReturnFromSettingsFrame }, // CPU: Dynarec
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[12],	295.0,	170.0,	 70.0,	56.0,	 5,	10,	 9,	 8,	Func_BiosSelectHLE,		Func_ReturnFromSettingsFrame }, // Bios: HLE
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[13],	375.0,	170.0,	 55.0,	56.0,	 5,	11,	 7,	 9,	Func_BiosSelectSD,		Func_ReturnFromSettingsFrame }, // Bios: SD
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[14],	440.0,	170.0,	 70.0,	56.0,	 6,	11,	 8,	 7,	Func_BiosSelectUSB,		Func_ReturnFromSettingsFrame }, // Bios: USB
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[15],	295.0,	240.0,	 75.0,	56.0,	 7,	12,	11,	11,	Func_BootBiosYes,		Func_ReturnFromSettingsFrame }, // Boot Thru Bios: Yes
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[16],	380.0,	240.0,	 75.0,	56.0,	 8,	12,	10,	10,	Func_BootBiosNo,		Func_ReturnFromSettingsFrame }, // Boot Thru Bios: No
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[8],	295.0,	310.0,	200.0,	56.0,	10,	13,	-1,	-1,	Func_ExecuteBios,		Func_ReturnFromSettingsFrame }, // Execute Bios
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[13],	295.0,	380.0,	 55.0,	56.0,	12,	 0,	14,	14,	Func_SaveSettingsSD,	Func_ReturnFromSettingsFrame }, // Save Settings: SD
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[14],	360.0,	380.0,	 70.0,	56.0,	12,	 0,	13,	13,	Func_SaveSettingsUSB,	Func_ReturnFromSettingsFrame }, // Save Settings: USB
	//Buttons for Video Tab (starts at button[15])
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[23],	325.0,	100.0,	 75.0,	56.0,	 1,	17,	16,	16,	Func_ShowFpsOn,			Func_ReturnFromSettingsFrame }, // Show FPS: On
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[24],	420.0,	100.0,	 75.0,	56.0,	 1,	18,	15,	15,	Func_ShowFpsOff,		Func_ReturnFromSettingsFrame }, // Show FPS: Off
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[25],	325.0,	170.0,	 75.0,	56.0,	15,	19,	18,	18,	Func_FpsLimitAuto,		Func_ReturnFromSettingsFrame }, // FPS Limit: Auto
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[24],	420.0,	170.0,	 75.0,	56.0,	16,	20,	17,	17,	Func_FpsLimitOff,		Func_ReturnFromSettingsFrame }, // FPS Limit: Off
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[23],	325.0,	240.0,	 75.0,	56.0,	17,	23,	20,	20,	Func_FrameSkipOn,		Func_ReturnFromSettingsFrame }, // Frame Skip: On
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[24],	420.0,	240.0,	 75.0,	56.0,	18,	24,	19,	19,	Func_FrameSkipOff,		Func_ReturnFromSettingsFrame }, // Frame Skip: Off
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[26],	325.0,	380.0,	 75.0,	56.0,	23,	 1,	22,	22,	Func_ScreenMode4_3,		Func_ReturnFromSettingsFrame }, // ScreenMode: 4:3
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[27],	420.0,	380.0,	 75.0,	56.0,	24,	 1,	21,	21,	Func_ScreenMode16_9,	Func_ReturnFromSettingsFrame }, // ScreenMode: 16:9
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[28],	285.0,	310.0,	 80.0,	56.0,	19,	21,	25,	24,	Func_DitheringNone,		Func_ReturnFromSettingsFrame }, // Dithering: None
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[30],	385.0,	310.0,	110.0,	56.0,	20,	22,	23,	25,	Func_DitheringDefault,	Func_ReturnFromSettingsFrame }, // Dithering: Game Dependent
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[31],	515.0,	310.0,	110.0,	56.0,	20,	22,	24,	23,	Func_DitheringAlways,	Func_ReturnFromSettingsFrame }, // Dithering: Always
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[28],	325.0,	430.0,	 75.0,	56.0,	-1,	-1,	27,	27,	Func_ScalingNone,		Func_ReturnFromSettingsFrame }, // Scaling: None
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[29],	420.0,	430.0,	 75.0,	56.0,	-1,	-1,	26,	26,	Func_Scaling2xSai,		Func_ReturnFromSettingsFrame }, // Scaling: 2xSai
	//Buttons for Input Tab (starts at button[28])
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[32],	180.0,	100.0,	280.0,	56.0,	 2,	29,	-1,	-1,	Func_AssignInput,		Func_ReturnFromSettingsFrame }, // Configure Input Assignment
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[33],	180.0,	170.0,	280.0,	56.0,	28,	30,	-1,	-1,	Func_InputOptions,		Func_ReturnFromSettingsFrame }, // Configure Input Options
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[34],	180.0,	240.0,	280.0,	56.0,	29,	 2,	-1,	-1,	Func_MapButtons,		Func_ReturnFromSettingsFrame }, // Configure Button Mappings
	//Buttons for Audio Tab (starts at button[31])
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[15],	345.0,	100.0,	 75.0,	56.0,	 3,	33,	32,	32,	Func_DisableAudioYes,	Func_ReturnFromSettingsFrame }, // Disable Audio: Yes
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[16],	440.0,	100.0,	 75.0,	56.0,	 3,	34,	31,	31,	Func_DisableAudioNo,	Func_ReturnFromSettingsFrame }, // Disable Audio: No
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[15],	345.0,	170.0,	 75.0,	56.0,	31,	35,	34,	34,	Func_DisableXaYes,		Func_ReturnFromSettingsFrame }, // Disable XA: Yes
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[16],	440.0,	170.0,	 75.0,	56.0,	32,	36,	33,	33,	Func_DisableXaNo,		Func_ReturnFromSettingsFrame }, // Disable XA: No
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[15],	345.0,	240.0,	 75.0,	56.0,	33,	37,	36,	36,	Func_DisableCddaYes,	Func_ReturnFromSettingsFrame }, // Disable CDDA: Yes
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[16],	440.0,	240.0,	 75.0,	56.0,	34,	37,	35,	35,	Func_DisableCddaNo,		Func_ReturnFromSettingsFrame }, // Disable CDDA: No
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[39],	345.0,	310.0,	170.0,	56.0,	35,	 3,	-1,	-1,	Func_VolumeToggle,		Func_ReturnFromSettingsFrame }, // Volume: low/medium/loud/loudest
	//Buttons for Saves Tab (starts at button[38])
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[13],	295.0,	100.0,	 55.0,	56.0,	 4,	42,	41,	39,	Func_MemcardSaveSD,		Func_ReturnFromSettingsFrame }, // Memcard Save: SD
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[14],	360.0,	100.0,	 70.0,	56.0,	 4,	43,	38,	40,	Func_MemcardSaveUSB,	Func_ReturnFromSettingsFrame }, // Memcard Save: USB
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[46],	440.0,	100.0,	 90.0,	56.0,	 4,	43,	39,	41,	Func_MemcardSaveCardA,	Func_ReturnFromSettingsFrame }, // Memcard Save: Card A
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[47],	540.0,	100.0,	 90.0,	56.0,	 4,	43,	40,	38,	Func_MemcardSaveCardB,	Func_ReturnFromSettingsFrame }, // Memcard Save: Card B
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[15],	295.0,	170.0,	 75.0,	56.0,	38,	44,	43,	43,	Func_AutoSaveYes,		Func_ReturnFromSettingsFrame }, // Auto Save Memcards: Yes
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[16],	380.0,	170.0,	 75.0,	56.0,	39,	45,	42,	42,	Func_AutoSaveNo,		Func_ReturnFromSettingsFrame }, // Auto Save Memcards: No
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[13],	295.0,	240.0,	 55.0,	56.0,	42,	 4,	45,	45,	Func_SaveStateSD,		Func_ReturnFromSettingsFrame }, // Save State: SD
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[14],	360.0,	240.0,	 70.0,	56.0,	43,	 4,	44,	44,	Func_SaveStateUSB,		Func_ReturnFromSettingsFrame }, // Save State: USB
};

struct TextBoxInfo
{
	menu::TextBox	*textBox;
	char*			textBoxString;
	float			x;
	float			y;
	float			scale;
	bool			centered;
} FRAME_TEXTBOXES[NUM_FRAME_TEXTBOXES] =
{ //	textBox	textBoxString		x		y		scale	centered
	//TextBoxes for General Tab (starts at textBox[0])
	{	NULL,	FRAME_STRINGS[5],	155.0,	128.0,	 1.0,	true }, // CPU Core: Pure Interp/Dynarec
	{	NULL,	FRAME_STRINGS[6],	155.0,	198.0,	 1.0,	true }, // Bios: HLE/SD/USB
	{	NULL,	FRAME_STRINGS[7],	155.0,	268.0,	 1.0,	true }, // Boot Thru Bios: Yes/No
	{	NULL,	FRAME_STRINGS[9],	155.0,	408.0,	 1.0,	true }, // Save settings.cfg: SD/USB
	//TextBoxes for Video Tab (starts at textBox[4])
	{	NULL,	FRAME_STRINGS[17],	190.0,	128.0,	 1.0,	true }, // Show FPS: On/Off
	{	NULL,	FRAME_STRINGS[18],	190.0,	198.0,	 1.0,	true }, // Limit FPS: Auto/Off
	{	NULL,	FRAME_STRINGS[19],	190.0,	268.0,	 1.0,	true }, // Frame Skip: On/Off
	{	NULL,	FRAME_STRINGS[20],	190.0,	408.0,	 1.0,	true }, // ScreenMode: 4x3/16x9
	{	NULL,	FRAME_STRINGS[22],	190.0,	338.0,	 1.0,	true }, // Dithering: None/Game Dependent/Always
	{	NULL,	FRAME_STRINGS[21],	190.0,	478.0,	 1.0,	true }, // Scaling: None/2xSai
	//TextBoxes for Input Tab (starts at textBox[])
	//TextBoxes for Audio Tab (starts at textBox[10])
	{	NULL,	FRAME_STRINGS[35],	210.0,	128.0,	 1.0,	true }, // Disable Audio: Yes/No
	{	NULL,	FRAME_STRINGS[36],	210.0,	198.0,	 1.0,	true }, // Disable XA Audio: Yes/No
	{	NULL,	FRAME_STRINGS[37],	210.0,	268.0,	 1.0,	true }, // Disable CDDA Audio: Yes/No
	{	NULL,	FRAME_STRINGS[38],	210.0,	338.0,	 1.0,	true }, // Volume: low/medium/loud/loudest
	//TextBoxes for Saves Tab (starts at textBox[14])
	{	NULL,	FRAME_STRINGS[43],	150.0,	128.0,	 1.0,	true }, // Memcard Save Device: SD/USB/CardA/CardB
	{	NULL,	FRAME_STRINGS[44],	150.0,	198.0,	 1.0,	true }, // Auto Save Memcards: Yes/No
	{	NULL,	FRAME_STRINGS[45],	150.0,	268.0,	 1.0,	true }, // Save State Device: SD/USB
};

SettingsFrame::SettingsFrame()
		: activeSubmenu(SUBMENU_GENERAL)
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
		FRAME_BUTTONS[i].button = new menu::Button(FRAME_BUTTONS[i].buttonStyle, &FRAME_BUTTONS[i].buttonString, 
										FRAME_BUTTONS[i].x, FRAME_BUTTONS[i].y, 
										FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].height);

	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		if (FRAME_BUTTONS[i].focusUp != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[FRAME_BUTTONS[i].focusUp].button);
		if (FRAME_BUTTONS[i].focusDown != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[FRAME_BUTTONS[i].focusDown].button);
		if (FRAME_BUTTONS[i].focusLeft != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_LEFT, FRAME_BUTTONS[FRAME_BUTTONS[i].focusLeft].button);
		if (FRAME_BUTTONS[i].focusRight != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_RIGHT, FRAME_BUTTONS[FRAME_BUTTONS[i].focusRight].button);
		FRAME_BUTTONS[i].button->setActive(true);
		if (FRAME_BUTTONS[i].clickedFunc) FRAME_BUTTONS[i].button->setClicked(FRAME_BUTTONS[i].clickedFunc);
		if (FRAME_BUTTONS[i].returnFunc) FRAME_BUTTONS[i].button->setReturn(FRAME_BUTTONS[i].returnFunc);
		add(FRAME_BUTTONS[i].button);
		menu::Cursor::getInstance().addComponent(this, FRAME_BUTTONS[i].button, FRAME_BUTTONS[i].x, 
												FRAME_BUTTONS[i].x+FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].y, 
												FRAME_BUTTONS[i].y+FRAME_BUTTONS[i].height);
	}

	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
	{
		FRAME_TEXTBOXES[i].textBox = new menu::TextBox(&FRAME_TEXTBOXES[i].textBoxString, 
										FRAME_TEXTBOXES[i].x, FRAME_TEXTBOXES[i].y, 
										FRAME_TEXTBOXES[i].scale, FRAME_TEXTBOXES[i].centered);
		add(FRAME_TEXTBOXES[i].textBox);
	}

	setDefaultFocus(FRAME_BUTTONS[0].button);
	setBackFunc(Func_ReturnFromSettingsFrame);
	setEnabled(true);
	activateSubmenu(SUBMENU_GENERAL);
}

SettingsFrame::~SettingsFrame()
{
	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
		delete FRAME_TEXTBOXES[i].textBox;
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}

}

void SettingsFrame::activateSubmenu(int submenu)
{
	activeSubmenu = submenu;

	//All buttons: hide; unselect
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		FRAME_BUTTONS[i].button->setVisible(false);
		FRAME_BUTTONS[i].button->setSelected(false);
		FRAME_BUTTONS[i].button->setActive(false);
	}
	//All textBoxes: hide
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
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[13].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			for (int i = 0; i < 4; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[0].button->setSelected(true);
			if (dynacore == DYNACORE_INTERPRETER)	FRAME_BUTTONS[5].button->setSelected(true);
			else									FRAME_BUTTONS[6].button->setSelected(true);
			FRAME_BUTTONS[7+biosDevice].button->setSelected(true);
			if (LoadCdBios == BOOTTHRUBIOS_YES)	FRAME_BUTTONS[10].button->setSelected(true);
			else								FRAME_BUTTONS[11].button->setSelected(true);
			for (int i = 5; i < 15; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			break;
		case SUBMENU_VIDEO:
			setDefaultFocus(FRAME_BUTTONS[1].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[15].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[21].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			for (int i = 4; i < 9; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[1].button->setSelected(true);
			if (showFPSonScreen == FPS_SHOW)	FRAME_BUTTONS[15].button->setSelected(true);
			else								FRAME_BUTTONS[16].button->setSelected(true);
			if (frameLimit == FRAMELIMIT_AUTO)	FRAME_BUTTONS[17].button->setSelected(true);
			else								FRAME_BUTTONS[18].button->setSelected(true);
			if (frameSkip == FRAMESKIP_ENABLE)	FRAME_BUTTONS[19].button->setSelected(true);
			else								FRAME_BUTTONS[20].button->setSelected(true);
			if (screenMode == SCREENMODE_4x3)	FRAME_BUTTONS[21].button->setSelected(true);
			else								FRAME_BUTTONS[22].button->setSelected(true);
			FRAME_BUTTONS[23+iUseDither].button->setSelected(true);
			for (int i = 15; i < 26; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			break;
		case SUBMENU_INPUT:
			setDefaultFocus(FRAME_BUTTONS[2].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[28].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[30].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			FRAME_BUTTONS[2].button->setSelected(true);
			for (int i = 28; i < 31; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			break;
		case SUBMENU_AUDIO:
			setDefaultFocus(FRAME_BUTTONS[3].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[31].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[37].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			for (int i = 10; i < 14; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[3].button->setSelected(true);
			if (audioEnabled == AUDIO_DISABLE)	FRAME_BUTTONS[31].button->setSelected(true);
			else								FRAME_BUTTONS[32].button->setSelected(true);
			if (Config.Xa == XA_DISABLE)		FRAME_BUTTONS[33].button->setSelected(true);
			else								FRAME_BUTTONS[34].button->setSelected(true);
			if (Config.Cdda == CDDA_DISABLE)	FRAME_BUTTONS[35].button->setSelected(true);
			else								FRAME_BUTTONS[36].button->setSelected(true);
			FRAME_BUTTONS[37].buttonString = FRAME_STRINGS[38+iVolume];
			for (int i = 31; i < 38; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			break;
		case SUBMENU_SAVES:
			setDefaultFocus(FRAME_BUTTONS[4].button);
			for (int i = 0; i < NUM_TAB_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[38].button);
				FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[44].button);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			for (int i = 14; i < 17; i++)
				FRAME_TEXTBOXES[i].textBox->setVisible(true);
			FRAME_BUTTONS[4].button->setSelected(true);
			FRAME_BUTTONS[38+nativeSaveDevice].button->setSelected(true);
			if (autoSave == AUTOSAVE_ENABLE)	FRAME_BUTTONS[42].button->setSelected(true);
			else								FRAME_BUTTONS[43].button->setSelected(true);
			if (saveStateDevice == SAVESTATEDEVICE_SD)	FRAME_BUTTONS[44].button->setSelected(true);
			else										FRAME_BUTTONS[45].button->setSelected(true);
			for (int i = 38; i < NUM_FRAME_BUTTONS; i++)
			{
				FRAME_BUTTONS[i].button->setVisible(true);
				FRAME_BUTTONS[i].button->setActive(true);
			}
			break;
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
	for (int i = 5; i <= 6; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[5].button->setSelected(true);

	int needInit = 0;
	if(hasLoadedISO && dynacore != DYNACORE_INTERPRETER){ SysClose(); needInit = 1; }
	dynacore = DYNACORE_INTERPRETER;
	if(hasLoadedISO && needInit) {
  	SysInit();
  	CheckCdrom();
    SysReset();
  	LoadCdrom();
  	Func_SetPlayGame();
  }
}

void Func_CpuDynarec()
{
	for (int i = 5; i <= 6; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[6].button->setSelected(true);

	int needInit = 0;
	if(hasLoadedISO && dynacore != DYNACORE_DYNAREC){ SysClose(); needInit = 1; }
	dynacore = DYNACORE_DYNAREC;
	if(hasLoadedISO && needInit) {
  	SysInit ();
  	CheckCdrom();
    SysReset();
  	LoadCdrom();
  	Func_SetPlayGame();
  }
}

void Func_BiosSelectHLE()
{
	for (int i = 7; i <= 9; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[7].button->setSelected(true);

	biosDevice = BIOSDEVICE_HLE;
}

void Func_BiosSelectSD()
{
	for (int i = 7; i <= 9; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[8].button->setSelected(true);

	biosDevice = BIOSDEVICE_SD;
}

void Func_BiosSelectUSB()
{
	for (int i = 7; i <= 9; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[9].button->setSelected(true);

	biosDevice = BIOSDEVICE_USB;
}

void Func_BootBiosYes()
{
	if(biosDevice == BIOSDEVICE_HLE) {
    menu::MessageBox::getInstance().setMessage("You must select a BIOS, not HLE");
    return;
  }
  
  for (int i = 10; i <= 11; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[10].button->setSelected(true);
  
	LoadCdBios = BOOTTHRUBIOS_YES;
}

void Func_BootBiosNo()
{
	for (int i = 10; i <= 11; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[11].button->setSelected(true);

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
  if(isoFile)
    free(isoFile);
  //TODO: load/save memcards here
	SysInit ();
	CheckCdrom();
	SysReset();
	pauseRemovalThread();
	resumeAudio();
	//resumeInput();
	menuActive = 0;
  SysStartCPU();
  menuActive = 1;
	//pauseInput();
	pauseAudio();
	continueRemovalThread();
}

extern void writeConfig(FILE* f);

void Func_SaveSettingsSD()
{
  fileBrowser_file* configFile_file;
  int (*configFile_init)(fileBrowser_file*) = fileBrowser_libfat_init;
  configFile_file = &saveDir_libfat_Default;
  if(configFile_init(configFile_file)) {                //only if device initialized ok
    FILE* f = fopen( "sd:/wiiSX/settings.cfg", "wb" );  //attempt to open file
    if(f) {
//      writeConfig(f);                                   //write out the config
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
    FILE* f = fopen( "usb:/wiiSX/settings.cfg", "wb" ); //attempt to open file
    if(f) {
//      writeConfig(f);                                   //write out the config
      fclose(f);
      menu::MessageBox::getInstance().setMessage("Saved settings.cfg to USB");
      return;
    }
  }
	menu::MessageBox::getInstance().setMessage("Error saving settings.cfg to USB");
}

void Func_ShowFpsOn()
{
	for (int i = 15; i <= 16; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[15].button->setSelected(true);
	showFPSonScreen = FPS_SHOW;
}

void Func_ShowFpsOff()
{
	for (int i = 15; i <= 16; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[16].button->setSelected(true);
	showFPSonScreen = FPS_HIDE;
}

extern "C" void GPUsetframelimit(unsigned long option);

void Func_FpsLimitAuto()
{
	for (int i = 17; i <= 18; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[17].button->setSelected(true);
	frameLimit = FRAMELIMIT_AUTO;
	GPUsetframelimit(0);
}

void Func_FpsLimitOff()
{
	for (int i = 17; i <= 18; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[18].button->setSelected(true);
	frameLimit = FRAMELIMIT_NONE;
	GPUsetframelimit(0);
}

void Func_FrameSkipOn()
{
	for (int i = 19; i <= 20; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[19].button->setSelected(true);
	frameSkip = FRAMESKIP_ENABLE;
	GPUsetframelimit(0);
}

void Func_FrameSkipOff()
{
	for (int i = 19; i <= 20; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[20].button->setSelected(true);
	frameSkip = FRAMESKIP_DISABLE;
	GPUsetframelimit(0);
}

void Func_ScreenMode4_3()
{
	for (int i = 21; i <= 22; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[21].button->setSelected(true);
	screenMode = SCREENMODE_4x3;
}

void Func_ScreenMode16_9()
{
	for (int i = 21; i <= 22; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[22].button->setSelected(true);
	screenMode = SCREENMODE_16x9;
}

void Func_DitheringNone()
{
	for (int i = 23; i <= 25; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[23].button->setSelected(true);
	iUseDither = USEDITHER_NONE;
	GPUsetframelimit(0);
}

void Func_DitheringDefault()
{
	for (int i = 23; i <= 25; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[24].button->setSelected(true);
	iUseDither = USEDITHER_DEFAULT;
	GPUsetframelimit(0);
}

void Func_DitheringAlways()
{
	for (int i = 23; i <= 25; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[25].button->setSelected(true);
	iUseDither = USEDITHER_ALWAYS;
	GPUsetframelimit(0);
}

void Func_ScalingNone()
{
	//TODO: Implement 2xSaI scaling and then make it an option.
}

void Func_Scaling2xSai()
{
	//TODO: Implement 2xSaI scaling and then make it an option.
}

void Func_AssignInput()
{
//	menu::MessageBox::getInstance().setMessage("Input configuration not implemented");
	pMenuContext->setActiveFrame(MenuContext::FRAME_CONFIGUREINPUT,ConfigureInputFrame::SUBMENU_REINIT);
}

void Func_InputOptions()
{
	menu::MessageBox::getInstance().setMessage("Input Options not implemented");
}

void Func_MapButtons()
{
	menu::MessageBox::getInstance().setMessage("Button Mapping not implemented");
}

void Func_DisableAudioYes()
{
	for (int i = 31; i <= 32; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[31].button->setSelected(true);
	audioEnabled = AUDIO_DISABLE;
}

void Func_DisableAudioNo()
{
	for (int i = 31; i <= 32; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[32].button->setSelected(true);
	audioEnabled = AUDIO_ENABLE;
}

void Func_DisableXaYes()
{
	for (int i = 33; i <= 34; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[33].button->setSelected(true);
	Config.Xa = XA_DISABLE;
}

void Func_DisableXaNo()
{
	for (int i = 33; i <= 34; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[34].button->setSelected(true);
	Config.Xa = XA_ENABLE;
}

void Func_DisableCddaYes()
{
	for (int i = 35; i <= 36; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[35].button->setSelected(true);
	Config.Cdda = CDDA_DISABLE;
}

void Func_DisableCddaNo()
{
	for (int i = 35; i <= 36; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[36].button->setSelected(true);
	Config.Cdda = CDDA_ENABLE;
}

void Func_VolumeToggle()
{
	iVolume--;
	if (iVolume<1)
		iVolume = 4;
	FRAME_BUTTONS[37].buttonString = FRAME_STRINGS[38+iVolume];
}

void Func_MemcardSaveSD()
{
	for (int i = 38; i <= 41; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[38].button->setSelected(true);
	nativeSaveDevice = NATIVESAVEDEVICE_SD;
}

void Func_MemcardSaveUSB()
{
	for (int i = 38; i <= 41; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[39].button->setSelected(true);
	nativeSaveDevice = NATIVESAVEDEVICE_USB;
}

void Func_MemcardSaveCardA()
{
	for (int i = 38; i <= 41; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[40].button->setSelected(true);
	nativeSaveDevice = NATIVESAVEDEVICE_CARDA;
}

void Func_MemcardSaveCardB()
{
	for (int i = 38; i <= 41; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[41].button->setSelected(true);
	nativeSaveDevice = NATIVESAVEDEVICE_CARDB;
}

void Func_AutoSaveYes()
{
	for (int i = 42; i <= 43; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[42].button->setSelected(true);
	autoSave = AUTOSAVE_ENABLE;
}

void Func_AutoSaveNo()
{
	for (int i = 42; i <= 43; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[43].button->setSelected(true);
	autoSave = AUTOSAVE_DISABLE;
}

void Func_SaveStateSD()
{
	for (int i = 44; i <= 45; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[44].button->setSelected(true);
	saveStateDevice = SAVESTATEDEVICE_SD;
}

void Func_SaveStateUSB()
{
	for (int i = 44; i <= 45; i++)
		FRAME_BUTTONS[i].button->setSelected(false);
	FRAME_BUTTONS[45].button->setSelected(true);
	saveStateDevice = SAVESTATEDEVICE_USB;
}

void Func_ReturnFromSettingsFrame()
{
	menu::Gui::getInstance().menuLogo->setLocation(580.0, 70.0, -50.0);
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}

