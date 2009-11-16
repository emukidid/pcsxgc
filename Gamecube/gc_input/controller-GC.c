/**
 * WiiSX - controller-GC.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 sepp256
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * Gamecube controller input module
 *
 * Wii64 homepage: http://www.emulatemii.com
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

u32 gc_connected;

static int _GetKeys(int Control, BUTTONS * Keys )
{
	if(padNeedScan){ gc_connected = PAD_ScanPads(); padNeedScan = 0; }
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));
	c->btns.All = 0xFFFF;  //needed
	
	controller_GC.available[Control] = (gc_connected & (1<<Control)) ? 1 : 0;
	if (!controller_GC.available[Control]) return 0;

	int b = PAD_ButtonsHeld(Control);
	c->btns.R_DPAD       = (b & PAD_BUTTON_RIGHT) ? 0 : 1;
	c->btns.L_DPAD       = (b & PAD_BUTTON_LEFT)  ? 0 : 1;
	c->btns.D_DPAD       = (b & PAD_BUTTON_DOWN)  ? 0 : 1;
	c->btns.U_DPAD       = (b & PAD_BUTTON_UP)    ? 0 : 1;
	
	c->btns.SQUARE_BUTTON    = (b & PAD_BUTTON_B)     ? 0 : 1;
	c->btns.CROSS_BUTTON     = (b & PAD_BUTTON_A)     ? 0 : 1;
  c->btns.CIRCLE_BUTTON    = (b & PAD_BUTTON_X)     ? 0 : 1;
	c->btns.TRIANGLE_BUTTON  = (b & PAD_BUTTON_Y)     ? 0 : 1;
  
	c->btns.START_BUTTON  = ((b & PAD_BUTTON_START) && (!(b & PAD_TRIGGER_Z))) ? 0 : 1;
	c->btns.SELECT_BUTTON = ((b & PAD_BUTTON_START) && ((b & PAD_TRIGGER_Z)))  ? 0 : 1;
	c->btns.R1_BUTTON    = ((b & PAD_TRIGGER_R) && (!(b & PAD_TRIGGER_Z)))     ? 0 : 1;
	c->btns.L1_BUTTON    = ((b & PAD_TRIGGER_L) && (!(b & PAD_TRIGGER_Z)))     ? 0 : 1;
	c->btns.R2_BUTTON    = ((b & PAD_TRIGGER_R) && ((b & PAD_TRIGGER_Z)))      ? 0 : 1;
	c->btns.L2_BUTTON    = ((b & PAD_TRIGGER_L) && ((b & PAD_TRIGGER_Z)))      ? 0 : 1;

	//adjust values by 128 cause PSX sticks range 0-255 with a 128 center pos
	c->rightStickX  = (u8)(PAD_SubStickX(Control)+127) & 0xFF;
	c->rightStickY  = (u8)(PAD_SubStickY(Control)+127) & 0xFF;
	c->leftStickX   = (u8)(PAD_StickX(Control)+127)    & 0xFF;
	c->leftStickY   = (u8)(-PAD_StickY(Control)+127)   & 0xFF;

	// X+Y quits to menu
	return (b & PAD_BUTTON_X) && (b & PAD_BUTTON_Y);
}

static void pause(int Control){
	PAD_ControlMotor(Control, PAD_MOTOR_STOP);
}

static void resume(int Control){ }

static void rumble(int Control, int rumble){
	if(rumble) PAD_ControlMotor(Control, PAD_MOTOR_RUMBLE);
	else PAD_ControlMotor(Control, PAD_MOTOR_STOP);
}

static void configure(int Control){
	// Don't know how this should be integrated
}

static void assign(int p, int v){
	// Nothing to do here
}

static void init(void);

controller_t controller_GC =
	{ _GetKeys,
	  configure,
	  init,
	  assign,
	  pause,
	  resume,
	  rumble,
	  {0, 0, 0, 0}
	 };

static void init(void){

	if(padNeedScan){ gc_connected = PAD_ScanPads(); padNeedScan = 0; }

	int i;
	for(i=0; i<4; ++i)
		controller_GC.available[i] = (gc_connected & (1<<i));
}
