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
}

void Func_LoadFromSD();
void Func_LoadFromDVD();
void Func_LoadFromUSB();
void Func_LoadFromSamba();
void Func_ReturnFromLoadRomFrame();

#define NUM_FRAME_BUTTONS 4
#define FRAME_BUTTONS loadRomFrameButtons
#define FRAME_STRINGS loadRomFrameStrings

static char FRAME_STRINGS[4][25] =
	{ "Load from SD",
	  "Load from DVD",
	  "Load from USB",
	  "Load from Samba"};

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
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[0],	150.0,	100.0,	340.0,	56.0,	 3,	 1,	-1,	-1,	Func_LoadFromSD,	Func_ReturnFromLoadRomFrame }, // Load From SD
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[1],	150.0,	180.0,	340.0,	56.0,	 0,	 2,	-1,	-1,	Func_LoadFromDVD,	Func_ReturnFromLoadRomFrame }, // Load From DVD
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[2],	150.0,	260.0,	340.0,	56.0,	 1,	 3,	-1,	-1,	Func_LoadFromUSB,	Func_ReturnFromLoadRomFrame }, // Load From USB
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[3],	150.0,	340.0,	340.0,	56.0,	 2,	 0,	-1,	-1,	Func_LoadFromSamba,	Func_ReturnFromLoadRomFrame }, // Load From Samba
};

LoadRomFrame::LoadRomFrame()
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
	// Deinit any existing romFile state
	if(isoFile_deinit) isoFile_deinit( &isoFile );
	// Change all the romFile pointers
	isoFile_topLevel = &topLevel_libfat_Default;
	isoFile_readDir  = fileBrowser_libfat_readDir;
	isoFile_open     = fileBrowser_libfat_open;
	isoFile_readFile = fileBrowser_libfatROM_readFile;
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
	// Deinit any existing romFile state
	if(isoFile_deinit) isoFile_deinit( &isoFile );
	// Change all the romFile pointers
	isoFile_topLevel = &topLevel_DVD;
	isoFile_readDir  = fileBrowser_libfat_readDir;
	isoFile_open     = fileBrowser_libfat_open;
	isoFile_readFile = fileBrowser_libfat_readFile;
	isoFile_seekFile = fileBrowser_libfat_seekFile;
	isoFile_init     = fileBrowser_libfat_init;
	isoFile_deinit   = fileBrowser_libfat_deinit;
	// Make sure the romFile system is ready before we browse the filesystem
	isoFile_init( isoFile_topLevel );

	pMenuContext->setActiveFrame(MenuContext::FRAME_FILEBROWSER,loadRomMode);
	fileBrowserFrame_OpenDirectory(isoFile_topLevel);
}

void Func_LoadFromUSB()
{
#ifdef WII
	// Deinit any existing romFile state
	if(isoFile_deinit) isoFile_deinit( &isoFile );
	// Change all the romFile pointers
	isoFile_topLevel = &topLevel_libfat_USB;
	isoFile_readDir  = fileBrowser_libfat_readDir;
	isoFile_open     = fileBrowser_libfat_open;
	isoFile_readFile = fileBrowser_libfatROM_readFile;
	isoFile_seekFile = fileBrowser_libfat_seekFile;
	isoFile_init     = fileBrowser_libfat_init;
	isoFile_deinit   = fileBrowser_libfatROM_deinit;
	// Make sure the romFile system is ready before we browse the filesystem
	isoFile_init( isoFile_topLevel );
	
	pMenuContext->setActiveFrame(MenuContext::FRAME_FILEBROWSER,loadRomMode);
	fileBrowserFrame_OpenDirectory(isoFile_topLevel);
#else
	menu::MessageBox::getInstance().setMessage("Available only for Wii");
#endif
}

void Func_LoadFromSamba()
{
#ifdef HW_RVL
	// Deinit any existing romFile state
	if(isoFile_deinit) isoFile_deinit( &isoFile );
	// Change all the romFile pointers
	isoFile_topLevel = &topLevel_SMB;
	isoFile_readDir  = fileBrowser_SMB_readDir;
	isoFile_open     = fileBrowser_SMB_open;
	isoFile_readFile = fileBrowser_SMB_readFile;
	isoFile_seekFile = fileBrowser_SMB_seekFile;
	isoFile_init     = fileBrowser_SMB_init;
	isoFile_deinit   = fileBrowser_SMB_deinit;
	// Make sure the romFile system is ready before we browse the filesystem
	isoFile_init( isoFile_topLevel );
	
	pMenuContext->setActiveFrame(MenuContext::FRAME_FILEBROWSER,loadRomMode);
	fileBrowserFrame_OpenDirectory(isoFile_topLevel);
#else
	menu::MessageBox::getInstance().setMessage("Available only for Wii");
#endif
}

void Func_ReturnFromLoadRomFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}
