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
}

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
                                    string& extension)
{
		// for every file type that's supported, try to match the extension
   FileInterface* image;

#ifdef WINDOWS
   if (extensionMatches(filename, ".rar"))
   {
      extension = filename.substr(filename.size() - string(".rar").size());
      image = new RARFileInterface(1);
      image->openFile(std::string(filename));
   }
   else if (extensionMatches(filename, ".bz.index"))
   {
      extension = filename.substr(filename.size() - string(".bz.index").size());
      extension = filename.substr(filename.rfind(".", filename.rfind(extension) - 1));
      image = new BZIndexFileInterface(1);
      image->openFile(std::string(filename).erase(filename.rfind(".index")));
      
   }
   else if (extensionMatches(filename, ".bz"))
   {
      extension = filename.substr(filename.size() - string(".bz").size());
      extension = filename.substr(filename.rfind(".", filename.rfind(extension) - 1));
      image = new BZIndexFileInterface(1);
      image->openFile(filename);
   }
   else if (extensionMatches(filename, ".Z.table"))
   {
      extension = filename.substr(filename.size() - string(".Z.table").size());
      extension = filename.substr(filename.rfind(".", filename.rfind(extension) - 1));
      image = new ZTableFileInterface(1);
      image->openFile(std::string(filename).erase(filename.rfind(".table")));
   }
   else if (extensionMatches(filename, ".Z"))
   {
      extension = filename.substr(filename.size() - string(".Z").size());
      extension = filename.substr(filename.rfind(".", filename.rfind(extension) - 1));
      image = new ZTableFileInterface(1);
      image->openFile(filename);
   }
   else 
#endif
   if (extensionMatches(filename, ".ccd"))
   {
      moobyMessage("Please open the image and not the ccd file.");
      image = new UncompressedFileInterface(1);
      extension = filename.substr(filename.size() - string(".ccd").size());
      image->openFile(filename.substr(filename.size() - string(".ccd").size()) 
         + string(".img"));
   }

		// the CUE interface will take the name of the file
		// in the cue sheet and open it as an uncompressed file
   else if (extensionMatches(filename, ".cue"))
   {
      moobyMessage("Please open the image and not the cue sheet.");
      extension = filename.substr(filename.size() - string(".cue").size());
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
         image->openFile(std::string(filename).erase(pos + 1) + cp.getCDName());
      }
      else
      {
         image->openFile(cp.getCDName());
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
      image->openFile(filename);
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
void FileInterface::openFile(const std::string& str)
      throw(Exception)
{
	if(isoFile->size <= 0) {
    Exception e(std::string("Cannot open file: ") + str + "\r\n");
    THROW(e);
  }
  isoFile_seekFile(isoFile,0,FILE_BROWSER_SEEK_SET);
  fileName = str;
  CDLength= CDTime(isoFile->size, CDTime::abByte) + CDTime(0,2,0);

  bufferPos.setMSF(MSFTime(255,255,255));
}

#ifdef WINDOWS
// seekUnbuffered for the .index/.table files.
void CompressedFileInterface::seekUnbuffered(const CDTime& cdt)
throw(std::exception, Exception)
{
		// calculate the index in the lookupTable where the data for CDTime cdt
		// should be located
   unsigned long requestedFrame = cdt.getAbsoluteFrame() - 150;
   unsigned long cdtime = requestedFrame / compressedFrames;
   if ((cdtime + 1) >= lookupTable.size())
   {
      Exception e("Seek past end of compressed index\r\n");
      THROW(e);
   }
   unsigned long seekStart = lookupTable[cdtime];
   unsigned long seekEnd = lookupTable[cdtime + 1];

		// read and decompress that data
   file.clear();
   file.seekg(seekStart, ios::beg);
   file.read((char*)compressedDataBuffer, seekEnd - seekStart);
   unsigned int destLen = compressedFrames * bytesPerFrame;

   decompressData ((char*)fileBuffer,
                   (char*)compressedDataBuffer,
                   seekEnd - seekStart,
                   destLen);

		// set the buffer pointer - make sure to check if
		// the requested frame doesn't line up with how the file is compressed
   bufferPointer = fileBuffer + (requestedFrame % compressedFrames) * bytesPerFrame;

		// set the buffer start and end times
   bufferPos = CDTime(cdtime * compressedFrames + 150, CDTime::abFrame);
   bufferEnd = CDTime(cdtime * compressedFrames + compressedFrames + 150, CDTime::abFrame);
}

void ZTableFileInterface::openFile(const std::string& str)
   throw(Exception)
{
   // open the z file
   FileInterface::openFile(str);

   // open the table file and read it 
   string indexFileName = str + string(".table");
   ifstream indexFile(indexFileName.c_str(), ios::binary);
   if (!indexFile)
   {
      Exception e(string("Cannot open file: ") + indexFileName);
      THROW(e);
   }

		// a Z.table file is binary where each element represents
		// one compressed frame.  
		//    4 bytes: the offset of the frame in the .Z file
		//    2 bytes: the length of the compressed frame
		// for every frame in the file.  internally, this plugin only
		// uses the offsets, but it needs the last element of the lookupTable
		// to be the entire CD length.  
		// The data is stored little endian, so flipBits will swap the
		// bits if it's not a little endian system
   unsigned long offset;
   short length;

   indexFile.read((char*)&offset, 4);
   indexFile.read((char*)&length, 2);

   while (indexFile)
   {
      flipBits(offset);
      flipBits(length);
      lookupTable.push_back(offset);
      indexFile.read((char*)&offset, 4);
      indexFile.read((char*)&length, 2);
   }

   lookupTable.push_back(offset + length);

		// seek to the end of the CD and use the bufferEnd time as the
		// CDLength
   seekUnbuffered(CDTime((lookupTable.size() - 2) * compressedFrames, CDTime::abFrame) + 
      CDTime(0,2,0));
   CDLength = bufferEnd;
}

// compresses the data in uncompressedData
void ZTableFileInterface::compressData(char* uncompressedData,
                             char* compressedData,
                             unsigned int inputLen,
                             unsigned int& outputLen)
   throw(Exception)
{
   int rc;
   if ( ( rc = compress( (unsigned char*)compressedData,
             (unsigned long*)&outputLen,
             (unsigned char*)uncompressedData,
             inputLen) ) != 0)
   {
      Exception e("ZDecompress error");
      THROW(e);
   }
}

// decompresses the data in compressedData
void ZTableFileInterface::decompressData(char* uncompressedData,
                                          char* compressedData,
                                          unsigned int inputLen,
                                          unsigned int& outputLen) 
   throw(Exception)
{
   int rc;
   if ( (rc = uncompress ( (unsigned char*)uncompressedData,
                           (unsigned long*)&outputLen,
                           (unsigned char*)compressedData,
                           inputLen) ) != 0)
   {
      Exception e("ZDecompress error");
      THROW(e);
   }
}

// table is a table of file positions, each element representing
// the position for that compressed frame.
// sizes is a table of the length of the compressed frame.
// this returns the data for the .Z.table file 
string ZTableFileInterface::toTable(const vector<unsigned long>& table,
                                    const vector<unsigned long>& sizes)
{
   string toReturn;
   vector<unsigned long>::size_type s;
   for (s = 0; s < table.size(); s++)
   {
      unsigned long aNumber= table[s];
      flipBits(aNumber);
      toReturn += string((char*)&aNumber, 4);
      short length = sizes[s];
      flipBits(length);
      toReturn += string((char*)&length, 2);
   }
   return toReturn;
}

// opens a BZ file
void BZIndexFileInterface::openFile(const std::string& str)
   throw(Exception)
{
   // open the bz file
   FileInterface::openFile(str);

   // open the index file and read it into the table
   string indexFileName = str + string(".index");

   ifstream indexFile(indexFileName.c_str(), ios::binary);
   if (!indexFile)
   {
      Exception e(string("Cannot open file: ") + indexFileName);
      THROW(e);
   }

   unsigned long offset;
	
	// the .BZ.table file is arranged so that one entry represents
	// 10 compressed frames.  that's because bzip only works well if
	// you have a large amount of data to compress.  each element in the 
	// table is a 4 byte unsigned integer representing the offset in
	// the .BZ file of that set of 10 compressed frames.  If there are
	// 'n' frames in the uncompressed image, there are 'n/10 + 1' entries
	// in the BZ.index file, where the last entry is the size of the
	// compressed file.

   indexFile.read((char*)&offset, 4);
   
   while (indexFile)
   {
      // if it's big endian, the bits need to be reversed
      flipBits(offset);

      lookupTable.push_back(offset);
      indexFile.read((char*)&offset, 4);
   }

		// to get the CD length, seek to the end of file and decompress the last frame
   seekUnbuffered(CDTime((lookupTable.size() - 2) * compressedFrames, CDTime::abFrame) + 
      CDTime(0,2,0));
   CDLength = bufferEnd;
}

// compresses uncompressedData
void BZIndexFileInterface::compressData(char* uncompressedData,
                             char* compressedData,
                             unsigned int inputLen,
                             unsigned int& outputLen)
   throw(Exception)
{
   int rc;
   if ( (rc = BZ2_bzBuffToBuffCompress( compressedData,
                                      &outputLen,
                                      uncompressedData,
                                      inputLen,
                                      1,
                                      0,
                                      30 ) ) != BZ_OK)
   {
      Exception e("BZCompress error");
      THROW(e);
   }
}

// decompresses compressedData
void BZIndexFileInterface::decompressData(char* uncompressedData,
                                          char* compressedData,
                                          unsigned int inputLen,
                                          unsigned int& outputLen) 
   throw(Exception)
{
   int rc;
   if ( (rc = BZ2_bzBuffToBuffDecompress ( uncompressedData,
                                           &outputLen,
                                           compressedData,
                                           inputLen,
                                           0,
                                           0) ) != BZ_OK)
   {
      Exception e("BZDecompress error");
      THROW(e);
   }
}

// writes the .BZ.index file
string BZIndexFileInterface::toTable(const vector<unsigned long>& table,
                                     const vector<unsigned long>& sizes)
{
   string toReturn;
   vector<unsigned long>::size_type s;
   unsigned long aNumber;
   for (s = 0; s < table.size(); s++)
   {
      aNumber= table[s];
      flipBits(aNumber);
      toReturn += string((char*)&aNumber, 4);
   }
   aNumber = table[table.size() - 1];
   aNumber += sizes[sizes.size() - 1];
   flipBits(aNumber);
   toReturn += string((char*)&aNumber, 4);

   return toReturn;
}


// opens a RAR file.
void RARFileInterface::openFile(const std::string& str) throw(Exception)
{
	// if there's CDDA data, you only want to decompress the file once...
   if (!alreadyUncompressed)
   {
			// first, check for a .cue file in the archive
      ArchiveList_struct *list,*itr;
      char* cueBuffer = NULL;
      unsigned long cueLen;
      string cueFileName(str);
      cueFileName.erase(cueFileName.find_last_of('.'));
      cueFileName += ".cue";
      string cueFileNameNoDir = fl_filename_name(cueFileName.c_str());

      string imageName;

			// if you find a .cue file, decompress it to get the file name to search
			// for in the archive.
      if (urarlib_get(&cueBuffer, &cueLen, (char*)cueFileNameNoDir.c_str(), (char*)str.c_str(), NULL))
      {
         ofstream out(cueFileName.c_str());
         out.write(cueBuffer, cueLen);
         out.close();
         CueParser cp(cueFileName);
         cp.parse();
         free(cueBuffer);
         imageName = cp.getCDName();
         imageName = fl_filename_name(imageName.c_str());
      }
			// try to find a cue file in the directory where the .rar file is
      else
      {
         unsigned long rarsize;
         urarlib_list((char*)str.c_str(),  (ArchiveList_struct*)&list);
         if (list == NULL)
         {
            moobyMessage("This is an invalid rar file");
            exit(0);
         }
         imageName = string((char*)list->item.Name);
         rarsize = list->item.UnpSize;
         itr = list;
         while(list != NULL)
         {
            itr = list->next;
            free(list->item.Name);
            free(list);
            list = itr;
         }
      }
			// uncompress the image to memory
      if (!urarlib_get(&theFile, &cueLen, (char*)imageName.c_str(), (char*)str.c_str(),NULL))
      {
         moobyMessage("couldnt extract image from rar file");
         exit(0);
      }

			// calculate the cd length and set the buffer info
      CDTime cdlen(cueLen, CDTime::abByte);
   
      fileBuffer = theFile;
      bufferPointer = theFile;
      bufferPos = CDTime(0,2,0);
      bufferEnd = cdlen + CDTime(0,2,0);
      CDLength = bufferEnd;

      setUncompressed(cueLen);
   }
		// if the file is already decompressed, just set the important variables
   else
   {
      CDTime cdlen(length, CDTime::abByte);
      fileBuffer = theFile;
      bufferPointer = theFile;
      bufferPos = CDTime(0,2,0);
      bufferEnd = cdlen;
      CDLength = bufferEnd;
   }
}
#endif

// reads data into the cache for UncompressedFileInterface
void UncompressedFileInterface::seekUnbuffered(const CDTime& cdt)
   throw(std::exception, Exception)
{
   CDTime seekTime(cdt - CDTime(0,2,0));
   isoFile_seekFile(isoFile,seekTime.getAbsoluteByte(),FILE_BROWSER_SEEK_SET);
   isoFile_readFile(isoFile,(char*)fileBuffer,bufferFrames * bytesPerFrame);
   bufferPointer = fileBuffer;
   bufferPos = cdt;
   bufferEnd = cdt + CDTime(bufferFrames, CDTime::abFrame);
}

