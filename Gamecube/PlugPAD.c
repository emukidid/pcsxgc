/*
	Basic Analog PAD plugin for PCSX Gamecube
	by emu_kidid based on the DC/MacOSX HID plugin

	TODO: Rumble?
*/

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

static BUTTONS PAD_1;
static BUTTONS PAD_2;

char padAutoAssign;
char padType[4];
char padAssign[4];
enum padType //delete me on wiiSXconfig.h creation
{
	PADTYPE_NONE=0,
	PADTYPE_GAMECUBE,
	PADTYPE_WII
};
enum padAssign
{
	PADASSIGN_INPUT0=0,
	PADASSIGN_INPUT1,
	PADASSIGN_INPUT2,
	PADASSIGN_INPUT3
};
enum padAutoAssign
{
	PADAUTOASSIGN_MANUAL=0,
	PADAUTOASSIGN_AUTOMATIC
};

extern void SysPrintf(char *fmt, ...);
extern int stop;

/* Controller type, later do this by a Variable in the GUI */
int controllerType = 0; // 0 = standard, 1 = analog (analog fails on old games)
long  PadFlags = 0;

virtualControllers_t virtualControllers[4];

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
	for(i=0; i<4; ++i)
		if(virtualControllers[i].inUse) DO_CONTROL(i, pause);
}

void resumeInput(void){
	int i;
	for(i=0; i<4; ++i)
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

long PAD__init(long flags) {
  int i,t,w;
  
  PadFlags |= flags;

	init_controller_ts();

	// 'Initialize' the unmapped virtual controllers
//	for(i=0; i<4; ++i){
//		unassign_controller(i);
//	}

	int num_assigned[num_controller_t];
	memset(num_assigned, 0, sizeof(num_assigned));

	// Map controllers in the priority given
	// Outer loop: virtual controllers
	for(i=0; i<4; ++i){
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
	for(; i<4; ++i){
		unassign_controller(i);
		padType[i] = PADTYPE_NONE;
	}

	return PSE_PAD_ERR_SUCCESS;
}

long PAD__shutdown(void) {
	return PSE_PAD_ERR_SUCCESS;
}

long PAD__open(void)
{
	return PSE_PAD_ERR_SUCCESS;
}

long PAD__close(void) {
	return PSE_PAD_ERR_SUCCESS;
}

long PAD__readPort1(PadDataS* pad) {

  if( virtualControllers[0].inUse && DO_CONTROL(0, GetKeys, (BUTTONS*)&PAD_1) ) {
    stop = 1;
  }

  if(controllerType==1) {
  	pad->controllerType = PSE_PAD_TYPE_ANALOGPAD; 	// Analog Pad  (Right JoyStick)
  	pad->leftJoyX  = PAD_1.leftStickX;
    pad->leftJoyY  = PAD_1.leftStickY;
    pad->rightJoyX = PAD_1.rightStickX;
    pad->rightJoyY = PAD_1.rightStickY;
	}
  else if(!controllerType) {
  	pad->controllerType = PSE_PAD_TYPE_STANDARD; 	// Standard Pad
	}

  pad->buttonStatus = PAD_1.Value;  //set the buttons
  
	return PSE_PAD_ERR_SUCCESS;
}

long PAD__readPort2(PadDataS* pad) {

  if( virtualControllers[1].inUse && DO_CONTROL(1, GetKeys, (BUTTONS*)&PAD_2) ) {
    stop = 1;
  }

  if(controllerType==1) {
  	pad->controllerType = PSE_PAD_TYPE_ANALOGPAD; 	// Analog Pad  (Right JoyStick)
  	pad->leftJoyX  = PAD_2.leftStickX;
    pad->leftJoyY  = PAD_2.leftStickY;
    pad->rightJoyX = PAD_2.rightStickX;
    pad->rightJoyY = PAD_2.rightStickY;
	}
  else if(!controllerType) {
  	pad->controllerType = PSE_PAD_TYPE_STANDARD; 	// Standard Pad
	}

  pad->buttonStatus = PAD_2.Value;  //set the buttons
  
	return PSE_PAD_ERR_SUCCESS;
}
