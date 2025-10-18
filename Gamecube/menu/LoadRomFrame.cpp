/**
 * WiiSX - LoadRomFrame.cpp
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
#include "LoadRomFrame.h"
#include "FileBrowserFrame.h"
#include "../libgui/Button.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"

extern "C" {
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "../fileBrowser/fileBrowser-SMB.h"
#include "../fileBrowser/fileBrowser-CARD.h"
void LoadingBar_showBar(float percent, const char* string);
}

void Func_LoadFromSD();
void Func_LoadFromDVD();
void Func_LoadFromUSB();
void Func_LoadFromSamba();
void Func_ReturnFromLoadRomFrame();

#ifdef HW_RVL
#define NUM_FRAME_BUTTONS 4
#else
#define NUM_FRAME_BUTTONS 2
#endif
#define FRAME_BUTTONS loadRomFrameButtons
#define FRAME_STRINGS loadRomFrameStrings

#ifdef HW_RVL
static const char FRAME_STRINGS[4][25] =
	{ "Load from SD",
	  "Load from DVD",
	  "Load from USB",
	  "Load from Samba"};
#else
static const char FRAME_STRINGS[2][25] =
	{ "Load from FAT",
	  "Load from DVD"};
#endif

struct ButtonInfoWithHint
{
	menu::Button	*button;
	int				buttonStyle;
	const char*		buttonString;
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
	const char*		buttonHint;
} FRAME_BUTTONS[NUM_FRAME_BUTTONS] =
{ //	button	buttonStyle	buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc			returnFunc
#ifdef HW_RVL
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[0],	150.0,	100.0,	340.0,	56.0,	 3,	 1,	-1,	-1,	Func_LoadFromSD,	Func_ReturnFromLoadRomFrame, "Front SD, Slot A, Slot B are supported (in that order)\nFAT32 only" },
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[1],	150.0,	180.0,	340.0,	56.0,	 0,	 2,	-1,	-1,	Func_LoadFromDVD,	Func_ReturnFromLoadRomFrame, "Requires a compatible disc drive/console\nISO9660 discs only" }, // Load From DVD
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[2],	150.0,	260.0,	340.0,	56.0,	 1,	 3,	-1,	-1,	Func_LoadFromUSB,	Func_ReturnFromLoadRomFrame, "USB Storage must be FAT32 formatted" }, // Load From USB
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[3],	150.0,	340.0,	340.0,	56.0,	 2,	 0,	-1,	-1,	Func_LoadFromSamba,	Func_ReturnFromLoadRomFrame, "Requires setup in settings.cfg" }, // Load From Samba
#else
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[0],	150.0,	180.0,	340.0,	56.0,	 1,	 1,	-1,	-1,	Func_LoadFromSD,	Func_ReturnFromLoadRomFrame, "M2Loader, GCLoader, SD2SP2, SlotA/B are supported (in that order)\nFAT32 only" },
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[1],	150.0,	260.0,	340.0,	56.0,	 0,	 0,	-1,	-1,	Func_LoadFromDVD,	Func_ReturnFromLoadRomFrame, "Requires a compatible disc drive/console\nISO9660 discs only" } // Load From DVD
#endif
};

LoadRomFrame::LoadRomFrame()
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
		FRAME_BUTTONS[i].button = new menu::Button(FRAME_BUTTONS[i].buttonStyle, &FRAME_BUTTONS[i].buttonString, 
										FRAME_BUTTONS[i].x, FRAME_BUTTONS[i].y, 
										FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].height, &FRAME_BUTTONS[i].buttonHint);

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
	setBackFunc(Func_ReturnFromLoadRomFrame);
	setEnabled(true);

}

LoadRomFrame::~LoadRomFrame()
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}

}

int loadRomMode = FileBrowserFrame::FILEBROWSER_LOADISO;

void LoadRomFrame::activateSubmenu(int submenu)
{
	loadRomMode = submenu;
}


extern MenuContext *pMenuContext;
extern void fileBrowserFrame_OpenDirectory(fileBrowser_file* dir);

void Func_LoadFromSD()
{
	LoadingBar_showBar(1.0f, "Reading directory ...");
	// Deinit any existing romFile state
	if(isoFile_deinit) isoFile_deinit( &isoFile );
	// Change all the romFile pointers
	isoFile_topLevel = &topLevel_libfat_Default;
	isoFile_readDir  = fileBrowser_libfat_readDir;
	isoFile_open     = fileBrowser_libfat_open;
	isoFile_seekFile = fileBrowser_libfat_seekFile;
	isoFile_init     = fileBrowser_libfat_init;
	isoFile_deinit   = fileBrowser_libfatROM_deinit;
	// Make sure the romFile system is ready before we browse the filesystem
	isoFile_init( isoFile_topLevel );

	pMenuContext->setActiveFrame(MenuContext::FRAME_FILEBROWSER,loadRomMode);
	fileBrowserFrame_OpenDirectory(isoFile_topLevel);
}

void Func_LoadFromDVD()
{
	LoadingBar_showBar(1.0f, "Reading directory ...");
	// Deinit any existing romFile state
	if(isoFile_deinit) isoFile_deinit( &isoFile );
	// Change all the romFile pointers
	isoFile_topLevel = &topLevel_DVD;
	isoFile_readDir  = fileBrowser_libfat_readDir;
	isoFile_open     = fileBrowser_libfat_open;
	isoFile_seekFile = fileBrowser_libfat_seekFile;
	isoFile_init     = fileBrowser_libfat_init;
	isoFile_deinit   = fileBrowser_libfat_deinit;
	// Make sure the romFile system is ready before we browse the filesystem
	isoFile_init( isoFile_topLevel );

	pMenuContext->setActiveFrame(MenuContext::FRAME_FILEBROWSER,loadRomMode);
	fileBrowserFrame_OpenDirectory(isoFile_topLevel);
}

#ifdef WII
void Func_LoadFromUSB()
{
	LoadingBar_showBar(1.0f, "Reading directory ...");
	// Deinit any existing romFile state
	if(isoFile_deinit) isoFile_deinit( &isoFile );
	// Change all the romFile pointers
	isoFile_topLevel = &topLevel_libfat_USB;
	isoFile_readDir  = fileBrowser_libfat_readDir;
	isoFile_open     = fileBrowser_libfat_open;
	isoFile_seekFile = fileBrowser_libfat_seekFile;
	isoFile_init     = fileBrowser_libfat_init;
	isoFile_deinit   = fileBrowser_libfatROM_deinit;
	// Make sure the romFile system is ready before we browse the filesystem
	isoFile_init( isoFile_topLevel );
	
	pMenuContext->setActiveFrame(MenuContext::FRAME_FILEBROWSER,loadRomMode);
	fileBrowserFrame_OpenDirectory(isoFile_topLevel);
}
#endif

#ifdef HW_RVL
void Func_LoadFromSamba()
{
	LoadingBar_showBar(1.0f, "Reading directory ...");
	// Deinit any existing romFile state
	if(isoFile_deinit) isoFile_deinit( &isoFile );
	// Change all the romFile pointers
	isoFile_topLevel = &topLevel_SMB;
	isoFile_readDir  = fileBrowser_SMB_readDir;
	isoFile_open     = fileBrowser_SMB_open;
	isoFile_seekFile = fileBrowser_SMB_seekFile;
	isoFile_init     = fileBrowser_SMB_init;
	isoFile_deinit   = fileBrowser_SMB_deinit;
	// Make sure the romFile system is ready before we browse the filesystem
	isoFile_init( isoFile_topLevel );
	
	pMenuContext->setActiveFrame(MenuContext::FRAME_FILEBROWSER,loadRomMode);
	fileBrowserFrame_OpenDirectory(isoFile_topLevel);
}
#endif

void Func_ReturnFromLoadRomFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}
