/**
 * WiiSX - controller.h
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * Standard prototypes for accessing different controllers
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
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


#ifndef CONTROLLER_H
#define CONTROLLER_H

extern char padNeedScan, wpadNeedScan;
extern u32 gc_connected;

void control_info_init(void);

typedef union {
	u16 All;
	struct {
  	unsigned SQUARE_BUTTON    : 1;
  	unsigned CROSS_BUTTON     : 1;
  	unsigned CIRCLE_BUTTON    : 1;
  	unsigned TRIANGLE_BUTTON  : 1;
  	unsigned R1_BUTTON        : 1;
  	unsigned L1_BUTTON        : 1;
  	unsigned R2_BUTTON        : 1;
  	unsigned L2_BUTTON        : 1;
  	unsigned L_DPAD           : 1;
  	unsigned D_DPAD           : 1;
  	unsigned R_DPAD           : 1;
  	unsigned U_DPAD           : 1;
  	unsigned START_BUTTON     : 1;
		unsigned UNK_1            : 1;
		unsigned UNK_2            : 1;
		unsigned SELECT_BUTTON    : 1;	
	};
} _BUTTONS;

typedef struct {
  _BUTTONS btns;
  u8 leftStickX;
	u8 leftStickY;
	u8 rightStickX;
	u8 rightStickY;
} BUTTONS;
  

typedef struct {
	// Call GetKeys to read in BUTTONS for a controller of this type
	// You should pass in controller num for this type
	//   Not for the player number assigned
	//   (eg use GC Controller 1, not player 1)
	int (*GetKeys)(int, BUTTONS*);
	// Interactively configure the button mapping
	void (*configure)(int);
	// Initialize the controllers, filling out available
	void (*init)(void);
	// Assign actual controller to virtual controller
	void (*assign)(int,int);
	// Pause/Resume a controller
	void (*pause)(int);
	void (*resume)(int);
	// Rumble controller (0 stops rumble)
	void (*rumble)(int, int);
	// Controllers plugged in/available of this type
	char available[4];
} controller_t;

typedef struct _virtualControllers_t {
	BOOL          inUse;   // This virtual controller is being controlled
	controller_t* control; // The type of controller being used
	int           number;  // The physical controller number
} virtualControllers_t;

extern virtualControllers_t virtualControllers[4];

// List of all the defined controller_t's
#define num_controller_t 3
extern controller_t controller_GC;
extern controller_t controller_Classic;
extern controller_t controller_WiimoteNunchuk;
extern controller_t* controller_ts[num_controller_t];

void init_controller_ts(void);
void assign_controller(int whichVirtual, controller_t*, int whichPhysical);
void unassign_controller(int whichVirtual);

#endif
