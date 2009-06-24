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
#include "plugins.h"
#include "PsxCommon.h"
#include "PSEmu_Plugin_Defs.h"

/* Button Bits */
#define PSX_BUTTON_TRIANGLE ~(1 << 12)
#define PSX_BUTTON_SQUARE 	~(1 << 15)
#define PSX_BUTTON_CROSS	~(1 << 14)
#define PSX_BUTTON_CIRCLE	~(1 << 13)
#define PSX_BUTTON_L2		~(1 << 8)
#define PSX_BUTTON_R2		~(1 << 9)
#define PSX_BUTTON_L1		~(1 << 10)
#define PSX_BUTTON_R1		~(1 << 11)
#define PSX_BUTTON_SELECT	~(1 << 0)
#define PSX_BUTTON_START	~(1 << 3)
#define PSX_BUTTON_DUP		~(1 << 4)
#define PSX_BUTTON_DRIGHT	~(1 << 5)
#define PSX_BUTTON_DDOWN	~(1 << 6)
#define PSX_BUTTON_DLEFT	~(1 << 7)

/* Controller type, later do this by a Variable in the GUI */
//#define TYPE_ANALOG //(Doesn't work on some/ALL?? games)

long  PadFlags = 0;

long PAD__init(long flags) {
	SysPrintf("start PAD_init()\r\n");

//	printf("Flags: %08x\n", flags);
	PadFlags |= flags;

	/* Read Configuration here */

	SysPrintf("end PAD_init()\r\n");
	
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

	int b = PAD_ButtonsHeld(0);
	uint16_t pad_status = 0xFFFF;	//bit pointless why is this done this way?
	
	/* PAD Buttons, Start = Start, Select = Start+Z, Cross = A, Square = B, Triangle = Y, Circle = X */
	if ((b & PAD_BUTTON_START) && (!(b & PAD_TRIGGER_Z)))
		pad_status &= PSX_BUTTON_START;
	if ((b & PAD_BUTTON_START) && (b & PAD_TRIGGER_Z))
		pad_status &= PSX_BUTTON_SELECT;
	if (b & PAD_BUTTON_A)
		pad_status &= PSX_BUTTON_CROSS;
	if (b & PAD_BUTTON_B)
		pad_status &= PSX_BUTTON_SQUARE;
	if (b & PAD_BUTTON_X)
		pad_status &= PSX_BUTTON_CIRCLE;
	if (b & PAD_BUTTON_Y)
		pad_status &= PSX_BUTTON_TRIANGLE;
	if (b & PAD_BUTTON_UP)
		pad_status &= PSX_BUTTON_DUP;
	if (b & PAD_BUTTON_DOWN)
		pad_status &= PSX_BUTTON_DDOWN;
	if (b & PAD_BUTTON_LEFT)
		pad_status &= PSX_BUTTON_DLEFT;
	if (b & PAD_BUTTON_RIGHT)
		pad_status &= PSX_BUTTON_DRIGHT;
	/* Shoulder Buttons, L1 = L, R1 = R, L2 = L+Z, R2 = R+Z */
	if ((b & PAD_TRIGGER_Z) && (b & PAD_TRIGGER_R))
		pad_status &= PSX_BUTTON_R2;
	if ((b & PAD_TRIGGER_Z) && (b & PAD_TRIGGER_L))
		pad_status &= PSX_BUTTON_L2;
	if ((!(b & PAD_TRIGGER_Z)) && (b & PAD_TRIGGER_R))
		pad_status &= PSX_BUTTON_R1;
	if ((!(b & PAD_TRIGGER_Z)) && (b & PAD_TRIGGER_L))
		pad_status &= PSX_BUTTON_L1;
					
#ifdef TYPE_ANALOG
	//adjust values by 128 cause psx values in range 0-255 where 128 is center position
	pad->leftJoyX  = (u8)(PAD_StickX(0)+128);				//analog stick
	pad->leftJoyY  = (u8)(PAD_StickY(0)+128);		
	pad->rightJoyX = (u8)(PAD_SubStickX(0)+128);			//C-stick (Left JoyStick)
	pad->rightJoyY = (u8)(PAD_SubStickY(0)+128);	
	pad->controllerType = PSE_PAD_TYPE_ANALOGJOY; 	// Analog Pad  (Right JoyStick)
#else
	pad->controllerType = PSE_PAD_TYPE_STANDARD; 	// Standard Pad
#endif
	pad->buttonStatus = pad_status;					//Copy Buttons
	return PSE_PAD_ERR_SUCCESS;
}

long PAD__readPort2(PadDataS* pad) {

	int b = PAD_ButtonsHeld(1);
	uint16_t pad_status = 0xFFFF;	//bit pointless why is this done this way?
	
	/* PAD Buttons, Start = Start, Select = Start+Z, Cross = A, Square = B, Triangle = Y, Circle = X */
	if ((b & PAD_BUTTON_START) && (!(b & PAD_TRIGGER_Z)))
		pad_status &= PSX_BUTTON_START;
	if ((b & PAD_BUTTON_START) && (b & PAD_TRIGGER_Z))
		pad_status &= PSX_BUTTON_SELECT;
	if (b & PAD_BUTTON_A)
		pad_status &= PSX_BUTTON_CROSS;
	if (b & PAD_BUTTON_B)
		pad_status &= PSX_BUTTON_SQUARE;
	if (b & PAD_BUTTON_X)
		pad_status &= PSX_BUTTON_CIRCLE;
	if (b & PAD_BUTTON_Y)
		pad_status &= PSX_BUTTON_TRIANGLE;
	if (b & PAD_BUTTON_UP)
		pad_status &= PSX_BUTTON_DUP;
	if (b & PAD_BUTTON_DOWN)
		pad_status &= PSX_BUTTON_DDOWN;
	if (b & PAD_BUTTON_LEFT)
		pad_status &= PSX_BUTTON_DLEFT;
	if (b & PAD_BUTTON_RIGHT)
		pad_status &= PSX_BUTTON_DRIGHT;
	/* Shoulder Buttons, L1 = L, R1 = R, L2 = L+Z, R2 = R+Z */
	if ((b & PAD_TRIGGER_Z) && (b & PAD_TRIGGER_R))
		pad_status &= PSX_BUTTON_R2;
	if ((b & PAD_TRIGGER_Z) && (b & PAD_TRIGGER_L))
		pad_status &= PSX_BUTTON_L2;
	if ((!(b & PAD_TRIGGER_Z)) && (b & PAD_TRIGGER_R))
		pad_status &= PSX_BUTTON_R1;
	if ((!(b & PAD_TRIGGER_Z)) && (b & PAD_TRIGGER_L))
		pad_status &= PSX_BUTTON_L1;
					
#ifdef TYPE_ANALOG
	//adjust values by 128 cause psx values in range 0-255 where 128 is center position
	pad->leftJoyX  = (u8)(PAD_StickX(1)+128);				//analog stick
	pad->leftJoyY  = (u8)(PAD_StickY(1)+128);		
	pad->rightJoyX = (u8)(PAD_SubStickX(1)+128);			//C-stick (Left JoyStick)
	pad->rightJoyY = (u8)(PAD_SubStickY(1)+128);	
	pad->controllerType = PSE_PAD_TYPE_ANALOGJOY; 	// Analog Pad  (Right JoyStick)
#else
	pad->controllerType = PSE_PAD_TYPE_STANDARD; 	// Standard Pad
#endif
	pad->buttonStatus = pad_status;					//Copy Buttons
	return PSE_PAD_ERR_SUCCESS;
}
