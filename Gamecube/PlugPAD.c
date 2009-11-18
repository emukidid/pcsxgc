/**
 * WiiSX - PlugPAD.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 sepp256
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * Basic Analog PAD plugin for WiiSX
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

#include <gccore.h>
#include <stdint.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <ogc/pad.h>
#include "../plugins.h"
#include "../PsxCommon.h"
#include "../PSEmu_Plugin_Defs.h"
#include "gc_input/controller.h"
#include "wiiSXconfig.h"
#include "PadSSSPSX.h"

static BUTTONS PAD_1;
static BUTTONS PAD_2;
PadDataS lastport1;
PadDataS lastport2;
static int pad_initialized = 0;

static struct
{
	SSSConfig config;
	int devcnt;
	u16 padStat[2];
	int padID[2];
	int padMode1[2];
	int padMode2[2];
	int padModeE[2];
	int padModeC[2];
	int padModeF[2];
	int padVib0[2];
	int padVib1[2];
	int padVibF[2][4];
	int padVibC[2];
	u64 padPress[2][16];
	int curPad;
	int curByte;
	int curCmd;
	int cmdLen;
} global;

extern void SysPrintf(char *fmt, ...);
extern int stop;

/* Controller type, later do this by a Variable in the GUI */
//extern char controllerType = 0; // 0 = standard, 1 = analog (analog fails on old games)
long  PadFlags = 0;

virtualControllers_t virtualControllers[2];

controller_t* controller_ts[num_controller_t] =
#if defined(WII) && !defined(NO_BT)
	{ &controller_GC, &controller_Classic,
	  &controller_WiimoteNunchuk,
	 };
#else
	{ &controller_GC,
	 };
#endif

// Use to invoke func on the mapped controller with args
#define DO_CONTROL(Control,func,args...) \
	virtualControllers[Control].control->func( \
		virtualControllers[Control].number, ## args)


void pauseInput(void){
	int i;
	for(i=0; i<2; ++i)
		if(virtualControllers[i].inUse) DO_CONTROL(i, pause);
}

void resumeInput(void){
	int i;
	for(i=0; i<2; ++i)
		if(virtualControllers[i].inUse) DO_CONTROL(i, resume);
}

void init_controller_ts(void){
	int i;
	for(i=0; i<num_controller_t; ++i)
		controller_ts[i]->init();
}

void assign_controller(int wv, controller_t* type, int wp){
	virtualControllers[wv].control = type;
	virtualControllers[wv].inUse   = 1;
	virtualControllers[wv].number  = wp;

	type->assign(wp,wv);
}

void unassign_controller(int wv){
	virtualControllers[wv].control = NULL;
	virtualControllers[wv].inUse   = 0;
	virtualControllers[wv].number  = -1;
}

void update_controller_assignment (void)
{
	//TODO: Map 5 or 8 controllers if multitaps are used.
	int i,t,w;

	init_controller_ts();

	int num_assigned[num_controller_t];
	memset(num_assigned, 0, sizeof(num_assigned));

	// Map controllers in the priority given
	// Outer loop: virtual controllers
	for(i=0; i<2; ++i){
		// Middle loop: controller type
		for(t=0; t<num_controller_t; ++t){
			controller_t* type = controller_ts[t];
			// Inner loop: which controller
			for(w=num_assigned[t]; w<4 && !type->available[w]; ++w, ++num_assigned[t]);
			// If we've exhausted this type, move on
			if(w == 4) continue;

			assign_controller(i, type, w);
			padType[i] = type == &controller_GC ? PADTYPE_GAMECUBE : PADTYPE_WII;
			padAssign[i] = w;

			// Don't assign the next type over this one or the same controller
			++num_assigned[t];
			break;
		}
		if(t == num_controller_t)
			break;
	}

	// 'Initialize' the unmapped virtual controllers
	for(; i<2; ++i){
		unassign_controller(i);
		padType[i] = PADTYPE_NONE;
	}
}

long PAD__init(long flags) {
	PadFlags |= flags;

	return PSE_PAD_ERR_SUCCESS;
}

long PAD__shutdown(void) {
	return PSE_PAD_ERR_SUCCESS;
}
/*
long PAD__open(void)
{
	return PSE_PAD_ERR_SUCCESS;
}

long PAD__close(void) {
	return PSE_PAD_ERR_SUCCESS;
}

long PAD__readPort1(PadDataS* pad) {
	//TODO: Multitap support; Light Gun support

	if( virtualControllers[0].inUse && DO_CONTROL(0, GetKeys, (BUTTONS*)&PAD_1) ) {
		stop = 1;
	}

	if(controllerType == CONTROLLERTYPE_STANDARD) {
		pad->controllerType = PSE_PAD_TYPE_STANDARD; 	// Standard Pad
	}
	else if(controllerType == CONTROLLERTYPE_ANALOG) {
		pad->controllerType = PSE_PAD_TYPE_ANALOGPAD; 	// Analog Pad  (Right JoyStick)
		pad->leftJoyX  = PAD_1.leftStickX;
		pad->leftJoyY  = PAD_1.leftStickY;
		pad->rightJoyX = PAD_1.rightStickX;
		pad->rightJoyY = PAD_1.rightStickY;
	}
	else { 
		//TODO: Light Gun
	}

	pad->buttonStatus = PAD_1.btns.All;  //set the buttons

	return PSE_PAD_ERR_SUCCESS;
}

long PAD__readPort2(PadDataS* pad) {
	//TODO: Multitap support; Light Gun support

	if( virtualControllers[1].inUse && DO_CONTROL(1, GetKeys, (BUTTONS*)&PAD_2) ) {
		stop = 1;
	}

	if(controllerType == CONTROLLERTYPE_STANDARD) {
		pad->controllerType = PSE_PAD_TYPE_STANDARD; 	// Standard Pad
	}
	else if(controllerType == CONTROLLERTYPE_ANALOG) {
		pad->controllerType = PSE_PAD_TYPE_ANALOGPAD; 	// Analog Pad  (Right JoyStick)
		pad->leftJoyX  = PAD_2.leftStickX;
		pad->leftJoyY  = PAD_2.leftStickY;
		pad->rightJoyX = PAD_2.rightStickX;
		pad->rightJoyY = PAD_2.rightStickY;
	}
	else { 
		//TODO: Light Gun
	}

	pad->buttonStatus = PAD_2.btns.All;  //set the buttons

	return PSE_PAD_ERR_SUCCESS;
}
*/

static void PADsetMode (const int pad, const int mode)
{
	static const u8 padID[] = { 0x41, 0x73, 0x41, 0x79 };
	global.padMode1[pad] = mode;
	global.padVib0[pad] = 0;
	global.padVib1[pad] = 0;
	global.padVibF[pad][0] = 0;
	global.padVibF[pad][1] = 0;
	global.padID[pad] = padID[global.padMode2[pad] * 2 + mode];
}

void checkAnalog(const int pad, const int press)
{
	static char prev[2] = { 0, 0};
	if ((prev[pad] != press) && (global.padModeF[pad] == 0))
	{
		prev[pad] = press;
		if (press) PADsetMode (pad, !global.padMode1[pad]);
	}
}

static void UpdateState (const int pad)
{
	const int vib0 = global.padVibF[pad][0] ? 1 : 0;
	const int vib1 = global.padVibF[pad][1] ? 1 : 0;

	if( virtualControllers[0].inUse && DO_CONTROL(0, GetKeys, (BUTTONS*)&PAD_1) ) {
    stop = 1;
  }
  lastport1.leftJoyX = PAD_1.leftStickX; lastport1.leftJoyY = PAD_1.leftStickY;
	lastport1.rightJoyX = PAD_1.rightStickX; lastport1.rightJoyY = PAD_1.rightStickY;
		
  if( virtualControllers[1].inUse && DO_CONTROL(1, GetKeys, (BUTTONS*)&PAD_2) ) {
    stop = 1;
  }
	lastport2.leftJoyX = PAD_2.leftStickX; lastport2.leftJoyY = PAD_2.leftStickY;
	lastport2.rightJoyX = PAD_2.rightStickX; lastport2.rightJoyY = PAD_2.rightStickY;
	
	checkAnalog( 0, controllerType == CONTROLLERTYPE_ANALOG ? 0 : 1 ) ;
	global.padStat[0] = (((PAD_1.btns.All>>8)&0xFF) | ( (PAD_1.btns.All<<8) & 0xFF00 )) &0xFFFF;
	checkAnalog( 1, controllerType == CONTROLLERTYPE_ANALOG ? 0 : 1 ) ;
	global.padStat[1] = (((PAD_2.btns.All>>8)&0xFF) | ( (PAD_2.btns.All<<8) & 0xFF00 )) &0xFFFF;
  
	lastport1.buttonStatus = global.padStat[0];
	lastport2.buttonStatus = global.padStat[1];

	/* Small Motor */
	if ((global.padVibF[pad][2] != vib0) )
	{
		global.padVibF[pad][2] = vib0;
		DO_CONTROL(pad, rumble, global.padVibF[pad][0]);
	}
	/* Big Motor */
	if ((global.padVibF[pad][3] != vib1) )
	{
		global.padVibF[pad][3] = vib1;
		DO_CONTROL(pad, rumble, global.padVibF[pad][1]);
	}

}

long SSS_PADopen (void *p)
{
	if (!pad_initialized)
	{
		memset (&global, 0, sizeof (global));
		memset( &lastport1, 0, sizeof(lastport1) ) ;
		memset( &lastport2, 0, sizeof(lastport2) ) ;
		global.padStat[0] = 0xffff;
		global.padStat[1] = 0xffff;
		PADsetMode (0, 1);  //port 0, analog
		PADsetMode (1, 1);  //port 1, analog
	}
	return 0;
}

long SSS_PADclose (void)
{
	if (pad_initialized) {
  	pad_initialized=0;
	}
	return 0 ;
}

long SSS_PADquery (void)
{
	return 3;
}

unsigned char SSS_PADstartPoll (int pad)
{
	global.curPad = pad -1;
	global.curByte = 0;
	return 0xff;
}

static const u8 cmd40[8] =
{
	0xff, 0x5a, 0x00, 0x00, 0x02, 0x00, 0x00, 0x5a
};
static const u8 cmd41[8] =
{
	0xff, 0x5a, 0xff, 0xff, 0x03, 0x00, 0x00, 0x5a,
};
static const u8 cmd44[8] =
{
	0xff, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const u8 cmd45[8] =
{
	0xff, 0x5a, 0x03, 0x02, 0x01, 0x02, 0x01, 0x00,
};
static const u8 cmd46[8] =
{
	0xff, 0x5a, 0x00, 0x00, 0x01, 0x02, 0x00, 0x0a,
};
static const u8 cmd47[8] =
{
	0xff, 0x5a, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00,
};
static const u8 cmd4c[8] =
{
	0xff, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const u8 cmd4d[8] =
{
	0xff, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};
static const u8 cmd4f[8] =
{
	0xff, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5a,
};

unsigned char SSS_PADpoll (const unsigned char value)
{
	const int pad = global.curPad;
	const int cur = global.curByte;
	static u8 buf[20];
	if (cur == 0)
	{
		global.curByte++;
		global.curCmd = value;
		switch (value)
		{
		case 0x40:
			global.cmdLen = sizeof (cmd40);
			memcpy (buf, cmd40, sizeof (cmd40));
			return 0xf3;
		case 0x41:
			global.cmdLen = sizeof (cmd41);
			memcpy (buf, cmd41, sizeof (cmd41));
			return 0xf3;
		case 0x42:
		case 0x43:
			if (value == 0x42) UpdateState (pad);
			global.cmdLen = 2 + 2 * (global.padID[pad] & 0x0f);
			buf[1] = global.padModeC[pad] ? 0x00 : 0x5a;
			*(u16*)&buf[2] = global.padStat[pad];
			if (value == 0x43 && global.padModeE[pad])
			{
				buf[4] = 0;
				buf[5] = 0;
				buf[6] = 0;
				buf[7] = 0;
				return 0xf3;
			}
			else
			{
				buf[ 4] = pad ? lastport2.rightJoyX : lastport1.rightJoyX ; 
				buf[ 5] = pad ? lastport2.rightJoyY : lastport1.rightJoyY ; 
				buf[ 6] = pad ? lastport2.leftJoyX : lastport1.leftJoyX ; 
				buf[ 7] = pad ? lastport2.leftJoyY : lastport1.leftJoyY ; 
				if (global.padID[pad] == 0x79)
				{ 
  				// do some pressure stuff (this is for PS2 only!)
				}
				return (u8)global.padID[pad];
			}
			break;
		case 0x44:
			global.cmdLen = sizeof (cmd44);
			memcpy (buf, cmd44, sizeof (cmd44));
			return 0xf3;
		case 0x45:
			global.cmdLen = sizeof (cmd45);
			memcpy (buf, cmd45, sizeof (cmd45));
			buf[4] = (u8)global.padMode1[pad];
			return 0xf3;
		case 0x46:
			global.cmdLen = sizeof (cmd46);
			memcpy (buf, cmd46, sizeof (cmd46));
			return 0xf3;
		case 0x47:
			global.cmdLen = sizeof (cmd47);
			memcpy (buf, cmd47, sizeof (cmd47));
			return 0xf3;
		case 0x4c:
			global.cmdLen = sizeof (cmd4c);
			memcpy (buf, cmd4c, sizeof (cmd4c));
			return 0xf3;
		case 0x4d:
			global.cmdLen = sizeof (cmd4d);
			memcpy (buf, cmd4d, sizeof (cmd4d));
			return 0xf3;
		case 0x4f:
			global.padID[pad] = 0x79;
			global.padMode2[pad] = 1;
			global.cmdLen = sizeof (cmd4f);
			memcpy (buf, cmd4f, sizeof (cmd4f));
			return 0xf3;
		}
	}
	switch (global.curCmd)
	{
	case 0x42:
		if (cur == global.padVib0[pad])
			global.padVibF[pad][0] = value;
		if (cur == global.padVib1[pad])
			global.padVibF[pad][1] = value;
		break;
	case 0x43:
		if (cur == 2)
		{
			global.padModeE[pad] = value;
			global.padModeC[pad] = 0;
		}
		break;
	case 0x44:
		if (cur == 2)
			PADsetMode (pad, value);
		if (cur == 3)
			global.padModeF[pad] = (value == 3);
		break;
	case 0x46:
		if (cur == 2)
		{
			switch(value)
			{
			case 0:
				buf[5] = 0x02;
				buf[6] = 0x00;
				buf[7] = 0x0A;
				break;
			case 1:
				buf[5] = 0x01;
				buf[6] = 0x01;
				buf[7] = 0x14;
				break;
			}
		}
		break;
	case 0x4c:
		if (cur == 2)
		{
			static const u8 buf5[] = { 0x04, 0x07, 0x02, 0x05 };
			buf[5] = buf5[value & 3];
		}
		break;
	case 0x4d:
		if (cur >= 2)
		{
			if (cur == global.padVib0[pad])
				buf[cur] = 0x00;
			if (cur == global.padVib1[pad])
				buf[cur] = 0x01;
			if (value == 0x00)
			{
				global.padVib0[pad] = cur;
				if ((global.padID[pad] & 0x0f) < (cur - 1) / 2)
					 global.padID[pad] = (global.padID[pad] & 0xf0) + (cur - 1) / 2;
			}
			else if (value == 0x01)
			{
				global.padVib1[pad] = cur;
				if ((global.padID[pad] & 0x0f) < (cur - 1) / 2)
					 global.padID[pad] = (global.padID[pad] & 0xf0) + (cur - 1) / 2;
			}
		}
		break;
	}
	if (cur >= global.cmdLen)
		return 0;
	return buf[global.curByte++];
}



long SSS_PADreadPort1 (PadDataS* pads)
{

	pads->buttonStatus = global.padStat[0];
 
	memset (pads, 0, sizeof (PadDataS));
	if ((global.padID[0] & 0xf0) == 0x40)
	{
		pads->rightJoyX = pads->rightJoyY = pads->leftJoyX = pads->leftJoyY = 128 ;
		pads->controllerType = PSE_PAD_TYPE_STANDARD;
	}
	else
	{
		pads->controllerType = PSE_PAD_TYPE_ANALOGPAD;
		if( virtualControllers[0].inUse && DO_CONTROL(0, GetKeys, (BUTTONS*)&PAD_1) ) {
      stop = 1;
    }
		pads->leftJoyX = PAD_1.leftStickX; pads->leftJoyY = PAD_1.leftStickY;
		pads->rightJoyX = PAD_1.rightStickX; pads->rightJoyY = PAD_1.rightStickY;
	}

	memcpy( &lastport1, pads, sizeof( lastport1 ) ) ;

	return 0;
}

long SSS_PADreadPort2 (PadDataS* pads)
{

	pads->buttonStatus = global.padStat[1];

	memset (pads, 0, sizeof (PadDataS));
	if ((global.padID[1] & 0xf0) == 0x40)
	{
		pads->rightJoyX = pads->rightJoyY = pads->leftJoyX = pads->leftJoyY = 128 ;
		pads->controllerType = PSE_PAD_TYPE_STANDARD;
	}
	else
	{
		pads->controllerType = PSE_PAD_TYPE_ANALOGPAD;
		if( virtualControllers[1].inUse && DO_CONTROL(1, GetKeys, (BUTTONS*)&PAD_2) ) {
      stop = 1;
    }
		pads->leftJoyX = PAD_2.leftStickX; pads->leftJoyY = PAD_2.leftStickY;
		pads->rightJoyX = PAD_2.rightStickX; pads->rightJoyY = PAD_2.rightStickY;
	}

	memcpy( &lastport2, pads, sizeof( lastport1 ) ) ;
	return 0;
}
