/**
 * WiiSX - controller-GC.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009, 2010 sepp256
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * Gamecube controller input module
 *
 * WiiSX homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *                sepp256@gmail.com
 *                emukidid@gmail.com
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

#ifdef __SWITCH__
#include <switch.h>
#include <string.h>
#include "controller.h"
#include "../wiiSXconfig.h"

enum {
	ANALOG_AS_ANALOG = 1, C_STICK_AS_ANALOG = 2,
};

enum {
	ANALOG_L         = 0x01 << 16,
	ANALOG_R         = 0x02 << 16,
	ANALOG_U         = 0x04 << 16,
	ANALOG_D         = 0x08 << 16,
	C_STICK_L        = 0x10 << 16,
	C_STICK_R        = 0x20 << 16,
	C_STICK_U        = 0x40 << 16,
	C_STICK_D        = 0x80 << 16,
	PAD_TRIGGER_Z_UP = 0x0100 << 16,
};

static button_t buttons[] = {
	{  0, ~0,               "None" },
	{  1, KEY_DUP,          "D-Up" },
	{  2, KEY_DLEFT,        "D-Left" },
	{  3, KEY_DRIGHT,       "D-Right" },
	{  4, KEY_DDOWN,        "D-Down" },
	{  5, KEY_L|KEY_MINUS, "L+Select" },
	{  6, KEY_R|KEY_MINUS, "R+Select" },
	{  7, KEY_L,        		"L" },
	{  8, KEY_R,       		"R" },
	{  9, KEY_A,            "A" },
	{ 10, KEY_B,            "B" },
	{ 11, KEY_X,            "X" },
	{ 12, KEY_Y,            "Y" },
	{ 13, KEY_PLUS,		"Start" },
	{ 14, KEY_MINUS,  		"Select" },
	{ 15, KEY_RSTICK_UP,      "A-Up" },
	{ 16, KEY_RSTICK_LEFT,    "A-Left" },
	{ 17, KEY_RSTICK_RIGHT,   "A-Right" },
	{ 18, KEY_RSTICK_DOWN,    "A-Down" },
};

static button_t analog_sources[] = {
	{ 0, ANALOG_AS_ANALOG,  "Analog Stick" }
};

static button_t menu_combos[] = {
	{ 0, KEY_X|KEY_Y, "X+Y" },
	{ 1, KEY_PLUS|KEY_X, "Start+X" },
};

static u64 getButtons(int Control)
{
	return hidKeysHeld(CONTROLLER_P1_AUTO);
}

static int _GetKeys(int Control, BUTTONS * Keys, controller_config_t* config)
{
	hidScanInput();
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));
	//Reset buttons & sticks
	c->btns.All = 0xFFFF;
	c->leftStickX = c->leftStickY = c->rightStickX = c->rightStickY = 128;

	if (!controller_SWITCH.available[Control]) return 0;

	u64 b = getButtons(Control);
	inline int isHeld(button_tp button){
		return (b & button->mask) == button->mask ? 0 : 1;
	}
	
	c->btns.SQUARE_BUTTON    = isHeld(config->SQU);
	c->btns.CROSS_BUTTON     = isHeld(config->CRO);
	c->btns.CIRCLE_BUTTON    = isHeld(config->CIR);
	c->btns.TRIANGLE_BUTTON  = isHeld(config->TRI);

	c->btns.R1_BUTTON    = isHeld(config->R1);
	c->btns.L1_BUTTON    = isHeld(config->L1);
	c->btns.R2_BUTTON    = isHeld(config->R2);
	c->btns.L2_BUTTON    = isHeld(config->L2);

	c->btns.L_DPAD       = isHeld(config->DL);
	c->btns.R_DPAD       = isHeld(config->DR);
	c->btns.U_DPAD       = isHeld(config->DU);
	c->btns.D_DPAD       = isHeld(config->DD);

	c->btns.START_BUTTON  = isHeld(config->START);
	c->btns.R3_BUTTON    = isHeld(config->R3);
	c->btns.L3_BUTTON    = isHeld(config->L3);
	c->btns.SELECT_BUTTON = isHeld(config->SELECT);

	//adjust values by 128 cause PSX sticks range 0-255 with a 128 center pos
	JoystickPosition pos_left, pos_right;

	//Read the joysticks' position
	hidJoystickRead(&pos_left, CONTROLLER_P1_AUTO, JOYSTICK_LEFT);
	hidJoystickRead(&pos_right, CONTROLLER_P1_AUTO, JOYSTICK_RIGHT);
		
	c->leftStickX = pos_left.dx;
	c->leftStickY = pos_left.dy;
	c->rightStickX = pos_right.dx;
	c->rightStickY = pos_right.dy;

	// Return 1 if whether the exit button(s) are pressed
	return isHeld(config->exit) ? 0 : 1;
}

static void pause(int Control){}

static void resume(int Control){ }

static void rumble(int Control, int rumble){}

static void configure(int Control, controller_config_t* config){}

static void assign(int p, int v){}

static void refreshAvailable(void);

controller_t controller_SWITCH =
	{ 'G',
	  _GetKeys,
	  configure,
	  assign,
	  pause,
	  resume,
	  rumble,
	  refreshAvailable,
	  {0, 0, 0, 0},
	  sizeof(buttons)/sizeof(buttons[0]),
	  buttons,
	  sizeof(analog_sources)/sizeof(analog_sources[0]),
	  analog_sources,
	  sizeof(menu_combos)/sizeof(menu_combos[0]),
	  menu_combos,
	  { .SQU        = &buttons[12], // Y
	    .CRO        = &buttons[10],  // B
	    .CIR        = &buttons[9], // A
	    .TRI        = &buttons[11], // X
	    .R1         = &buttons[8],  // R
	    .L1         = &buttons[7],  // L
	    .R2         = &buttons[6],  // R + Select
	    .L2         = &buttons[5],  // L + Select
	    .R3         = &buttons[0],  // None
	    .L3         = &buttons[0],  // None
	    .DL         = &buttons[2],  // D-Pad Left
	    .DR         = &buttons[3],  // D-Pad Right
	    .DU         = &buttons[1],  // D-Pad Up
	    .DD         = &buttons[4],  // D-Pad Down
	    .START      = &buttons[13], // Start
	    .SELECT     = &buttons[14], // Select
	    .analogL    = &analog_sources[0], // Analog Stick
	    .analogR    = &analog_sources[0], // C stick
	    .exit       = &menu_combos[1], // Start+X
	    .invertedYL = 0,
	    .invertedYR = 0,
	  }
	 };


static void refreshAvailable(void){

	controller_SWITCH.available[0] = 1;
}
#endif
