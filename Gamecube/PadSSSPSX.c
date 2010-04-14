/**
 * WiiSX - PadSSSPSX.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009, 2010 sepp256
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * Basic Analog PAD plugin for WiiSX
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
extern PadDataS lastport1;
extern PadDataS lastport2;
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
extern long  PadFlags;

extern virtualControllers_t virtualControllers[2];

// Use to invoke func on the mapped controller with args
#define DO_CONTROL(Control,func,args...) \
	virtualControllers[Control].control->func( \
		virtualControllers[Control].number, ## args)

void assign_controller(int wv, controller_t* type, int wp);

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

static void UpdateState (const int pad)
{
	const int vib0 = global.padVibF[pad][0] ? 1 : 0;
	const int vib1 = global.padVibF[pad][1] ? 1 : 0;

	//TODO: Rework the 4 calls to GetKeys to reuse the same code & reset BUTTONS when no controller in use
	int Control = 0;
#if defined(WII) && !defined(NO_BT)
	//Need to switch between Classic and WiimoteNunchuck if user swapped extensions
	if (padType[virtualControllers[Control].number] == PADTYPE_WII)
	{
		if (virtualControllers[Control].control == &controller_Classic &&
			!controller_Classic.available[virtualControllers[Control].number] &&
			controller_WiimoteNunchuk.available[virtualControllers[Control].number])
			assign_controller(Control, &controller_WiimoteNunchuk, virtualControllers[Control].number);
		else if (virtualControllers[Control].control == &controller_WiimoteNunchuk &&
			!controller_WiimoteNunchuk.available[virtualControllers[Control].number] &&
			controller_Classic.available[virtualControllers[Control].number])
			assign_controller(Control, &controller_Classic, virtualControllers[Control].number);
	}
#endif
	if(virtualControllers[Control].inUse)
		if(DO_CONTROL(Control, GetKeys, (BUTTONS*)&PAD_1, virtualControllers[Control].config))
			stop = 1;

	lastport1.leftJoyX = PAD_1.leftStickX; lastport1.leftJoyY = PAD_1.leftStickY;
	lastport1.rightJoyX = PAD_1.rightStickX; lastport1.rightJoyY = PAD_1.rightStickY;

	Control = 1;
#if defined(WII) && !defined(NO_BT)
	//Need to switch between Classic and WiimoteNunchuck if user swapped extensions
	if (padType[virtualControllers[Control].number] == PADTYPE_WII)
	{
		if (virtualControllers[Control].control == &controller_Classic &&
			!controller_Classic.available[virtualControllers[Control].number] &&
			controller_WiimoteNunchuk.available[virtualControllers[Control].number])
			assign_controller(Control, &controller_WiimoteNunchuk, virtualControllers[Control].number);
		else if (virtualControllers[Control].control == &controller_WiimoteNunchuk &&
			!controller_WiimoteNunchuk.available[virtualControllers[Control].number] &&
			controller_Classic.available[virtualControllers[Control].number])
			assign_controller(Control, &controller_Classic, virtualControllers[Control].number);
	}
#endif
	if(virtualControllers[Control].inUse)
		if(DO_CONTROL(Control, GetKeys, (BUTTONS*)&PAD_2, virtualControllers[Control].config))
			stop = 1;

	lastport2.leftJoyX = PAD_2.leftStickX; lastport2.leftJoyY = PAD_2.leftStickY;
	lastport2.rightJoyX = PAD_2.rightStickX; lastport2.rightJoyY = PAD_2.rightStickY;
	
	PADsetMode( 0, controllerType == CONTROLLERTYPE_ANALOG ? 1 : 0);
	global.padStat[0] = (((PAD_1.btns.All>>8)&0xFF) | ( (PAD_1.btns.All<<8) & 0xFF00 )) &0xFFFF;
	PADsetMode( 1, controllerType == CONTROLLERTYPE_ANALOG ? 1 : 0);
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
		int Control = 0;
#if defined(WII) && !defined(NO_BT)
		//Need to switch between Classic and WiimoteNunchuck if user swapped extensions
		if (padType[virtualControllers[Control].number] == PADTYPE_WII)
		{
			if (virtualControllers[Control].control == &controller_Classic &&
				!controller_Classic.available[virtualControllers[Control].number] &&
				controller_WiimoteNunchuk.available[virtualControllers[Control].number])
				assign_controller(Control, &controller_WiimoteNunchuk, virtualControllers[Control].number);
			else if (virtualControllers[Control].control == &controller_WiimoteNunchuk &&
				!controller_WiimoteNunchuk.available[virtualControllers[Control].number] &&
				controller_Classic.available[virtualControllers[Control].number])
				assign_controller(Control, &controller_Classic, virtualControllers[Control].number);
		}
#endif
		if(virtualControllers[Control].inUse)
			if(DO_CONTROL(Control, GetKeys, (BUTTONS*)&PAD_1, virtualControllers[Control].config))
				stop = 1;

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
		int Control = 1;
#if defined(WII) && !defined(NO_BT)
		//Need to switch between Classic and WiimoteNunchuck if user swapped extensions
		if (padType[virtualControllers[Control].number] == PADTYPE_WII)
		{
			if (virtualControllers[Control].control == &controller_Classic &&
				!controller_Classic.available[virtualControllers[Control].number] &&
				controller_WiimoteNunchuk.available[virtualControllers[Control].number])
				assign_controller(Control, &controller_WiimoteNunchuk, virtualControllers[Control].number);
			else if (virtualControllers[Control].control == &controller_WiimoteNunchuk &&
				!controller_WiimoteNunchuk.available[virtualControllers[Control].number] &&
				controller_Classic.available[virtualControllers[Control].number])
				assign_controller(Control, &controller_Classic, virtualControllers[Control].number);
		}
#endif
		if(virtualControllers[Control].inUse)
			if(DO_CONTROL(Control, GetKeys, (BUTTONS*)&PAD_2, virtualControllers[Control].config))
				stop = 1;

		pads->leftJoyX = PAD_2.leftStickX; pads->leftJoyY = PAD_2.leftStickY;
		pads->rightJoyX = PAD_2.rightStickX; pads->rightJoyY = PAD_2.rightStickY;
	}

	memcpy( &lastport2, pads, sizeof( lastport1 ) ) ;
	return 0;
}
