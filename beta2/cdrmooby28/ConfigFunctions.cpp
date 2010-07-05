/************************************************************************

Copyright mooby 2002

CDRMooby2 ConfigFunctions.cpp
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/
#ifdef WINDOWS

#pragma warning(disable:4786)

#include "defines.h"

#include "externs.h"

#include <FL/Fl.H>
#include <FL/fl_ask.h>
#include <FL/Fl_Window.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Check_Button.H>

#include <iostream>
#include "Utils.hpp"
#include "Preferences.hpp"
#include "ConfigFunctions.hpp"

extern Preferences prefs;

void ConfigWindow::makeWindow()
{
  { 
    Fl_Window* o = new Fl_Window(500, 500, "CDRMooby2 Config");
    w = o;
    { Fl_Check_Button* o = new Fl_Check_Button(20,20,220,40, "Repeat all CDDA tracks");
      o->value(prefs.prefsMap[repeatString] == repeatAllString);
      o->callback((Fl_Callback*)repeatAllCDDA, this);
      repeatAllButton = o;
    }
    { Fl_Check_Button* o = new Fl_Check_Button(20,60,220,40, "Repeat one CDDA track");
      o->value(prefs.prefsMap[repeatString] == repeatOneString);
      o->callback((Fl_Callback*)repeatOneCDDA, this);
      repeatOneButton = o;
    }
    { Fl_Check_Button* o = new Fl_Check_Button(20,100,220,40, "Play one CDDA track and stop");
      o->value(prefs.prefsMap[repeatString] == playOneString);
      o->callback((Fl_Callback*)playOneCDDA, this);
      playOneButton = o;
    }
    { Fl_Value_Slider* o = new Fl_Value_Slider(20, 140, 210, 25, "CDDA Volume");
      o->type(1);
      o->value(atof(prefs.prefsMap[volumeString].c_str()));
      o->callback((Fl_Callback*)CDDAVolume);
    }
    { Fl_Button* o = new Fl_Button(20, 230, 95, 25, "Compress");
      o->callback((Fl_Callback*)bzCompress);
    }
    { Fl_Button* o = new Fl_Button(130, 230, 95, 25, "Decompress");
      o->callback((Fl_Callback*)bzDecompress);
    }
    { Fl_Button* o = new Fl_Button(20, 305, 95, 25, "Compress");
      o->callback((Fl_Callback*)zCompress);
    }
    { Fl_Button* o = new Fl_Button(130, 305, 95, 25, "Decompress");
      o->callback((Fl_Callback*)zDecompress);
    }
    new Fl_Box(5, 200, 250, 25, "bz.index compression");
    new Fl_Box(5, 280, 250, 25, ".Z.table compression");
    { 
      Fl_Box* o;
      if (prefs.prefsMap[autorunString] == std::string(""))
         o = new Fl_Box(0, 350, 250, 25, "No autorun image selected");   
      else
         o = new Fl_Box(0, 350, 250, 25, prefs.prefsMap[autorunString].c_str());
      autorunBox = o;
    }
    {
      Fl_Button* o = new Fl_Button(20, 380, 200, 25, "Choose an autorun image");
      o->callback((Fl_Callback*)chooseAutorunImage, this);
    }
    {
      Fl_Button* o = new Fl_Button(20, 415, 200, 25, "Clear the autorun image");
      o->callback((Fl_Callback*)clearAutorunImage, this);
    }
    { Fl_Return_Button* o = new Fl_Return_Button(165, 465, 80, 25, "OK");
      o->callback((Fl_Callback*)configOK, this);
    }
    { Fl_Check_Button* o = new Fl_Check_Button(270,50,220,40, "Enable subchannel data");
      o->value(prefs.prefsMap[subEnableString] != std::string());
      o->callback((Fl_Callback*)subEnable);
    }
    { Fl_Check_Button* o = new Fl_Check_Button(270,100,220,40, "Use new caching (may be slower)");
      o->value(prefs.prefsMap[cachingModeString] == newCachingString);
      o->callback((Fl_Callback*)newCaching);
    }
    { Fl_Value_Slider* o = new Fl_Value_Slider(270, 140, 210, 25, "Cache size (1 frame = 2353 bytes)");
      o->type(1);
      o->range(1, 50000);
      o->step(1);
      o->value(atoi(prefs.prefsMap[cacheSizeString].c_str()));
      o->callback((Fl_Callback*)cacheSize);
    }
    o->set_modal();
    o->end();
  }
}



/** configure plugin external functions **/

long CALLBACK CDRconfigure(void)
{
   RunConfig();
   return 0;
}


int    CD_Configure(UINT32 *par)
{
	RunConfig();
   return 0;
}

void CALLBACK CDVDconfigure()
{
   CDRconfigure();
}
#endif
