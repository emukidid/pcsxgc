/**
 * WiiSX - controller-WiimoteNunchuk.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 sepp256
 * 
 * Wiimote + Nunchuk input module
 *
 * WiiSX homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *                sepp256@gmail.com
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

#ifdef HW_RVL

#include <string.h>
#include <math.h>
#include <wiiuse/wpad.h>
#include "controller.h"

#ifndef PI
#define PI 3.14159f
#endif

enum { STICK_X, STICK_Y };
static int getStickValue(joystick_t* j, int axis, int maxAbsValue){
	double angle = PI * j->ang/180.0f;
	double magnitude = (j->mag > 1.0f) ? 1.0f :
	                    (j->mag < -1.0f) ? -1.0f : j->mag;
	double value;
	if(axis == STICK_X)
		value = magnitude * sin( angle );
	else
		value = magnitude * cos( angle );
	return (int)(value * maxAbsValue);
}

static int _GetKeys(int Control, BUTTONS * Keys )
{
	if(wpadNeedScan){ WPAD_ScanPads(); wpadNeedScan = 0; }
	WPADData* wpad = WPAD_Data(Control);
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));
	c->btns.All = 0xFFFF;  //needed

	// Only use a connected nunchuck controller
	if(wpad->err == WPAD_ERR_NONE &&
	   wpad->exp.type == WPAD_EXP_NUNCHUK){
		controller_WiimoteNunchuk.available[Control] = 1;
	} else {
		controller_WiimoteNunchuk.available[Control] = 0;
		if(wpad->err == WPAD_ERR_NONE &&
		   wpad->exp.type == WPAD_EXP_CLASSIC){
			controller_Classic.available[Control] = 1;
		}
		return 0;
	}

	int b = wpad->btns_h;
	int bmod = b & WPAD_NUNCHUK_BUTTON_C;	//use bmod to switch to analog input
	s8 nunStickX = getStickValue(&wpad->exp.nunchuk.js, STICK_X, 127);
	s8 nunStickY = getStickValue(&wpad->exp.nunchuk.js, STICK_Y, 127);
	s8 fakeStickX = -127*((b & WPAD_BUTTON_LEFT)>>8)  + 127*((b & WPAD_BUTTON_RIGHT)>>9);
	s8 fakeStickY = -127*((b & WPAD_BUTTON_DOWN)>>10) + 127*((b & WPAD_BUTTON_UP)>>11);
	s8 stickThresh = 63;
	if (bmod)
	{
		c->rightStickX  = (u8)(nunStickX+127) & 0xFF;
		c->rightStickY  = (u8)(nunStickY+127) & 0xFF;
		c->leftStickX   = (u8)(fakeStickX+127) & 0xFF;
		c->leftStickY   = (u8)(fakeStickY+127) & 0xFF;

		c->btns.R1_BUTTON       = (b & WPAD_BUTTON_A)          ? 0 : 1;
		c->btns.L1_BUTTON       = (b & WPAD_NUNCHUK_BUTTON_Z)  ? 0 : 1;
	}
	else
	{
		c->rightStickX  = 0;
		c->rightStickY  = 0;
		c->leftStickX   = 0;
		c->leftStickY   = 0;

		c->btns.R_DPAD          = (nunStickX >  stickThresh) ? 0 : 1;
		c->btns.L_DPAD          = (nunStickX < -stickThresh) ? 0 : 1;
		c->btns.D_DPAD          = (nunStickY < -stickThresh) ? 0 : 1;
		c->btns.U_DPAD          = (nunStickY >  stickThresh) ? 0 : 1;

		c->btns.CIRCLE_BUTTON   = (b & WPAD_BUTTON_RIGHT) ? 0 : 1;
		c->btns.SQUARE_BUTTON   = (b & WPAD_BUTTON_LEFT)  ? 0 : 1;
		c->btns.CROSS_BUTTON    = (b & WPAD_BUTTON_DOWN)  ? 0 : 1;
		c->btns.TRIANGLE_BUTTON = (b & WPAD_BUTTON_UP)    ? 0 : 1;

		c->btns.R2_BUTTON       = (b & WPAD_BUTTON_A)          ? 0 : 1;
		c->btns.L2_BUTTON       = (b & WPAD_NUNCHUK_BUTTON_Z)  ? 0 : 1;
	}

	c->btns.START_BUTTON    = (b & WPAD_BUTTON_HOME)                       ? 0 : 1;
	c->btns.SELECT_BUTTON   = (b & (WPAD_BUTTON_MINUS | WPAD_BUTTON_PLUS)) ? 0 : 1;

	// 1+2 quits to menu
	return (b & WPAD_BUTTON_1) && (b & WPAD_BUTTON_2);
}

static void pause(int Control){ }

static void resume(int Control){ }

static void rumble(int Control, int rumble){ }

static void configure(int Control){
	// Don't know how this should be integrated
}

static void assign(int p, int v){
	// TODO: Light up the LEDs appropriately
}

static void init(void);

controller_t controller_WiimoteNunchuk =
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
	int i;
	WPAD_ScanPads();
	for(i=0; i<4; ++i){
		WPADData* wpad = WPAD_Data(i);
		// Only use a connected nunchuk
		if(wpad->err == WPAD_ERR_NONE &&
		   wpad->exp.type == WPAD_EXP_NUNCHUK){
			controller_WiimoteNunchuk.available[i] = 1;
			WPAD_SetDataFormat(i, WPAD_DATA_EXPANSION);
		} else
			controller_WiimoteNunchuk.available[i] = 0;
	}
}
#endif
