/**
 * WiiSX - ConfigureButtonsFrame.cpp
 * Copyright (C) 2010 sepp256
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
#include "ConfigureButtonsFrame.h"
#include "../libgui/GuiTypes.h"
#include "../libgui/GuiResources.h"
#include "../libgui/Button.h"
#include "../libgui/TextBox.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"

extern "C" {
#include "../gc_input/controller.h"
}

void Func_NextPad();
void Func_DefaultConfig();
void Func_ToggleConfigSlot();
void Func_LoadConfig();
void Func_SaveConfig();

void Func_ToggleButtonExit();
void Func_ToggleButtonL1();
void Func_ToggleButtonL2();
void Func_ToggleButtonSelect();
void Func_ToggleButtonStart();
void Func_ToggleButtonR1();
void Func_ToggleButtonR2();
void Func_ToggleButtonDup();
void Func_ToggleButtonDleft();
void Func_ToggleButtonDright();
void Func_ToggleButtonDdown();
void Func_ToggleButtonTri();
void Func_ToggleButtonSqu();
void Func_ToggleButtonCir();
void Func_ToggleButtonCro();
void Func_ToggleAnalogL();
void Func_ToggleAnalogR();
void Func_ToggleInvertYL();
void Func_ToggleInvertYR();
void Func_ToggleButtonL3();
void Func_ToggleButtonR3();

void Func_ReturnFromConfigureButtonsFrame();


#define NUM_FRAME_BUTTONS 26
#define FRAME_BUTTONS configureButtonsFrameButtons
#define FRAME_STRINGS configureButtonsFrameStrings
#define NUM_FRAME_TEXTBOXES 2
#define FRAME_TEXTBOXES configureButtonsFrameTextBoxes
#define TITLE_STRING configureButtonsTitleString

static char FRAME_STRINGS[27][20] =
	{ "Next Pad",
	  "Load Default",
	  "Slot 1",
	  "Load",
	  "Save",
	  "X+Y",
	  "Btn L1",
	  "Btn L2",
	  "Btn Select",
	  "Btn Start",
	  "Btn R1",
	  "Btn R2",
	  "Btn Dup",
	  "Btn Dleft",
	  "Btn Dright",
	  "Btn Ddown",
	  "Btn Tri",
	  "Btn Squ",
	  "Btn Cir",
	  "Btn Cro",
	  "Analog Stick L",
	  "Analog Stick R",
	  "Invert Y L",
	  "Invert Y R",
	  "Btn L3",
	  "Btn R3",
	  "Menu:"};

static char TITLE_STRING[64] = "Gamecube Pad 1 to PSX Pad 1 Mapping";

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
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[0],	 40.0,	 80.0,	115.0,	40.0,	22,	 5,	 4,	 1,	Func_NextPad,			Func_ReturnFromConfigureButtonsFrame }, // Next Pad
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[1],	175.0,	 80.0,	145.0,	40.0,	24,	 8,	 0,	 2,	Func_DefaultConfig,		Func_ReturnFromConfigureButtonsFrame }, // Restore Default Config
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[2],	340.0,	 80.0,	 80.0,	40.0,	25,	 9,	 1,	 3,	Func_ToggleConfigSlot,	Func_ReturnFromConfigureButtonsFrame }, // Cycle Through Config Slots
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[3],	440.0,	 80.0,	 70.0,	40.0,	23,	10,	 2,	 4,	Func_LoadConfig,		Func_ReturnFromConfigureButtonsFrame }, // Load Config
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[4],	530.0,	 80.0,	 70.0,	40.0,	23,	10,	 3,	 0,	Func_SaveConfig,		Func_ReturnFromConfigureButtonsFrame }, // Save Config

	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[5],	 30.0,	160.0,	100.0,	40.0,	 0,	12,	10,	 6,	Func_ToggleButtonExit,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button Exit

	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[6],	140.0,	130.0,	 80.0,	40.0,	 0,	 7,	 5,	 8,	Func_ToggleButtonL1,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button L1
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[7],	140.0,	180.0,	 80.0,	40.0,	 6,	12,	 5,	 8,	Func_ToggleButtonL2,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button L2
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[8],	235.0,	140.0,	 80.0,	40.0,	 1,	20,	 6,	 9,	Func_ToggleButtonSelect,Func_ReturnFromConfigureButtonsFrame }, // Toggle Button Select
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[9],	325.0,	140.0,	 80.0,	40.0,	 2,	21,	 8,	10,	Func_ToggleButtonStart,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button Start
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[10],	420.0,	130.0,	 80.0,	40.0,	 3,	11,	 9,	 5,	Func_ToggleButtonR1,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button R1
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[11],	420.0,	180.0,	 80.0,	40.0,	10,	16,	 9,	 5,	Func_ToggleButtonR2,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button R2

	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[12],	 75.0,	230.0,	 80.0,	40.0,	 5,	13,	16,	16,	Func_ToggleButtonDup,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button D-up
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[13],	 30.0,	280.0,	 80.0,	40.0,	12,	15,	18,	14,	Func_ToggleButtonDleft,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button D-left
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[14],	120.0,	280.0,	 80.0,	40.0,	12,	15,	13,	17,	Func_ToggleButtonDright,Func_ReturnFromConfigureButtonsFrame }, // Toggle Button D-right
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[15],	 75.0,	330.0,	 80.0,	40.0,	13,	22,	19,	20,	Func_ToggleButtonDdown,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button D-down

	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[16],	485.0,	230.0,	 80.0,	40.0,	11,	17,	12,	12,	Func_ToggleButtonTri,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button Triangle
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[17],	440.0,	280.0,	 80.0,	40.0,	16,	19,	14,	18,	Func_ToggleButtonSqu,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button Square
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[18],	530.0,	280.0,	 80.0,	40.0,	16,	19,	17,	13,	Func_ToggleButtonCir,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button Circle
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[19],	485.0,	330.0,	 80.0,	40.0,	17,	23,	21,	15,	Func_ToggleButtonCro,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button Cross

	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[20],	165.0,	345.0,	150.0,	40.0,	 8,	24,	15,	21,	Func_ToggleAnalogL,		Func_ReturnFromConfigureButtonsFrame }, // Toggle Analog Stick L
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[21],	325.0,	345.0,	150.0,	40.0,	 9,	25,	20,	19,	Func_ToggleAnalogR,		Func_ReturnFromConfigureButtonsFrame }, // Toggle Analog Stick R
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[22],	 95.0,	395.0,	130.0,	40.0,	15,	 0,	23,	24,	Func_ToggleInvertYL,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Analog Invert Y L
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[23],	415.0,	395.0,	130.0,	40.0,	19,	 3,	25,	22,	Func_ToggleInvertYR,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Analog Invert Y R
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[24],	235.0,	395.0,	 80.0,	40.0,	20,	 1,	22,	25,	Func_ToggleButtonL3,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button L3
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[25],	325.0,	395.0,	 80.0,	40.0,	21,	 2,	24,	23,	Func_ToggleButtonR3,	Func_ReturnFromConfigureButtonsFrame }, // Toggle Button R3
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
	{	NULL,	TITLE_STRING,		320.0,	 50.0,	 1.0,	true }, // ______ Pad X to N64 Pad Y Mapping
	{	NULL,	FRAME_STRINGS[26],	 80.0,	145.0,	 0.9,	true }, // Menu Combo
};

ConfigureButtonsFrame::ConfigureButtonsFrame()
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
	for (int i = 0; i < 5; i++)
		FRAME_BUTTONS[i].button->setFontSize(0.8);
	for (int i = 5; i < NUM_FRAME_BUTTONS; i++)
		FRAME_BUTTONS[i].button->setFontSize(0.7);

	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
	{
		FRAME_TEXTBOXES[i].textBox = new menu::TextBox(&FRAME_TEXTBOXES[i].textBoxString, 
										FRAME_TEXTBOXES[i].x, FRAME_TEXTBOXES[i].y, 
										FRAME_TEXTBOXES[i].scale, FRAME_TEXTBOXES[i].centered);
		add(FRAME_TEXTBOXES[i].textBox);
	}

	setDefaultFocus(FRAME_BUTTONS[0].button);
	setBackFunc(Func_ReturnFromConfigureButtonsFrame);
	setEnabled(true);
	activateSubmenu(SUBMENU_PSX_PADNONE);
}

ConfigureButtonsFrame::~ConfigureButtonsFrame()
{
	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
		delete FRAME_TEXTBOXES[i].textBox;
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}

}

static char controllerTypeStrings[5][17] =
	{ "Gamecube",
	  "Classic",
	  "Wiimote+Nunchuck",
	  "Wiimote",
	  "NULL"};

enum ActivePadType
{
	ACTIVEPADTYPE_GAMECUBE=0,
	ACTIVEPADTYPE_CLASSIC,
	ACTIVEPADTYPE_WIIMOTENUNCHUCK,
	ACTIVEPADTYPE_WIIMOTE,
	ACTIVEPADTYPE_NONE,
};

enum ActivePadAssigned
{
	ACTIVEPADASSIGNED_FALSE=0,
	ACTIVEPADASSIGNED_TRUE,
};

int activePad=ConfigureButtonsFrame::SUBMENU_PSX_PAD0;
int activePadType=ACTIVEPADTYPE_NONE;
int activePadAssigned=ACTIVEPADASSIGNED_TRUE;

void ConfigureButtonsFrame::activateSubmenu(int submenu)
{
	controller_config_t* currentConfig;

	if (submenu != SUBMENU_PSX_PADNONE)
	{
		activePad = submenu;
		menu::Gui::getInstance().menuLogo->setVisible(false);
	}

	//Fill out title text
	if (virtualControllers[activePad].control == &controller_GC)
		activePadType = ACTIVEPADTYPE_GAMECUBE;
#ifdef HW_RVL
	else if (virtualControllers[activePad].control == &controller_Classic)
		activePadType = ACTIVEPADTYPE_CLASSIC;
	else if (virtualControllers[activePad].control == &controller_WiimoteNunchuk)
		activePadType = ACTIVEPADTYPE_WIIMOTENUNCHUCK;
	else if (virtualControllers[activePad].control == &controller_Wiimote)
		activePadType = ACTIVEPADTYPE_WIIMOTE;
#endif //HW_RVL
	else
		activePadType = ACTIVEPADTYPE_NONE;

	if (activePadType == ACTIVEPADTYPE_NONE)
	{
		sprintf(TITLE_STRING, "PSX Pad %d: No Physical Controller Assigned", activePad+1 );

		if (activePadAssigned == ACTIVEPADASSIGNED_TRUE) //Reset to "Next Pad" button
		{
			activePadAssigned = ACTIVEPADASSIGNED_FALSE;
			menu::Focus::getInstance().clearPrimaryFocus();
		}
		for (int i = 5; i < NUM_FRAME_BUTTONS; i++)
			strcpy(FRAME_STRINGS[i], "");
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_UP, NULL);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_DOWN, NULL);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_LEFT, NULL);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_RIGHT, NULL);
		for (int i = 1; i < NUM_FRAME_BUTTONS; i++)
			FRAME_BUTTONS[i].button->setActive(false);
	}
	else
	{
		sprintf(TITLE_STRING, "PSX Pad %d: %s Pad %d Mapping", activePad+1, controllerTypeStrings[activePadType], virtualControllers[activePad].number+1 );
	
		currentConfig = virtualControllers[activePad].config;

		if (activePadAssigned == ACTIVEPADASSIGNED_FALSE) //Reset to "Next Pad" button
		{
			activePadAssigned = ACTIVEPADASSIGNED_TRUE;
		}

		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[FRAME_BUTTONS[0].focusUp].button);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[FRAME_BUTTONS[0].focusDown].button);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_LEFT, FRAME_BUTTONS[FRAME_BUTTONS[0].focusLeft].button);
		FRAME_BUTTONS[0].button->setNextFocus(menu::Focus::DIRECTION_RIGHT, FRAME_BUTTONS[FRAME_BUTTONS[0].focusRight].button);
		for (int i = 1; i < NUM_FRAME_BUTTONS; i++)
			FRAME_BUTTONS[i].button->setActive(true);

		//Assign text to each button
		strcpy(FRAME_STRINGS[5], currentConfig->exit->name);
		strcpy(FRAME_STRINGS[6], currentConfig->L1->name);
		strcpy(FRAME_STRINGS[7], currentConfig->L2->name);
		strcpy(FRAME_STRINGS[8], currentConfig->SELECT->name);
		strcpy(FRAME_STRINGS[9], currentConfig->START->name);
		strcpy(FRAME_STRINGS[10], currentConfig->R1->name);
		strcpy(FRAME_STRINGS[11], currentConfig->R2->name);
		strcpy(FRAME_STRINGS[12], currentConfig->DU->name);
		strcpy(FRAME_STRINGS[13], currentConfig->DL->name);
		strcpy(FRAME_STRINGS[14], currentConfig->DR->name);
		strcpy(FRAME_STRINGS[15], currentConfig->DD->name);
		strcpy(FRAME_STRINGS[16], currentConfig->TRI->name);
		strcpy(FRAME_STRINGS[17], currentConfig->SQU->name);
		strcpy(FRAME_STRINGS[18], currentConfig->CIR->name);
		strcpy(FRAME_STRINGS[19], currentConfig->CRO->name);
		strcpy(FRAME_STRINGS[20], currentConfig->analogL->name);
		strcpy(FRAME_STRINGS[21], currentConfig->analogR->name);
		if (currentConfig->invertedYL)
			strcpy(FRAME_STRINGS[22], "Inverted Y");
		else
			strcpy(FRAME_STRINGS[22], "Normal Y");
		if (currentConfig->invertedYR)
			strcpy(FRAME_STRINGS[23], "Inverted Y");
		else
			strcpy(FRAME_STRINGS[23], "Normal Y");
		strcpy(FRAME_STRINGS[24], currentConfig->L3->name);
		strcpy(FRAME_STRINGS[25], currentConfig->R3->name);
	}
}

void ConfigureButtonsFrame::updateFrame(float deltaTime)
{
	activateSubmenu(activePad);
}

#define NUM_LINES 10

void ConfigureButtonsFrame::drawChildren(menu::Graphics &gfx)
{
	if(isVisible())
	{
		int base_x = 204;
		int base_y = 188;
		int lines[NUM_LINES][4] = {{164, 265, 221, 244}, //D-pad (17,56)
								   {476, 265, 419, 244}, //Tri,Squ,Cir,Cro btns (215,56)
								   {275, 160, 302, 240}, //SELECT (98,52)
								   {365, 160, 338, 242}, //START (134,54)
								   {220, 170, 242, 192}, //L1 (38,4)
								   {420, 170, 398, 192}, //R1 (194,4)
								   {220, 200, 232, 204}, //L2 (28,16)
								   {420, 200, 408, 204}, //R2 (204,16)
								   {260, 345, 278, 294}, //AnalogL (74,106)
								   {380, 345, 362, 294}};//AnalogR (158,106)

		GXColor controllerColors[6] = {	{  1,  29, 169, 255}, //blue
										{254,  32,  21, 255}, //orange/red
										{  8, 147,  48, 255}, //green
										{255, 192,   1, 255}, //yellow/gold
										{150, 150, 255, 255}, //light blue lines
										{255, 255, 255, 255}};//white

		//Draw PSX Controller
		menu::Image* controllerIcon = NULL;
//		gfx.setColor(controllerColors[activePad]);
		gfx.setColor(controllerColors[5]);
		controllerIcon = menu::Resources::getInstance().getImage(menu::Resources::IMAGE_PSX_CONTROLLER);
		controllerIcon->activateImage(GX_TEXMAP0);
//		GX_SetTevColorIn(GX_TEVSTAGE0,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,GX_CC_RASC);
		GX_SetTevColorIn(GX_TEVSTAGE0,GX_CC_ZERO,GX_CC_TEXC,GX_CC_RASC,GX_CC_ZERO);
		GX_SetTevColorOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
		GX_SetTevAlphaIn(GX_TEVSTAGE0,GX_CA_ZERO,GX_CA_RASA,GX_CA_TEXA,GX_CA_ZERO);
		GX_SetTevAlphaOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
		gfx.enableBlending(true);
		gfx.drawImage(0, base_x, base_y, 232, 152, 0, 1, 0, 1);
		gfx.setTEV(GX_PASSCLR);

		//Draw lines and circles
		gfx.setColor(controllerColors[5]);
		gfx.setLineWidth(1);
		gfx.drawCircle(115, 300, 60, 33);
		gfx.drawCircle(525, 300, 60, 33);

		for (int i=0; i<NUM_LINES; i++)
			gfx.drawLine(lines[i][0], lines[i][1], lines[i][2], lines[i][3]);

		//Draw buttons
		menu::ComponentList::const_iterator iteration;
		for (iteration = componentList.begin(); iteration != componentList.end(); iteration++)
		{
			(*iteration)->draw(gfx);
		}
	}
}

extern MenuContext *pMenuContext;

void Func_NextPad()
{
	activePad = (activePad+1) %2;

	pMenuContext->getFrame(MenuContext::FRAME_CONFIGUREBUTTONS)->activateSubmenu(activePad);
}

void Func_DefaultConfig()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	memcpy(currentConfig, &virtualControllers[activePad].control->config_default, sizeof(controller_config_t));

	pMenuContext->getFrame(MenuContext::FRAME_CONFIGUREBUTTONS)->activateSubmenu(activePad);
}

static unsigned int which_slot = 0;

void Func_ToggleConfigSlot()
{
	which_slot = (which_slot+1) %4;
	FRAME_STRINGS[2][5] = which_slot + '1';
}

void Func_LoadConfig()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	memcpy(currentConfig, &virtualControllers[activePad].control->config_slot[which_slot], sizeof(controller_config_t));

	pMenuContext->getFrame(MenuContext::FRAME_CONFIGUREBUTTONS)->activateSubmenu(activePad);
}

void Func_SaveConfig()
{
	char buffer [50] = "";
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	memcpy(&virtualControllers[activePad].control->config_slot[which_slot], currentConfig, sizeof(controller_config_t));

	//todo: save button configuration to file here

	sprintf(buffer,"Saved current button mapping to slot %d.",which_slot+1);
	menu::MessageBox::getInstance().fadeMessage(buffer);
//	menu::MessageBox::getInstance().setMessage(buffer);
}

void Func_ToggleButtonExit()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->exit->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_menu_combos;
	currentConfig->exit = &virtualControllers[activePad].control->menu_combos[currentButton];
	strcpy(FRAME_STRINGS[5], currentConfig->exit->name);
}

void Func_ToggleButtonL1()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->L1->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->L1 = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[6], currentConfig->L1->name);
}

void Func_ToggleButtonL2()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->L2->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->L2 = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[7], currentConfig->L2->name);
}

void Func_ToggleButtonSelect()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->SELECT->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->SELECT = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[8], currentConfig->SELECT->name);
}

void Func_ToggleButtonStart()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->START->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->START = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[9], currentConfig->START->name);
}

void Func_ToggleButtonR1()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->R1->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->R1 = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[10], currentConfig->R1->name);
}

void Func_ToggleButtonR2()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->R2->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->R2 = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[11], currentConfig->R2->name);
}

void Func_ToggleButtonDup()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->DU->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->DU = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[12], currentConfig->DU->name);
}

void Func_ToggleButtonDleft()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->DL->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->DL = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[13], currentConfig->DL->name);
}

void Func_ToggleButtonDright()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->DR->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->DR = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[14], currentConfig->DR->name);
}

void Func_ToggleButtonDdown()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->DD->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->DD = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[15], currentConfig->DD->name);
}

void Func_ToggleButtonTri()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->TRI->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->TRI = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[16], currentConfig->TRI->name);
}

void Func_ToggleButtonSqu()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->SQU->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->SQU = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[17], currentConfig->SQU->name);
}

void Func_ToggleButtonCir()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->CIR->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->CIR = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[18], currentConfig->CIR->name);
}

void Func_ToggleButtonCro()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->CRO->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->CRO = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[19], currentConfig->CRO->name);
}

void Func_ToggleAnalogL()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->analogL->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_analog_sources;
	currentConfig->analogL = &virtualControllers[activePad].control->analog_sources[currentButton];
	strcpy(FRAME_STRINGS[20], currentConfig->analogL->name);
}

void Func_ToggleAnalogR()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->analogR->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_analog_sources;
	currentConfig->analogR = &virtualControllers[activePad].control->analog_sources[currentButton];
	strcpy(FRAME_STRINGS[21], currentConfig->analogR->name);
}

void Func_ToggleInvertYL()
{
	int invertedY = virtualControllers[activePad].config->invertedYL;
	if (invertedY)
	{
		virtualControllers[activePad].config->invertedYL = 0;
		strcpy(FRAME_STRINGS[22], "Normal Y");
	}
	else
	{
		virtualControllers[activePad].config->invertedYL = 1;
		strcpy(FRAME_STRINGS[22], "Inverted Y");
	}
}

void Func_ToggleInvertYR()
{
	int invertedY = virtualControllers[activePad].config->invertedYR;
	if (invertedY)
	{
		virtualControllers[activePad].config->invertedYR = 0;
		strcpy(FRAME_STRINGS[23], "Normal Y");
	}
	else
	{
		virtualControllers[activePad].config->invertedYR = 1;
		strcpy(FRAME_STRINGS[23], "Inverted Y");
	}
}

void Func_ToggleButtonL3()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->L3->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->L3 = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[24], currentConfig->L3->name);
}

void Func_ToggleButtonR3()
{
	controller_config_t* currentConfig = virtualControllers[activePad].config;
	int currentButton = currentConfig->R3->index;
	currentButton = (currentButton+1) %virtualControllers[activePad].control->num_buttons;
	currentConfig->R3 = &virtualControllers[activePad].control->buttons[currentButton];
	strcpy(FRAME_STRINGS[25], currentConfig->R3->name);
}

void Func_ReturnFromConfigureButtonsFrame()
{
	menu::Gui::getInstance().menuLogo->setVisible(true);
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_INPUT);
}
