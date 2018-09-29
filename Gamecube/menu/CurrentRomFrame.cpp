/**
 * Wii64 - CurrentRomFrame.cpp
 * Copyright (C) 2009 sepp256
 *
 * Wii64 homepage: http://www.emulatemii.com
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
#include "CurrentRomFrame.h"
#include "../libgui/Button.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"
#include "../wiiSXconfig.h"
#include "../../PsxCommon.h"

extern "C" {
/*#include "../gc_memory/memory.h"
#include "../gc_memory/Saves.h"
#include "../main/rom.h"
#include "../main/plugin.h"
#include "../main/savestates.h"*/
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "../fileBrowser/fileBrowser-CARD.h"
extern int LoadMcd(int mcd, fileBrowser_file *savepath);
extern int SaveMcd(int mcd, fileBrowser_file *savepath);
extern s32 CDR_getTN(unsigned char *buffer);
}

void Func_ShowRomInfo();
void Func_ResetROM();
void Func_SwapCD();
void Func_LoadSave();
void Func_SaveGame();
void Func_LoadState();
void Func_SaveState();
void Func_StateCycle();
void Func_ReturnFromCurrentRomFrame();

#define NUM_FRAME_BUTTONS 8
#define FRAME_BUTTONS currentRomFrameButtons
#define FRAME_STRINGS currentRomFrameStrings

/* Button Layout:
 * [Restart Game] [Swap CD]
 * [Load MemCard] [Save MemCard]
 * [Show ISO Info]
 * [Load State] [Slot "x"]
 * [Save State]
 */

static char FRAME_STRINGS[8][15] =
	{ "Restart Game",
	  "Swap CD",
	  "Load MemCards",
	  "Save MemCards",
	  "Show ISO Info",
	  "Load State",
	  "Save State",
	  "Slot 0"};

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
{ //	button	buttonStyle	buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc			returnFunc
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[0],	100.0,	 60.0,	210.0,	56.0,	 6,	 2,	 1,	 1,	Func_ResetROM,		Func_ReturnFromCurrentRomFrame }, // Reset ROM
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[1],	330.0,	 60.0,	210.0,	56.0,	 7,	 3,	 0,	 0,	Func_SwapCD,		Func_ReturnFromCurrentRomFrame }, // Swap CD
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[2],	100.0,	120.0,	210.0,	56.0,	 0,	 4,	 3,	 3,	Func_LoadSave,		Func_ReturnFromCurrentRomFrame }, // Load MemCards
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[3],	330.0,	120.0,	210.0,	56.0,	 1,	 4,	 2,	 2,	Func_SaveGame,		Func_ReturnFromCurrentRomFrame }, // Save MemCards
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[4],	150.0,	180.0,	340.0,	56.0,	 2,	 5,	-1,	-1,	Func_ShowRomInfo,	Func_ReturnFromCurrentRomFrame }, // Show ISO Info
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[5],	150.0,	240.0,	220.0,	56.0,	 4,	 6,	 7,	 7,	Func_LoadState,		Func_ReturnFromCurrentRomFrame }, // Load State 
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[6],	150.0,	300.0,	220.0,	56.0,	 5,	 0,	 7,	 7,	Func_SaveState,		Func_ReturnFromCurrentRomFrame }, // Save State 
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[7],	390.0,	270.0,	100.0,	56.0,	 4,	 1,	 5,	 5,	Func_StateCycle,	Func_ReturnFromCurrentRomFrame }, // Cycle State 
};

CurrentRomFrame::CurrentRomFrame()
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
	setDefaultFocus(FRAME_BUTTONS[0].button);
	setBackFunc(Func_ReturnFromCurrentRomFrame);
	setEnabled(true);

}

CurrentRomFrame::~CurrentRomFrame()
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}

}

extern MenuContext *pMenuContext;
extern char CdromId[10];
extern char CdromLabel[33];
extern "C" {
s32 MoobyCDRgetTN(unsigned char *buffer);
};

void Func_ShowRomInfo()
{
	char RomInfo[256] = "";
	char buffer [50];
	
	sprintf(buffer,"CD-ROM Label: %s\n",CdromLabel);
  strcat(RomInfo,buffer);
  sprintf(buffer,"CD-ROM ID: %s\n", CdromId);
  strcat(RomInfo,buffer);
  sprintf(buffer,"ISO Size: %d Mb\n",isoFile.size/1024/1024);
  strcat(RomInfo,buffer);
  sprintf(buffer,"Country: %s\n",(!Config.PsxType) ? "NTSC":"PAL");
  strcat(RomInfo,buffer);
  sprintf(buffer,"BIOS: %s\n",(Config.HLE==BIOS_USER_DEFINED) ? "USER DEFINED":"HLE");
  strcat(RomInfo,buffer);
  /*unsigned char tracks[2];
  CDR_getTN(&tracks[0]);
  sprintf(buffer,"Number of tracks %i\n", tracks[1]);
  strcat(RomInfo,buffer);*/

	menu::MessageBox::getInstance().setMessage(RomInfo);
}

extern BOOL hasLoadedISO;

extern "C" {
void SysReset();
void SysInit();
void SysClose();
void CheckCdrom();
void LoadCdrom();
}

void Func_SetPlayGame();

void Func_ResetROM()
{
  SysClose();
	SysInit ();
	CheckCdrom();
  SysReset();
	LoadCdrom();
	menu::MessageBox::getInstance().setMessage("Game restarted");
	Func_SetPlayGame();
}

void Func_SwapCD()
{
	//Call Filebrowser with "Swap CD"
	pMenuContext->setActiveFrame(MenuContext::FRAME_LOADROM,FileBrowserFrame::FILEBROWSER_SWAPCD);
}

extern "C" char mcd1Written;
extern "C" char mcd2Written;
extern "C" int LoadState();
extern "C" int SaveState();
extern "C" void savestates_select_slot(unsigned int s);

void Func_LoadSave()
{
	if(!hasLoadedISO)
	{
		menu::MessageBox::getInstance().setMessage("Please load a ISO first");
		return;
	}

	switch (nativeSaveDevice)
  {
  	case NATIVESAVEDEVICE_SD:
  	case NATIVESAVEDEVICE_USB:
  		// Adjust saveFile pointers
  		saveFile_dir = (nativeSaveDevice==NATIVESAVEDEVICE_SD) ? &saveDir_libfat_Default:&saveDir_libfat_USB;
  		saveFile_readFile  = fileBrowser_libfat_readFile;
  		saveFile_writeFile = fileBrowser_libfat_writeFile;
  		saveFile_init      = fileBrowser_libfat_init;
  		saveFile_deinit    = fileBrowser_libfat_deinit;
  		break;
  	case NATIVESAVEDEVICE_CARDA:
  	case NATIVESAVEDEVICE_CARDB:
  		// Adjust saveFile pointers
  		saveFile_dir       = (nativeSaveDevice==NATIVESAVEDEVICE_CARDA) ? &saveDir_CARD_SlotA:&saveDir_CARD_SlotB;
  		saveFile_readFile  = fileBrowser_CARD_readFile;
  		saveFile_writeFile = fileBrowser_CARD_writeFile;
  		saveFile_init      = fileBrowser_CARD_init;
  		saveFile_deinit    = fileBrowser_CARD_deinit;
  		break;
  }

  // Try loading everything
  int result = 0;
  saveFile_init(saveFile_dir);
  result += LoadMcd(1,saveFile_dir);
  result += LoadMcd(2,saveFile_dir);
  saveFile_deinit(saveFile_dir);

	switch (nativeSaveDevice)
	{
		case NATIVESAVEDEVICE_SD:
			if (result) menu::MessageBox::getInstance().setMessage("Loaded save from SD card");
			else		menu::MessageBox::getInstance().setMessage("No saves found on SD card");
			break;
		case NATIVESAVEDEVICE_USB:
			if (result) menu::MessageBox::getInstance().setMessage("Loaded save from USB device");
			else		menu::MessageBox::getInstance().setMessage("No saves found on USB device");
			break;
		case NATIVESAVEDEVICE_CARDA:
			if (result) menu::MessageBox::getInstance().setMessage("Loaded save from memcard in slot A");
			else		menu::MessageBox::getInstance().setMessage("No saves found on memcard A");
			break;
		case NATIVESAVEDEVICE_CARDB:
			if (result) menu::MessageBox::getInstance().setMessage("Loaded save from memcard in slot A");
			else		menu::MessageBox::getInstance().setMessage("No saves found on memcard B");
			break;
	}
	mcd1Written = mcd2Written = false;
}

void Func_SaveGame()
{
  if(!mcd1Written && !mcd2Written) {
    menu::MessageBox::getInstance().setMessage("Nothing to save");
    return;
  }
	switch (nativeSaveDevice)
  {
  	case NATIVESAVEDEVICE_SD:
  	case NATIVESAVEDEVICE_USB:
  		// Adjust saveFile pointers
  		saveFile_dir = (nativeSaveDevice==NATIVESAVEDEVICE_SD) ? &saveDir_libfat_Default:&saveDir_libfat_USB;
  		saveFile_readFile  = fileBrowser_libfat_readFile;
  		saveFile_writeFile = fileBrowser_libfat_writeFile;
  		saveFile_init      = fileBrowser_libfat_init;
  		saveFile_deinit    = fileBrowser_libfat_deinit;
  		break;
  	case NATIVESAVEDEVICE_CARDA:
  	case NATIVESAVEDEVICE_CARDB:
  		// Adjust saveFile pointers
  		saveFile_dir       = (nativeSaveDevice==NATIVESAVEDEVICE_CARDA) ? &saveDir_CARD_SlotA:&saveDir_CARD_SlotB;
  		saveFile_readFile  = fileBrowser_CARD_readFile;
  		saveFile_writeFile = fileBrowser_CARD_writeFile;
  		saveFile_init      = fileBrowser_CARD_init;
  		saveFile_deinit    = fileBrowser_CARD_deinit;
  		break;
  }

	// Try saving everything
	int amountSaves = mcd1Written + mcd2Written;
	int result = 0;
  saveFile_init(saveFile_dir);
  result += SaveMcd(1,saveFile_dir);
  result += SaveMcd(2,saveFile_dir);
  saveFile_deinit(saveFile_dir);

	if (result>=amountSaves) {	
		switch (nativeSaveDevice)
		{
			case NATIVESAVEDEVICE_SD:
				menu::MessageBox::getInstance().setMessage("Saved game to SD card");
				break;
			case NATIVESAVEDEVICE_USB:
				menu::MessageBox::getInstance().setMessage("Saved game to USB device");
				break;
			case NATIVESAVEDEVICE_CARDA:
				menu::MessageBox::getInstance().setMessage("Saved game to memcard in Slot A");
				break;
			case NATIVESAVEDEVICE_CARDB:
				menu::MessageBox::getInstance().setMessage("Saved game to memcard in Slot B");
				break;
		}
		mcd1Written = mcd2Written = false;
	}
	else		menu::MessageBox::getInstance().setMessage("Failed to Save");

}

void Func_LoadState()
{
  if(LoadState()) {
    menu::MessageBox::getInstance().setMessage("Save State Loaded Successfully");
  } else {
    menu::MessageBox::getInstance().setMessage("Save doesn't exist");
  }
}

void Func_SaveState()
{
  if(SaveState()) {
    menu::MessageBox::getInstance().setMessage("Save State Saved Successfully");
  } else {
    menu::MessageBox::getInstance().setMessage("Error Saving State");
  }
}

static unsigned int which_slot = 0;

void Func_StateCycle()
{
	
	which_slot = (which_slot+1) %10;
	savestates_select_slot(which_slot);
	FRAME_STRINGS[7][5] = which_slot + '0';

}

void Func_ReturnFromCurrentRomFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}
