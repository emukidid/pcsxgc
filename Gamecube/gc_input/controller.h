/**
 * WiiSX - controller.h
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * Copyright (C) 2007, 2008, 2009, 2010 sepp256
 * 
 * Standard prototypes for accessing different controllers
 *
 * WiiSX homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *                emukidid@gmail.com
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


#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdio.h>

extern char padNeedScan, wpadNeedScan;
extern u32 gc_connected;

void control_info_init(void);
void auto_assign_controllers(void);

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
	int index;
	unsigned int mask;
	char* name;
} button_t;

typedef button_t* button_tp;

#define CONTROLLER_CONFIG_VERSION 1
typedef struct {
	button_tp SQU, CRO, CIR, TRI;
	button_tp R1, L1, R2, L2, R3, L3;
	button_tp DL, DR, DU, DD;
	button_tp START, SELECT;
	button_tp analogL, analogR, exit;
	int invertedYL, invertedYR;
} controller_config_t;

typedef struct {
	// Identifier used in configuration file names
	char identifier;
	// Call GetKeys to read in BUTTONS for a controller of this type
	// You should pass in controller num for this type
	//   Not for the player number assigned
	//   (eg use GC Controller 1, not player 1)
	int (*GetKeys)(int, BUTTONS*, controller_config_t*);
	// Set the configuration for a controller of this type
	// You should pass in physical controller num as above
	void (*configure)(int, controller_config_t*);
	// Assign actual controller to virtual controller
	void (*assign)(int,int);
	// Pause/Resume a controller
	void (*pause)(int);
	void (*resume)(int);
	// Rumble controller (0 stops rumble)
	void (*rumble)(int, int);
	// Fill in available[] for this type
	void (*refreshAvailable)(void);
	// Controllers plugged in/available of this type
	char available[4];
	// Number of buttons available for this controller type
	int num_buttons;
	// Pointer to buttons for this controller type
	button_t* buttons;
	// Number of analog sources available for this controller type
	int num_analog_sources;
	// Pointer to analog sources for this controller type
	button_t* analog_sources;
	// Number of menu combos available for this controller type
	int num_menu_combos;
	// Pointer to analog sources for this controller type
	button_t* menu_combos;
	// Default controller mapping
	controller_config_t config_default; 
	// Current controller mapping
	controller_config_t config[4]; 
	// Saved controller mappings
	controller_config_t config_slot[4]; 
} controller_t;

typedef struct _virtualControllers_t {
	BOOL          inUse;   // This virtual controller is being controlled
	controller_t* control; // The type of controller being used
	int           number;  // The physical controller number
	controller_config_t* config; // This is no longer needed...
} virtualControllers_t;

extern virtualControllers_t virtualControllers[2];

// List of all the defined controller_t's
#if defined(WII) && !defined(NO_BT)

#define num_controller_t 4
extern controller_t controller_GC;
extern controller_t controller_Classic;
extern controller_t controller_WiimoteNunchuk;
extern controller_t controller_Wiimote;
extern controller_t* controller_ts[num_controller_t];

#else // WII && !NO_BT

#define num_controller_t 1
extern controller_t controller_GC;
extern controller_t* controller_ts[num_controller_t];

#endif // WII && !NO_BT

void init_controller_ts(void);
void assign_controller(int whichVirtual, controller_t*, int whichPhysical);
void unassign_controller(int whichVirtual);

int load_configurations(FILE*, controller_t*);
void save_configurations(FILE*, controller_t*);

#endif
