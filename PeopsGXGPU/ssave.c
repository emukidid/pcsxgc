/***************************************************************************
                           ssave.c  -  description
                             -------------------
    begin                : Sun Mar 08 2009
    copyright            : (C) 1999-2009 by Pete Bernert
    web                  : www.pbernert.com   
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
// 2009/03/08 - Pete  
// - generic cleanup for the Peops release
//
//*************************************************************************// 

#include "stdafx.h"

#include "externals.h"

////////////////////////////////////////////////////////////////////////
// screensaver stuff: dynamically load kernel32.dll to avoid export dependeny
////////////////////////////////////////////////////////////////////////
#if 0
void EnableScreenSaver(BOOL bEnab)
{
 HINSTANCE hKernel32 = NULL;
 EXECUTION_STATE (WINAPI *D_SetThreadExecutionState)(EXECUTION_STATE esFlags);

 if(bEnab)
  {
   hKernel32 = LoadLibrary("kernel32.dll");
   if(hKernel32 != NULL)
    {
     if((D_SetThreadExecutionState=(EXECUTION_STATE (WINAPI *)(EXECUTION_STATE))GetProcAddress(hKernel32,"SetThreadExecutionState"))!=NULL)
      D_SetThreadExecutionState(ES_SYSTEM_REQUIRED|ES_DISPLAY_REQUIRED);
     FreeLibrary(hKernel32);
    }
  }
 else
  {
   hKernel32 = LoadLibrary("kernel32.dll");
   if(hKernel32 != NULL)
    {
     if((D_SetThreadExecutionState=(EXECUTION_STATE (WINAPI *)(EXECUTION_STATE))GetProcAddress(hKernel32,"SetThreadExecutionState"))!=NULL)
      D_SetThreadExecutionState(ES_SYSTEM_REQUIRED|ES_DISPLAY_REQUIRED|ES_CONTINUOUS);
     FreeLibrary(hKernel32);
    }
  }
}

#endif
