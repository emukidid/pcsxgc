/**
 * WiiSX - ConfigureInputFrame.cpp
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
#include "ConfigureInputFrame.h"
#include "../libgui/Button.h"
#include "../libgui/TextBox.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
//#include "../libgui/MessageBox.h"
//#include "../main/timers.h"
//#include "../main/wii64config.h"

extern "C" {
#include "../gc_input/controller.h"
}

void Func_AutoSelectInput();
void Func_ManualSelectInput();

void Func_TogglePad0Type();
void Func_TogglePad1Type();
void Func_TogglePad2Type();
void Func_TogglePad3Type();
void Func_TogglePad0Assign();
void Func_TogglePad1Assign();
void Func_TogglePad2Assign();
void Func_TogglePad3Assign();

void Func_ReturnFromConfigureInputFrame();


#define NUM_FRAME_BUTTONS 10
#define FRAME_BUTTONS configureInputFrameButtons
#define FRAME_STRINGS configureInputFrameStrings
#define NUM_FRAME_TEXTBOXES 5
#define FRAME_TEXTBOXES configureInputFrameTextBoxes

static const char FRAME_STRINGS[16][15] =
	{ "Pad Assignment",
	  "PSX Pad 1",
	  "PSX Pad 2",
	  "PSX Pad 3",
	  "PSX Pad 4",

	  "Automatic",
	  "Manual",
	  "None",
	  "Gamecube Pad",
	  "Wii Pad",
	  "Auto Assign",
	  "",
	  "1",
	  "2",
	  "3",
	  "4"};

struct ButtonInfo
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
} FRAME_BUTTONS[NUM_FRAME_BUTTONS] =
{ //	button	buttonStyle buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc				returnFunc
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[5],	240.0,	 80.0,	135.0,	56.0,	 8,	 2,	 1,	 1,	Func_AutoSelectInput,	Func_ReturnFromConfigureInputFrame }, // Automatic Pad Assignment 0
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[6],	395.0,	 80.0,	120.0,	56.0,	 9,	 4,	 0,	 0,	Func_ManualSelectInput,	Func_ReturnFromConfigureInputFrame }, // Manual Pad Assignment 1
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[10],	240.0,	150.0,	200.0,	56.0,	 0,	 3,	 4,	 4,	Func_TogglePad0Type,	Func_ReturnFromConfigureInputFrame }, // Toggle Pad 0 Type 2
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[10],	240.0,	220.0,	200.0,	56.0,	 2,	 6,	 5,	 5,	Func_TogglePad1Type,	Func_ReturnFromConfigureInputFrame }, // Toggle Pad 1 Type 3
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[11],	460.0,	150.0,	 55.0,	56.0,	 1,	 5,	 2,	 2,	Func_TogglePad0Assign,	Func_ReturnFromConfigureInputFrame }, // Toggle Pad 0 Assignment 4
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[11],	460.0,	220.0,	 55.0,	56.0,	 4,	 8,	 3,	 3,	Func_TogglePad1Assign,	Func_ReturnFromConfigureInputFrame }, // Toggle Pad 1 Assignment 5
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[10],	240.0,	290.0,	200.0,	56.0,	 3,	 7,	 8,	 8,	Func_TogglePad2Type,	Func_ReturnFromConfigureInputFrame }, // Toggle Pad 2 Type 6
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[10],	240.0,	360.0,	200.0,	56.0,	 6,	 0,	 9,	 9,	Func_TogglePad3Type,	Func_ReturnFromConfigureInputFrame }, // Toggle Pad 3 Type 7
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[11],	460.0,	290.0,	 55.0,	56.0,	 5,	 9,	 6,	 6,	Func_TogglePad2Assign,	Func_ReturnFromConfigureInputFrame }, // Toggle Pad 2 Assignment 8
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[11],	460.0,	360.0,	 55.0,	56.0,	 8,	 1,	 7,	 7,	Func_TogglePad3Assign,	Func_ReturnFromConfigureInputFrame }, // Toggle Pad 3 Assignment 9
	

};

struct TextBoxInfo
{
	menu::TextBox	*textBox;
	const char*		textBoxString;
	float			x;
	float			y;
	float			scale;
	bool			centered;
} FRAME_TEXTBOXES[NUM_FRAME_TEXTBOXES] =
{ //	textBox	textBoxString		x		y		scale	centered
	{	NULL,	FRAME_STRINGS[0],	125.0,	108.0,	 1.0,	true }, // Pad Assignment
	{	NULL,	FRAME_STRINGS[1],	125.0,	178.0,	 1.0,	true }, // Pad 1
	{	NULL,	FRAME_STRINGS[2],	125.0,	248.0,	 1.0,	true }, // Pad 2
	{	NULL,	FRAME_STRINGS[3],	125.0,	318.0,	 1.0,	true }, // Pad 3
	{	NULL,	FRAME_STRINGS[4],	125.0,	388.0,	 1.0,	true }, // Pad 4
};

ConfigureInputFrame::ConfigureInputFrame()
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
	setBackFunc(Func_ReturnFromConfigureInputFrame);
	setEnabled(true);
	activateSubmenu(SUBMENU_REINIT);
}

ConfigureInputFrame::~ConfigureInputFrame()
{
	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
		delete FRAME_TEXTBOXES[i].textBox;
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}

}

void ConfigureInputFrame::activateSubmenu(int submenu)
{
	if (padAutoAssign == PADAUTOASSIGN_AUTOMATIC)
	{
		FRAME_BUTTONS[0].button->setSelected(true);
		FRAME_BUTTONS[1].button->setSelected(false);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_DOWN, NULL);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_UP, NULL);
		FRAME_BUTTONS[1].button->setNextFocus(menu::Focus::DIRECTION_DOWN, NULL);
		FRAME_BUTTONS[1].button->setNextFocus(menu::Focus::DIRECTION_UP, NULL);
		
		FRAME_BUTTONS[2].button->setActive(false);
		FRAME_BUTTONS[2].buttonString = FRAME_STRINGS[10];
		FRAME_BUTTONS[4].button->setActive(false);
		FRAME_BUTTONS[4].buttonString = FRAME_STRINGS[11];
		FRAME_BUTTONS[3].button->setActive(false);
		FRAME_BUTTONS[3].buttonString = FRAME_STRINGS[10];
		FRAME_BUTTONS[5].button->setActive(false);
		FRAME_BUTTONS[5].buttonString = FRAME_STRINGS[11];
		FRAME_BUTTONS[6].button->setActive(false);
		FRAME_BUTTONS[6].buttonString = FRAME_STRINGS[10];
		FRAME_BUTTONS[8].button->setActive(false);
		FRAME_BUTTONS[8].buttonString = FRAME_STRINGS[11];
		FRAME_BUTTONS[7].button->setActive(false);
		FRAME_BUTTONS[7].buttonString = FRAME_STRINGS[10];
		FRAME_BUTTONS[9].button->setActive(false);
		FRAME_BUTTONS[9].buttonString = FRAME_STRINGS[11];


	}
	else
	{
		FRAME_BUTTONS[0].button->setSelected(false);
		FRAME_BUTTONS[1].button->setSelected(true);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[2].button);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[7].button);
		FRAME_BUTTONS[1].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[4].button);
		FRAME_BUTTONS[1].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[9].button);

		FRAME_BUTTONS[2].button->setActive(true);
		FRAME_BUTTONS[2].buttonString = FRAME_STRINGS[padType[0]+7];
		FRAME_BUTTONS[4].button->setActive(true);
		FRAME_BUTTONS[4].buttonString = FRAME_STRINGS[padAssign[0]+12];
		FRAME_BUTTONS[3].button->setActive(true);
		FRAME_BUTTONS[3].buttonString = FRAME_STRINGS[padType[1]+7];
		FRAME_BUTTONS[5].button->setActive(true);
		FRAME_BUTTONS[5].buttonString = FRAME_STRINGS[padAssign[1]+12];
		FRAME_BUTTONS[6].button->setActive(true);
		FRAME_BUTTONS[6].buttonString = FRAME_STRINGS[padType[2]+7];
		FRAME_BUTTONS[8].button->setActive(true);
		FRAME_BUTTONS[8].buttonString = FRAME_STRINGS[padAssign[2]+12];
		FRAME_BUTTONS[7].button->setActive(true);
		FRAME_BUTTONS[7].buttonString = FRAME_STRINGS[padType[3]+7];
		FRAME_BUTTONS[9].button->setActive(true);
		FRAME_BUTTONS[9].buttonString = FRAME_STRINGS[padAssign[3]+12];

	}
}

extern MenuContext *pMenuContext;

void Func_AutoSelectInput()
{
	padAutoAssign = PADAUTOASSIGN_AUTOMATIC;
	pMenuContext->getFrame(MenuContext::FRAME_CONFIGUREINPUT)->activateSubmenu(ConfigureInputFrame::SUBMENU_REINIT);
}

void Func_ManualSelectInput()
{
	padAutoAssign = PADAUTOASSIGN_MANUAL;
	pMenuContext->getFrame(MenuContext::FRAME_CONFIGUREINPUT)->activateSubmenu(ConfigureInputFrame::SUBMENU_REINIT);
}

void Func_AssignPad(int i)
{
	controller_t* type = NULL;
	switch (padType[i])
	{
	case PADTYPE_GAMECUBE:
		type = &controller_GC;
		break;
#ifdef HW_RVL
	case PADTYPE_WII:
		//Note: Wii expansion detection is done in InputStatusBar.cpp during MainFrame draw
		if (controller_Classic.available[(int)padAssign[i]])
			type = &controller_Classic;
		else
			type = &controller_WiimoteNunchuk;
		break;
#endif
	}
	assign_controller(i, type, (int) padAssign[i]);
}

void Func_TogglePadType(int i) {
#ifdef HW_RVL
	padType[i] = (padType[i]+1) %3;
#else
	padType[i] = (padType[i]+1) %2;
#endif

	if (padType[i]) Func_AssignPad(i);
	else			unassign_controller(i);
	pMenuContext->getFrame(MenuContext::FRAME_CONFIGUREINPUT)->activateSubmenu(ConfigureInputFrame::SUBMENU_REINIT);
}

void Func_TogglePad0Type()
{
	Func_TogglePadType(PADASSIGN_INPUT0);
}

void Func_TogglePad1Type()
{
	Func_TogglePadType(PADASSIGN_INPUT1);
}

void Func_TogglePad2Type()
{
	Func_TogglePadType(PADASSIGN_INPUT2);
}

void Func_TogglePad3Type()
{
	Func_TogglePadType(PADASSIGN_INPUT3);
}

void Func_TogglePadAssign(int i)
{
	padAssign[i] = (padAssign[i]+1) %4;

	if (padType[i]) Func_AssignPad(i);

	pMenuContext->getFrame(MenuContext::FRAME_CONFIGUREINPUT)->activateSubmenu(ConfigureInputFrame::SUBMENU_REINIT);
}

void Func_TogglePad0Assign()
{
	Func_TogglePadAssign(PADASSIGN_INPUT0);
}

void Func_TogglePad1Assign()
{
	Func_TogglePadAssign(PADASSIGN_INPUT1);
}

void Func_TogglePad2Assign()
{
	Func_TogglePadAssign(PADASSIGN_INPUT2);
}

void Func_TogglePad3Assign()
{
	Func_TogglePadAssign(PADASSIGN_INPUT3);
}

void Func_ReturnFromConfigureInputFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_INPUT);
}
