/**
 * Wii64 - ConfigureInputFrame.cpp
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
//*#include "../gc_input/controller.h"
}

void Func_AutoSelectInput();
void Func_ManualSelectInput();

void Func_TogglePad0Type();
void Func_TogglePad1Type();
void Func_TogglePad0Assign();
void Func_TogglePad1Assign();

void Func_ReturnFromConfigureInputFrame();


#define NUM_FRAME_BUTTONS 6
#define FRAME_BUTTONS configureInputFrameButtons
#define FRAME_STRINGS configureInputFrameStrings
#define NUM_FRAME_TEXTBOXES 3
#define FRAME_TEXTBOXES configureInputFrameTextBoxes

static char FRAME_STRINGS[16][15] =
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
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[5],	240.0,	 80.0,	135.0,	56.0,	 5,	 2,	 1,	 1,	Func_AutoSelectInput,	Func_ReturnFromConfigureInputFrame }, // Automatic Pad Assignment
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[6],	395.0,	 80.0,	120.0,	56.0,	 9,	 6,	 0,	 0,	Func_ManualSelectInput,	Func_ReturnFromConfigureInputFrame }, // Manual Pad Assignment
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[10],	240.0,	150.0,	200.0,	56.0,	 0,	 3,	 4,	 4,	Func_TogglePad0Type,	Func_ReturnFromConfigureInputFrame }, // Toggle Pad 0 Type
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[10],	240.0,	220.0,	200.0,	56.0,	 2,	 0,	 5,	 5,	Func_TogglePad1Type,	Func_ReturnFromConfigureInputFrame }, // Toggle Pad 1 Type
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[11],	460.0,	150.0,	 55.0,	56.0,	 1,	 5,	 2,	 2,	Func_TogglePad0Assign,	Func_ReturnFromConfigureInputFrame }, // Toggle Pad 0 Assignment
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[11],	460.0,	220.0,	 55.0,	56.0,	 4,	 1,	 3,	 3,	Func_TogglePad1Assign,	Func_ReturnFromConfigureInputFrame }, // Toggle Pad 1 Assignment

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
	{	NULL,	FRAME_STRINGS[0],	125.0,	108.0,	 1.0,	true }, // Pad Assignment
	{	NULL,	FRAME_STRINGS[1],	125.0,	178.0,	 1.0,	true }, // Pad 1
	{	NULL,	FRAME_STRINGS[2],	125.0,	248.0,	 1.0,	true }, // Pad 2
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
		for (int i = 0; i < 2; i++)
		{
			FRAME_BUTTONS[i+2].button->setActive(false);
			FRAME_BUTTONS[i+2].buttonString = FRAME_STRINGS[10];
			FRAME_BUTTONS[i+4].button->setActive(false);
			FRAME_BUTTONS[i+4].buttonString = FRAME_STRINGS[11];
		}
	}
	else
	{
		FRAME_BUTTONS[0].button->setSelected(false);
		FRAME_BUTTONS[1].button->setSelected(true);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[2].button);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[3].button);
		FRAME_BUTTONS[1].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[4].button);
		FRAME_BUTTONS[1].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[5].button);
		for (int i = 0; i < 2; i++)
		{
			FRAME_BUTTONS[i+2].button->setActive(true);
			FRAME_BUTTONS[i+2].buttonString = FRAME_STRINGS[padType[i]+7];
			FRAME_BUTTONS[i+4].button->setActive(true);
			FRAME_BUTTONS[i+4].buttonString = FRAME_STRINGS[padAssign[i]+12];
		}
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
/*	controller_t* type = NULL;
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
*/}

void Func_TogglePad0Type()
{
	int i = PADASSIGN_INPUT0;
#ifdef HW_RVL
	padType[i] = (padType[i]+1) %3;
#else
	padType[i] = (padType[i]+1) %2;
#endif

	if (padType[i]) Func_AssignPad(i);
//*	else			unassign_controller(i);
	pMenuContext->getFrame(MenuContext::FRAME_CONFIGUREINPUT)->activateSubmenu(ConfigureInputFrame::SUBMENU_REINIT);
}

void Func_TogglePad1Type()
{
	int i = PADASSIGN_INPUT1;
#ifdef HW_RVL
	padType[i] = (padType[i]+1) %3;
#else
	padType[i] = (padType[i]+1) %2;
#endif

	if (padType[i]) Func_AssignPad(i);
//*	else			unassign_controller(i);
	pMenuContext->getFrame(MenuContext::FRAME_CONFIGUREINPUT)->activateSubmenu(ConfigureInputFrame::SUBMENU_REINIT);
}

void Func_TogglePad0Assign()
{
	int i = PADASSIGN_INPUT0;
	padAssign[i] = (padAssign[i]+1) %4;

	if (padType[i]) Func_AssignPad(i);

	pMenuContext->getFrame(MenuContext::FRAME_CONFIGUREINPUT)->activateSubmenu(ConfigureInputFrame::SUBMENU_REINIT);
}

void Func_TogglePad1Assign()
{
	int i = PADASSIGN_INPUT1;
	padAssign[i] = (padAssign[i]+1) %4;

	if (padType[i]) Func_AssignPad(i);

	pMenuContext->getFrame(MenuContext::FRAME_CONFIGUREINPUT)->activateSubmenu(ConfigureInputFrame::SUBMENU_REINIT);
}

void Func_ReturnFromConfigureInputFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_INPUT);
}
