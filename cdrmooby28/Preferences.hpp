/************************************************************************

Copyright mooby 2002

CDRMooby2 Preferences.hpp
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/

#ifdef WINDOWS
#pragma warning(disable:4786)
#endif

#ifndef PREFERENCES_HPP
#define PREFERENCES_HPP

#include <map>
#include <list>
#include <string>

#ifdef WINDOWS
#include <FL/Fl_Preferences.h>
#endif

static const char* repeatString = "repeat";
static const char* volumeString = "volume";
static const char* autorunString = "autorun";
static const char* lastrunString = "lastrun";
static const char* cacheSizeString = "cachesize";
static const char* cachingModeString = "cachemode";
static const char* subEnableString = "subenable";

// these are the repeat mode strings
static const char* repeatAllString = "repeatAll";
static const char* repeatOneString = "repeatOne";
static const char* playOneString = "playOne";
static const char* oldCachingString = "old";
static const char* newCachingString = "new";

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
