/***************************************************************************
                      gpuPeopsSoft.c  -  description
                             -------------------
    begin                : Sun Oct 28 2001
    copyright            : (C) 2001 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************// 
// History of changes:
//
// 2001/10/28 - Pete  
// - generic cleanup (well... not much in this file) for the Peops release
//
//*************************************************************************// 

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////
// GENERIC FUNCS //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#if defined(__GX__) || defined(__SWITCH__)
#else
HINSTANCE hInst=NULL;

BOOL APIENTRY DllMain(HANDLE hModule,                  // DLL INIT
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
 hInst=(HINSTANCE)hModule;
 return TRUE;                                          // very quick :)
}
#endif //!__GX__
