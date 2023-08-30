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


#include <string.h>
#include <ogc/pad.h>
#include "controller.h"
#include "../wiiSXconfig.h"
extern char controllerType;

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
	{  0, ~0,                                "None" },
	{  1, PAD_BUTTON_UP,                     "D-Up" },
	{  2, PAD_BUTTON_LEFT,                   "D-Left" },
	{  3, PAD_BUTTON_RIGHT,                  "D-Right" },
	{  4, PAD_BUTTON_DOWN,                   "D-Down" },
	{  5, PAD_TRIGGER_L|PAD_TRIGGER_Z_UP,    "L-Z" },
	{  6, PAD_TRIGGER_R|PAD_TRIGGER_Z_UP,    "R-Z" },
	{  7, PAD_TRIGGER_L|PAD_TRIGGER_Z,       "L+Z" },
	{  8, PAD_TRIGGER_R|PAD_TRIGGER_Z,       "R+Z" },
	{  9, PAD_BUTTON_A,                      "A" },
	{ 10, PAD_BUTTON_B,                      "B" },
	{ 11, PAD_BUTTON_X,                      "X" },
	{ 12, PAD_BUTTON_Y,                      "Y" },
	{ 13, PAD_BUTTON_START|PAD_TRIGGER_Z_UP, "Start-Z" },
	{ 14, PAD_BUTTON_START|PAD_TRIGGER_Z,    "Start+Z" },
	{ 15, C_STICK_U,                         "C-Up" },
	{ 16, C_STICK_L,                         "C-Left" },
	{ 17, C_STICK_R,                         "C-Right" },
	{ 18, C_STICK_D,                         "C-Down" },
	{ 19, ANALOG_U,                          "A-Up" },
	{ 20, ANALOG_L,                          "A-Left" },
	{ 21, ANALOG_R,                          "A-Right" },
	{ 22, ANALOG_D,                          "A-Down" },
};

static button_t analog_sources[] = {
	{ 0, ANALOG_AS_ANALOG,  "Analog Stick" },
	{ 1, C_STICK_AS_ANALOG, "C-Stick" },
};

static button_t menu_combos[] = {
	{ 0, PAD_BUTTON_X|PAD_BUTTON_Y, "X+Y" },
	{ 1, PAD_BUTTON_START|PAD_BUTTON_X, "Start+X" },
};

u32 gc_connected;

static unsigned int getButtons(int Control)
{
	unsigned int b = PAD_ButtonsHeld(Control);
	s8 stickX      = PAD_StickX(Control);
	s8 stickY      = PAD_StickY(Control);
	s8 substickX   = PAD_SubStickX(Control);
	s8 substickY   = PAD_SubStickY(Control);
	
	if(stickX    < -48) b |= ANALOG_L;
	if(stickX    >  48) b |= ANALOG_R;
	if(stickY    >  48) b |= ANALOG_U;
	if(stickY    < -48) b |= ANALOG_D;
	
	if(substickX < -48) b |= C_STICK_L;
	if(substickX >  48) b |= C_STICK_R;
	if(substickY >  48) b |= C_STICK_U;
	if(substickY < -48) b |= C_STICK_D;
	
	if(!(b & PAD_TRIGGER_Z)) b |= PAD_TRIGGER_Z_UP;

	return b;
}

static inline u8 GCtoPSXAnalog(int a)
{
	a = a * 4 / 3; // GC ranges -96 ~ 96 (192 values, PSX has 256)
	if(a >= 128) a = 127; else if(a < -128) a = -128; // clamp
	return a + 128; // PSX controls range 0-255
}

static int _GetKeys(int Control, BUTTONS * Keys, controller_config_t* config)
{
	if(padNeedScan){ gc_connected = PAD_ScanPads(); padNeedScan = 0; }
	BUTTONS* c = Keys;
	u8 lastRawX = c->lastRawX;
	u8 lastRawY = c->lastRawY;
	u16 gunX = c->gunX;
	u16 gunY = c->gunY;
	
	memset(c, 0, sizeof(BUTTONS));
	//Reset buttons & sticks
	c->btns.All = 0xFFFF;
	c->leftStickX = c->leftStickY = c->rightStickX = c->rightStickY = 128;

	controller_GC.available[Control] = (gc_connected & (1<<Control)) ? 1 : 0;
	if (!controller_GC.available[Control]) return 0;

	unsigned int b = getButtons(Control);
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

	if(controllerType == CONTROLLERTYPE_LIGHTGUN) {
		c->lastRawX = PAD_StickX(Control);
		c->lastRawY = PAD_StickY(Control);
		
		// Keep it within 0 to 1023
		c->gunX += gunX + ((PAD_StickX(Control)) / 6);
		c->gunY += gunY - ((PAD_StickY(Control)) / 6);
		c->gunX = c->gunX > 1023 ? 1023 : (c->gunX < 0 ? 0 : c->gunX);
		c->gunY = c->gunY > 1023 ? 1023 : (c->gunY < 0 ? 0 : c->gunY);
	}
	else {
		//adjust values by 128 cause PSX sticks range 0-255 with a 128 center pos
		int stickX = 0, stickY = 0;
		if(config->analogL->mask == ANALOG_AS_ANALOG){
			stickX = PAD_StickX(Control);
			stickY = PAD_StickY(Control);
		} else if(config->analogL->mask == C_STICK_AS_ANALOG){
			stickX = PAD_SubStickX(Control);
			stickY = PAD_SubStickY(Control);
		}
		c->leftStickX = GCtoPSXAnalog(stickX);
		c->leftStickY = GCtoPSXAnalog(config->invertedYL ? stickY : -stickY);

		if(config->analogR->mask == ANALOG_AS_ANALOG){
			stickX = PAD_StickX(Control);
			stickY = PAD_StickY(Control);
		} else if(config->analogR->mask == C_STICK_AS_ANALOG){
			stickX = PAD_SubStickX(Control);
			stickY = PAD_SubStickY(Control);
		}
		c->rightStickX = GCtoPSXAnalog(stickX);
		c->rightStickY = GCtoPSXAnalog(config->invertedYR ? stickY : -stickY);
	}

	// Return 1 if whether the exit button(s) are pressed
	return isHeld(config->exit) ? 0 : 1;
}

static void pause(int Control){
	PAD_ControlMotor(Control, PAD_MOTOR_STOP);
}

static void resume(int Control){ }

static void rumble(int Control, int rumble){
	PAD_ControlMotor(Control, (rumble && rumbleEnabled) ? PAD_MOTOR_RUMBLE : PAD_MOTOR_STOP);
}

static void configure(int Control, controller_config_t* config){
	// Don't know how this should be integrated
}

static void assign(int p, int v){
	// Nothing to do here
}

static void refreshAvailable(void);

controller_t controller_GC =
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
	  { .SQU        = &buttons[10], // B
	    .CRO        = &buttons[9],  // A
	    .CIR        = &buttons[11], // X
	    .TRI        = &buttons[12], // Y
	    .R1         = &buttons[6],  // Right Trigger - Z
	    .L1         = &buttons[5],  // Left Trigger - Z
	    .R2         = &buttons[8],  // Right Trigger + Z
	    .L2         = &buttons[7],  // Left Trigger + Z
	    .R3         = &buttons[0],  // None
	    .L3         = &buttons[0],  // None
	    .DL         = &buttons[2],  // D-Pad Left
	    .DR         = &buttons[3],  // D-Pad Right
	    .DU         = &buttons[1],  // D-Pad Up
	    .DD         = &buttons[4],  // D-Pad Down
	    .START      = &buttons[13], // Start - Z
	    .SELECT     = &buttons[14], // Start + Z
	    .analogL    = &analog_sources[0], // Analog Stick
	    .analogR    = &analog_sources[1], // C stick
	    .exit       = &menu_combos[1], // Start+X
	    .invertedYL = 0,
	    .invertedYR = 0,
	  }
	 };


static void refreshAvailable(void){

	if(padNeedScan){ gc_connected = PAD_ScanPads(); padNeedScan = 0; }

	int i;
	for(i=0; i<4; ++i)
		controller_GC.available[i] = (gc_connected & (1<<i));
}
