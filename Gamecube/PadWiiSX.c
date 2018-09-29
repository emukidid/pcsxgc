/**
 * WiiSX - PadWiiSX.c
 * Copyright (C) 1999-2002  Pcsx Team
 * Copyright (C) 2010 sepp256
 * 
 * PAD plugin for WiiSX based on Pcsxbox sources
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../PsxCommon.h"
#include "../PSEmu_Plugin_Defs.h"
#include "gc_input/controller.h"
#include "wiiSXconfig.h"

extern virtualControllers_t virtualControllers[2];
extern int stop;

// Use to invoke func on the mapped controller with args
#define DO_CONTROL(Control,func,args...) \
	virtualControllers[Control].control->func( \
		virtualControllers[Control].number, ## args)

void assign_controller(int wv, controller_t* type, int wp);

static BUTTONS PAD_1;
static BUTTONS PAD_2;

//extern unsigned int m_psxfix_controller1 ;
//extern unsigned int m_psxfix_controller2 ;
//extern unsigned int m_psxfix_controller3 ;
//extern unsigned int m_psxfix_controller4 ;
//extern unsigned int m_psxfix_multitap ;


static unsigned char buf[256];
unsigned char stdpar[10] = { 0x00, 0x41, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
unsigned char mousepar[8] = { 0x00, 0x12, 0x5a, 0xff, 0xff, 0xff, 0xff };
unsigned char analogpar[10] = { 0x00, 0xff, 0x5a, 0xff, 0xff,0xff,0xff,0xff,0xff };
//unsigned char multipar[36] = { 0x00, 0x80, 0x5a, 0xff, 0xff,0xff,0xff,0xff,0xff };

static int bufcount, bufc;

//PadDataS padd1, padd2;
//int readnopoll( int port );
//void xbox_read_sticks( unsigned int port, unsigned char *lx, unsigned char *ly, unsigned char *rx, unsigned char *ry ) ;

s32 PAD__readPort1(PadDataS* ppad)
{
#ifdef __SWITCH__
#else
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


    ppad->buttonStatus = (PAD_1.btns.All&0xFFFF);
	if ( controllerType == CONTROLLERTYPE_ANALOG )
	{
		ppad->controllerType = PSE_PAD_TYPE_ANALOGPAD; 
		//ppad->controllerType = PSE_PAD_TYPE_ANALOGJOY ;
		ppad->leftJoyX = PAD_1.leftStickX; ppad->leftJoyY = PAD_1.leftStickY;
		ppad->rightJoyX = PAD_1.rightStickX; ppad->rightJoyY = PAD_1.rightStickY;
	}
	else
	{
		ppad->controllerType = PSE_PAD_TYPE_STANDARD; // standard
		ppad->rightJoyX = ppad->rightJoyY = ppad->leftJoyX = ppad->leftJoyY = 128 ;
	}
#endif
	return 0 ;
}

s32 PAD__readPort2(PadDataS* ppad)
{
#ifdef __SWITCH__
#else
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


    ppad->buttonStatus = (PAD_2.btns.All&0xFFFF);
	if ( controllerType == CONTROLLERTYPE_ANALOG )
	{
		ppad->controllerType = PSE_PAD_TYPE_ANALOGPAD; 
		//ppad->controllerType = PSE_PAD_TYPE_ANALOGJOY ;
		ppad->leftJoyX = PAD_2.leftStickX; ppad->leftJoyY = PAD_2.leftStickY;
		ppad->rightJoyX = PAD_2.rightStickX; ppad->rightJoyY = PAD_2.rightStickY;
	}
	else
	{
		ppad->controllerType = PSE_PAD_TYPE_STANDARD; // standard
		ppad->rightJoyX = ppad->rightJoyY = ppad->leftJoyX = ppad->leftJoyY = 128 ;
	}
#endif
	return 0 ;
}

unsigned char _PADstartPoll(PadDataS *pad, unsigned int ismultitap) {
	int i ;
	unsigned short value ;
	bufc = 0;

	if ( ismultitap )
	{
/* TODO: Implement 0,1,or 2 multitaps and integrate with gc_input
		buf[0] = 0x00 ;
		buf[1] = 0x80 ;
		buf[2] = 0x5a ;

		for ( i = 0 ; i < 4 ; i++ )
		{
			value = readnopoll( i ) ;
			buf[ (i*8) + 3] = 0x41 ;
			buf[ (i*8) + 4] = 0x5a ;
			buf[ (i*8) + 5] = value & 0xff ;
			buf[ (i*8) + 6] = value >> 8 ;
			buf[ (i*8) + 7] = 0xFF ;
			buf[ (i*8) + 8] = 0xFF ;
			buf[ (i*8) + 9] = 0xFF ;
			buf[ (i*8) + 10]= 0xFF ;
		}

		if ( m_psxfix_controller1 )
		{
			buf[ (0*8) + 3] = 0x73 ;
//			xbox_read_sticks( 0, &(buf[ (0*8) + 9]), &(buf[ (0*8) + 10]), &(buf[ (0*8) + 7]), &(buf[ (0*8) + 8]) ) ;
		}
		if ( m_psxfix_controller2 )
		{
			buf[ (1*8) + 3] = 0x73 ;
//			xbox_read_sticks( 1, &(buf[ (1*8) + 9]), &(buf[ (1*8) + 10]), &(buf[ (1*8) + 7]), &(buf[ (1*8) + 8]) ) ;
		}
		if ( m_psxfix_controller3 )
		{
			buf[ (2*8) + 3] = 0x73 ;
//			xbox_read_sticks( 2, &(buf[ (2*8) + 9]), &(buf[ (2*8) + 10]), &(buf[ (2*8) + 7]), &(buf[ (2*8) + 8]) ) ;
		}
		if ( m_psxfix_controller4 )
		{
			buf[ (3*8) + 3] = 0x73 ;
//			xbox_read_sticks( 3, &(buf[ (3*8) + 9]), &(buf[ (3*8) + 10]), &(buf[ (3*8) + 7]), &(buf[ (3*8) + 8]) ) ;
		}
		bufcount = 34;
*/
	}
	else
	{
		switch (pad->controllerType) {
			case PSE_PAD_TYPE_MOUSE:
				mousepar[3] = pad->buttonStatus & 0xff;
				mousepar[4] = pad->buttonStatus >> 8;
				mousepar[5] = pad->moveX;
				mousepar[6] = pad->moveY;

				memcpy(buf, mousepar, 7);
				bufcount = 6;
				break;
			case PSE_PAD_TYPE_ANALOGPAD: // scph1150
				analogpar[1] = 0x73;
				analogpar[3] = pad->buttonStatus & 0xff;
				analogpar[4] = pad->buttonStatus >> 8;
				analogpar[5] = pad->rightJoyX;
				analogpar[6] = pad->rightJoyY;
				analogpar[7] = pad->leftJoyX;
				analogpar[8] = pad->leftJoyY;

				memcpy(buf, analogpar, 9);
				bufcount = 8;
				break;
			case PSE_PAD_TYPE_ANALOGJOY: // scph1110
				analogpar[1] = 0x53;
				analogpar[3] = pad->buttonStatus & 0xff;
				analogpar[4] = pad->buttonStatus >> 8;
				analogpar[5] = pad->rightJoyX;
				analogpar[6] = pad->rightJoyY;
				analogpar[7] = pad->leftJoyX;
				analogpar[8] = pad->leftJoyY;

				memcpy(buf, analogpar, 9);
				bufcount = 8;
				break;
			case 0 : //nothing plugged in
				buf[0] = 0xFF ;
				buf[1] = 0xFF ;
				buf[2] = 0xFF ;
				buf[3] = 0xFF ;
				buf[4] = 0xFF ;
				bufcount = 4 ;
				break ;
			case PSE_PAD_TYPE_STANDARD:
			default:
				stdpar[3] = pad->buttonStatus & 0xff;
				stdpar[4] = pad->buttonStatus >> 8;

				memcpy(buf, stdpar, 5);
				bufcount = 4;
		}
	}

	//writexbox("ending padpoll\r\n") ;
	return buf[bufc++];
}

unsigned char PAD__poll(const unsigned char value) {
	//writexbox("padpoll\r\n") ;
	if (bufc > bufcount) return 0xff;
	return buf[bufc++];
}

unsigned char PAD__startPoll(int pad) {
	PadDataS padd;

	//writexbox("pad1 startpoll\r\n") ;

	memset( &padd, 0, sizeof(padd) ) ;

	if (pad == 1)	PAD__readPort1(&padd);
	else			PAD__readPort2(&padd);

//	return _PADstartPoll(&padd, m_psxfix_multitap);
	return _PADstartPoll(&padd, 0);
}
