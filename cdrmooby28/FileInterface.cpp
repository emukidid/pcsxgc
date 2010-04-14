/************************************************************************

Copyright mooby 2002

CDRMooby2 FileInterface.cpp
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/

#ifdef WINDOWS
#pragma warning(disable:4786)
#endif

#include "FileInterface.hpp"
#include "TrackParser.hpp"
#include "Utils.hpp"
#include "Preferences.hpp"

#include <sstream>

#include <stdio.h>
#ifdef WINDOWS
#include <bzlib.h>
#include <zlib.h>

#include "unrar/unrarlib.h"
bool RARFileInterface::alreadyUncompressed = false;
unsigned char* RARFileInterface::theFile = NULL;
unsigned long RARFileInterface::length = 0;
#endif

extern "C" {
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "../fileBrowser/fileBrowser-DVD.h"
#include "../fileBrowser/fileBrowser-CARD.h"
extern fileBrowser_file *isoFile; 
extern fileBrowser_file *cddaFile;
extern fileBrowser_file *subFile;
};

// leave this here or the unrarlib will complain about errors
using namespace std;

extern Preferences prefs;

FileInterface::FileInterface(const unsigned long requestedFrames, 
      const unsigned long requiredFrames)
{
  
  bufferFrames = 0;
  cacheMode = oldMode;
  fileBuffer = NULL; 
  pregapTime = CDTime(99,59,74);
  pregapLength = CDTime(0,0,0);
   
   //cache.setMaxSize(atoi(prefs.prefsMap[cacheSizeString].c_str()));
   cache.setMaxSize(1);
   if (requiredFrames != 0)
   {
      bufferFrames = (requestedFrames < requiredFrames) ? requiredFrames : requestedFrames;
      fileBuffer = new unsigned char[bufferFrames * bytesPerFrame];
   }
      // set the holdout size
//   if (prefs.prefsMap[cachingModeString] == newCachingString)
//      cacheMode = newMode;
//   else if (prefs.prefsMap[cachingModeString] == oldCachingString)
      cacheMode = oldMode;
}


// given a file name, return the extension of the file and
// return the correct FileInterface for that file type
FileInterface* FileInterfaceFactory(const std::string& filename,
                                    string& extension, int type)
{
		// for every file type that's supported, try to match the extension
   FileInterface* image;
   char buf[1024];
   memset( buf, '\0', 1024 );

   if (extensionMatches(filename, ".ccd"))
   {
      moobyMessage("Please open the image and not the ccd file.");
      image = new UncompressedFileInterface(1);
      extension = filename.substr(filename.find_last_of('.'));
      filename.copy(buf, filename.find_last_of('.'));
      image->openFile(buf + string(".img"), type);
   }

		// the CUE interface will take the name of the file
		// in the cue sheet and open it as an uncompressed file
   else if (extensionMatches(filename, ".cue"))
   {
      moobyMessage("Please open the image and not the cue sheet.");
      extension = filename.substr(filename.find_last_of('.'));
      CueParser cp(filename);
      cp.parse();
      image = new UncompressedFileInterface(1);

         // try to figure out the directory of the image.
         // if none is in the cue sheet,
         // just use the directory of filename.
      std::string::size_type pos = cp.getCDName().rfind('/');
      if (pos == std::string::npos)
      {
         pos = cp.getCDName().rfind('\\');
      }
      if (pos == std::string::npos)
      {
         pos = filename.rfind('/');
         if (pos == std::string::npos)
            pos = filename.rfind('\\');
         image->openFile(std::string(filename).erase(pos + 1) + cp.getCDName(), type);
      }
      else
      {
         image->openFile(cp.getCDName(), type);
      }
   }
		// all other file types that aren't directly supported, 
		// try to open them with UncompressedFileInterface
   else
   {
      if (extensionMatches(filename, ".iso"))
      {
         moobyMessage("This plugin does not support ISO-9660 images. "
            "If this is a binary image, rename it with a \".bin\" extension.");
      }

      extension = filename.substr(filename.find_last_of('.'));
      image = new UncompressedFileInterface(1);
      image->openFile(filename, type);
   }

   if (image)
      return image;
   else
   {
      Exception e(string("Error opening file: ") + filename);
      THROW(e);
   }
}

FileInterface& FileInterface::setPregap(const CDTime& gapLength,
                                        const CDTime& gapTime)
{ 
   if (pregapLength == CDTime(0,0,0))
   {
      pregapLength = gapLength; 
      pregapTime = gapTime; 
      CDLength += gapLength; 
   }
   return *this; 
}

// opens the file and calculates the length of the cd
void FileInterface::openFile(const std::string& str, int type)
      throw(Exception)
{
  fileBrowser_file tempFile;
  memset(&tempFile, 0, sizeof(fileBrowser_file));
  strcpy(&tempFile.name[0], str.c_str());

	if(isoFile_open(&tempFile) == FILE_BROWSER_ERROR_NO_FILE) {
    Exception e(std::string("Cannot open file: ") + str + "\r\n");
    THROW(e);
  }
  
  if(type==FILE_BROWSER_ISO_FILE_PTR) {
    memcpy(isoFile, &tempFile, sizeof(fileBrowser_file));
    isoFile->attr = type;
    filePtr = isoFile;
  }
  else if(type==FILE_BROWSER_CDDA_FILE_PTR) {
    memcpy(cddaFile, &tempFile, sizeof(fileBrowser_file));
    cddaFile->attr = type;
    filePtr = cddaFile;
  }
  else if(type==FILE_BROWSER_SUB_FILE_PTR) {
    memcpy(subFile, &tempFile, sizeof(fileBrowser_file));
    subFile->attr = type;
    filePtr = subFile;
  }
  
  isoFile_seekFile(filePtr,0,FILE_BROWSER_SEEK_SET);
  fileName = str;
  CDLength= CDTime(filePtr->size, CDTime::abByte) + CDTime(0,2,0);

  bufferPos.setMSF(MSFTime(255,255,255));
}

// reads data into the cache for UncompressedFileInterface
void UncompressedFileInterface::seekUnbuffered(const CDTime& cdt)
   throw(std::exception, Exception)
{
   CDTime seekTime(cdt - CDTime(0,2,0));
   isoFile_seekFile(filePtr,seekTime.getAbsoluteByte(),FILE_BROWSER_SEEK_SET);
   isoFile_readFile(filePtr,(char*)fileBuffer,bufferFrames * bytesPerFrame);
   bufferPointer = fileBuffer;
   bufferPos = cdt;
   bufferEnd = cdt + CDTime(bufferFrames, CDTime::abFrame);
}

