/************************************************************************

Copyright mooby 2002

CDRMooby2 Preferences.hpp
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/

#pragma warning(disable:4786)

#ifndef PREFERENCES_HPP
#define PREFERENCES_HPP

#include <map>
#include <list>
#include <string>
#include <FL/Fl_Preferences.h>

static char* repeatString = "repeat";
static char* volumeString = "volume";
static char* autorunString = "autorun";
static char* lastrunString = "lastrun";
static char* cacheSizeString = "cachesize";
static char* cachingModeString = "cachemode";
static char* subEnableString = "subenable";

// these are the repeat mode strings
static char* repeatAllString = "repeatAll";
static char* repeatOneString = "repeatOne";
static char* playOneString = "playOne";
static char* oldCachingString = "old";
static char* newCachingString = "new";

// a wrapper for preference information.
class Preferences
{
public:
   Preferences();
   ~Preferences();
	void write();

private:
   void open();
   bool initialized;

public:
   std::map<std::string, std::string> prefsMap;
   std::list<std::string> allPrefs;
};

#endif
