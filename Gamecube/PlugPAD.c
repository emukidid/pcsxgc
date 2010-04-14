/**
 * WiiSX - PlugPAD.c
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

PadDataS lastport1;
PadDataS lastport2;

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
	  &controller_Wiimote,
	 };
#else
	{ &controller_GC,
	 };
#endif

// Use to invoke func on the mapped controller with args
#define DO_CONTROL(Control,func,args...) \
	virtualControllers[Control].control->func( \
		virtualControllers[Control].number, ## args)

void control_info_init(void){
	//Call once during emulator start to auto assign controllers
	init_controller_ts();
	auto_assign_controllers();
}

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
	int i, j;
	for(i=0; i<num_controller_t; ++i){
		controller_ts[i]->refreshAvailable();

		for(j=0; j<4; ++j){
			memcpy(&controller_ts[i]->config[j],
			       &controller_ts[i]->config_default,
			       sizeof(controller_config_t));
			memcpy(&controller_ts[i]->config_slot[j],
			       &controller_ts[i]->config_default,
			       sizeof(controller_config_t));
		}
	}
}

void assign_controller(int wv, controller_t* type, int wp){
	virtualControllers[wv].control = type;
	virtualControllers[wv].inUse   = 1;
	virtualControllers[wv].number  = wp;
	virtualControllers[wv].config  = &type->config[wv];

	type->assign(wp,wv);
}

void unassign_controller(int wv){
	virtualControllers[wv].control = NULL;
	virtualControllers[wv].inUse   = 0;
	virtualControllers[wv].number  = -1;
}

void auto_assign_controllers(void)
{
	//TODO: Map 5 or 8 controllers if multitaps are used.
	int i,t,w;
	int num_assigned[num_controller_t];

//	init_controller_ts();

	memset(num_assigned, 0, sizeof(num_assigned));

	// Map controllers in the priority given
	// Outer loop: virtual controllers
	for(i=0; i<2; ++i){
		// Middle loop: controller type
		for(t=0; t<num_controller_t; ++t){
			controller_t* type = controller_ts[t];
			type->refreshAvailable();

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

int load_configurations(FILE* f, controller_t* controller){
	int i,j;
	char magic[4] = { 
		'W', 'X', controller->identifier, CONTROLLER_CONFIG_VERSION
	};
	char actual[4];
	fread(actual, 1, 4, f);
	if(memcmp(magic, actual, 4))
		return 0;
	
	inline button_t* getPointer(button_t* list, int size){
		int index;
		fread(&index, 4, 1, f);
		return list + (index % size);
	}
	inline button_t* getButton(void){
		return getPointer(controller->buttons, controller->num_buttons);
	}
	
	for(i=0; i<4; ++i){
		controller->config_slot[i].SQU = getButton();
		controller->config_slot[i].CRO = getButton();
		controller->config_slot[i].CIR = getButton();
		controller->config_slot[i].TRI = getButton();
		
		controller->config_slot[i].R1 = getButton();
		controller->config_slot[i].L1 = getButton();
		controller->config_slot[i].R2 = getButton();
		controller->config_slot[i].L2 = getButton();
		controller->config_slot[i].R3 = getButton();
		controller->config_slot[i].L3 = getButton();

		controller->config_slot[i].DL = getButton();
		controller->config_slot[i].DR = getButton();
		controller->config_slot[i].DU = getButton();
		controller->config_slot[i].DD = getButton();

		controller->config_slot[i].START  = getButton();
		controller->config_slot[i].SELECT = getButton();
				
		controller->config_slot[i].analogL = 
			getPointer(controller->analog_sources, controller->num_analog_sources);
		controller->config_slot[i].analogR = 
			getPointer(controller->analog_sources, controller->num_analog_sources);
		controller->config_slot[i].exit =
			getPointer(controller->menu_combos, controller->num_menu_combos);
		fread(&controller->config_slot[i].invertedYL, 4, 1, f);
		fread(&controller->config_slot[i].invertedYR, 4, 1, f);
	}

	if (loadButtonSlot != LOADBUTTON_DEFAULT)
		for(j=0; j<4; ++j)
			memcpy(&controller->config[j],
			       &controller->config_slot[(int)loadButtonSlot],
			       sizeof(controller_config_t));
	
	return 1;
}

void save_configurations(FILE* f, controller_t* controller){
	int i;
	char magic[4] = { 
		'W', 'X', controller->identifier, CONTROLLER_CONFIG_VERSION
	};
	fwrite(magic, 1, 4, f);
	
	for(i=0; i<4; ++i){
		fwrite(&controller->config_slot[i].SQU->index, 4, 1, f);
		fwrite(&controller->config_slot[i].CRO->index, 4, 1, f);
		fwrite(&controller->config_slot[i].CIR->index, 4, 1, f);
		fwrite(&controller->config_slot[i].TRI->index, 4, 1, f);
		
		fwrite(&controller->config_slot[i].R1->index, 4, 1, f);
		fwrite(&controller->config_slot[i].L1->index, 4, 1, f);
		fwrite(&controller->config_slot[i].R2->index, 4, 1, f);
		fwrite(&controller->config_slot[i].L2->index, 4, 1, f);
		fwrite(&controller->config_slot[i].R3->index, 4, 1, f);
		fwrite(&controller->config_slot[i].L3->index, 4, 1, f);
		
		fwrite(&controller->config_slot[i].DL->index, 4, 1, f);
		fwrite(&controller->config_slot[i].DR->index, 4, 1, f);
		fwrite(&controller->config_slot[i].DU->index, 4, 1, f);
		fwrite(&controller->config_slot[i].DD->index, 4, 1, f);
		
		fwrite(&controller->config_slot[i].START->index, 4, 1, f);
		fwrite(&controller->config_slot[i].SELECT->index, 4, 1, f);
				
		fwrite(&controller->config_slot[i].analogL->index, 4, 1, f);
		fwrite(&controller->config_slot[i].analogR->index, 4, 1, f);
		fwrite(&controller->config_slot[i].exit->index, 4, 1, f);
		fwrite(&controller->config_slot[i].invertedYL, 4, 1, f);
		fwrite(&controller->config_slot[i].invertedYR, 4, 1, f);
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
