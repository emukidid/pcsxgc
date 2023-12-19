/**
 * WiiSX - ConfigureButtonsFrame.h
 * Copyright (C) 2010 sepp256
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

#ifndef CONFIGUREBUTTONSFRAME_H
#define CONFIGUREBUTTONSFRAME_H

#include "../libgui/Frame.h"
#include "MenuTypes.h"

class ConfigureButtonsFrame : public menu::Frame
{
public:
	ConfigureButtonsFrame();
	~ConfigureButtonsFrame();
	void activateSubmenu(int submenu);
	void updateFrame(float deltaTime);
	void drawChildren(menu::Graphics& gfx);

	enum ConfigureButtonsSubmenus
	{
		SUBMENU_PSX_PAD0=0,
		SUBMENU_PSX_PAD1,
		SUBMENU_PSX_PAD2,
		SUBMENU_PSX_PAD3,
		SUBMENU_PSX_PADNONE,
	};

private:

};

#endif
