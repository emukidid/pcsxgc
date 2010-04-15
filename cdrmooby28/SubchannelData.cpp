/************************************************************************

Copyright mooby 2002

CDRMooby2 SubchannelData.cpp
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/

#ifdef WINDOWS
#pragma warning(disable:4786)
#endif

#include "SubchannelData.hpp"
#include "Preferences.hpp"

extern "C" {
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "../fileBrowser/fileBrowser-DVD.h"
#include "../fileBrowser/fileBrowser-CARD.h"
extern fileBrowser_file *subFile;
};

extern Preferences prefs;

using namespace std;

#include <fstream>

// tries to open the subchannel files to determine which
// one to use
SubchannelData* SubchannelDataFactory(const std::string& fileroot, int type)
{
   SubchannelData* scd = NULL;

   if (prefs.prefsMap[subEnableString] == std::string())
   {
      scd = new DisabledSubchannelData();
      return scd;
   }
   fileBrowser_file tempFile;
   memset(&tempFile, 0, sizeof(fileBrowser_file));
   strcpy(&tempFile.name[0], (fileroot + std::string(".sub")).c_str());

   if(isoFile_open(&tempFile) != FILE_BROWSER_ERROR_NO_FILE)
   {
      scd = new SUBSubchannelData();
      scd->openFile(fileroot + std::string(".sub"), type);
      return scd;
   }
   
   memset(&tempFile, 0, sizeof(fileBrowser_file));
   strcpy(&tempFile.name[0], (fileroot + std::string(".sbi")).c_str());
   if(isoFile_open(&tempFile) != FILE_BROWSER_ERROR_NO_FILE)
   {
      scd = new SBISubchannelData();
      scd->openFile(fileroot + std::string(".sbi"), type);
      return scd;
   }

   memset(&tempFile, 0, sizeof(fileBrowser_file));
   strcpy(&tempFile.name[0], (fileroot + std::string(".m3s")).c_str());
   if(isoFile_open(&tempFile) != FILE_BROWSER_ERROR_NO_FILE)
   {
      scd = new M3SSubchannelData();
      scd->openFile(fileroot + std::string(".m3s"), type);
      return scd;
   }

   scd = new NoSubchannelData();
   return scd;
}

SUBSubchannelData::SUBSubchannelData() 
{
      // set the cache to be the size given in the prefs
   cache.setMaxSize(atoi(prefs.prefsMap[cacheSizeString].c_str()));
}

// SUB files read from the file whenever data is needed
void SUBSubchannelData::openFile(const string& file, int type)
   throw(Exception)
{
   fileBrowser_file tempFile;
   memset(&tempFile, 0, sizeof(fileBrowser_file));
   strcpy(&tempFile.name[0], file.c_str());
   if(isoFile_open(&tempFile) != FILE_BROWSER_ERROR_NO_FILE) {
     if(subFile) {
       isoFile_deinit(subFile);
     }
     memcpy(subFile, &tempFile, sizeof(fileBrowser_file));
     subFile->attr = type;
     filePtr = subFile;
   }
}

void SUBSubchannelData::seek(const CDTime& cdt)
   throw(Exception)
{
	// seek in the file for the data requested and set the subframe
	// data
   try
   {
         // try the cache first
      if (enableCache && cache.find(cdt, sf))
      {
         return;
      }
      
      isoFile_seekFile(filePtr,(cdt.getAbsoluteFrame() - 150) * SubchannelFrameSize,FILE_BROWSER_SEEK_SET);
      isoFile_readFile(filePtr,(char*)sf.subData, SubchannelFrameSize);


      if (enableCache)
         cache.insert(cdt, sf);
   }
   catch(...)
   {
      sf.setTime(CDTime(cdt));
   }
}

// opens the SBI file and caches all the subframes in a map
void SBISubchannelData::openFile(const std::string& file, int type) 
      throw(Exception)
{
   fileBrowser_file tempFile;
   memset(&tempFile, 0, sizeof(fileBrowser_file));
   strcpy(&tempFile.name[0], file.c_str());
   if(isoFile_open(&tempFile) != FILE_BROWSER_ERROR_NO_FILE) {
     if(subFile) {
       isoFile_deinit(subFile);
     }
     memcpy(subFile, &tempFile, sizeof(fileBrowser_file));
     subFile->attr = type;
     filePtr = subFile;
   }

   try
   {
      unsigned char buffer[4];
      isoFile_seekFile(filePtr,0,FILE_BROWSER_SEEK_SET);
      isoFile_readFile(filePtr,(char*)&buffer, 4);
      
      if (string((char*)&buffer) != string("SBI"))
      {
         Exception e(file + string(" isn't an SBI file"));
         THROW(e);
      }
      for(int i = 0; i < filePtr->size; i+=4)
      {
         isoFile_seekFile(filePtr,i,FILE_BROWSER_SEEK_SET);
         isoFile_readFile(filePtr,(char*)&buffer, 4);
 
         CDTime now((unsigned char*)&buffer, msfbcd);
         SubchannelFrame subf(now);
            // numbers are BCD in file, so only convert the
            // generated subchannel data
         bool convert1 = false,
            convert2 = false;
         switch(buffer[3])
         {
         case 1:
            isoFile_readFile(filePtr,(char*)&subf.subData[12], 10);
            break;
         case 2:
            isoFile_readFile(filePtr,(char*)&subf.subData[15], 3);
            break;
         case 3:
            isoFile_readFile(filePtr,(char*)&subf.subData[19], 3);
            break;
         default:
            Exception e("Unknown buffer type in SBI file");
            THROW(e);
            break;
         }
         subMap[now] = subf;
      }
   }
   catch(Exception&)
   {
      throw;
   }
}

// if the data is in the map, return it.  otherwise, make up data
void SBISubchannelData::seek(const CDTime& cdt)
   throw(Exception)
{
   map<CDTime, SubchannelFrame>::iterator itr = subMap.find(cdt);
   if (itr == subMap.end())
   {
      sf.setTime(cdt);
   }
   else
   {
      sf = (*itr).second;
   }
}

// opens and caches the M3S data
void M3SSubchannelData::openFile(const std::string& file, int type) 
   throw(Exception)
{
   fileBrowser_file tempFile;
   memset(&tempFile, 0, sizeof(fileBrowser_file));
   strcpy(&tempFile.name[0], file.c_str());
   if(isoFile_open(&tempFile) != FILE_BROWSER_ERROR_NO_FILE) {
     if(subFile) {
       isoFile_deinit(subFile);
     }
     memcpy(subFile, &tempFile, sizeof(fileBrowser_file));
     subFile->attr = type;
     filePtr = subFile;
   }

   CDTime t(3,0,0);
   char buffer[16];
   for(int i = 0; i < filePtr->size; i+=4)
   {
      isoFile_seekFile(filePtr,i,FILE_BROWSER_SEEK_SET);
      isoFile_readFile(filePtr,(char*)&buffer, 16);
 
      SubchannelFrame subf(t);
      memcpy(&subf.subData[12], buffer, 16);
      subMap[t] = subf;
      t += CDTime(0,0,1);         
      if (t == CDTime(4,0,0))
         break;
   }
 
}

// if no data is found, create data. otherwise, return the data found.
void M3SSubchannelData::seek(const CDTime& cdt)
   throw(Exception)
{
   map<CDTime, SubchannelFrame>::iterator itr = subMap.find(cdt);
   if (itr == subMap.end())
   {
      sf.setTime(cdt);
   }
   else
   {
      sf = (*itr).second;
   }
}
