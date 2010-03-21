/************************************************************************

Copyright mooby 2002

CDRMooby2 FileInterface.hpp
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/


#ifndef FILEINTERFACE_HPP
#define FILEINTERFACE_HPP

#include "CDTime.hpp"
#include "TimeCache.hpp"
#include "Frame.hpp"

#include <vector>
#include <fstream>

// the constants for the number of frames in compressed files as well as
// how large the required buffers are
const unsigned long BZIndexBufferFrames = 10;
const unsigned long BZIndexBufferBytes = BZIndexBufferFrames * bytesPerFrame;
const unsigned long ZTableBufferFrames = 1;
const unsigned long ZTableBufferBytes = ZTableBufferFrames * bytesPerFrame;
const unsigned long UncompressedBufferFrames = 10;
const unsigned long UncompressedBufferBytes = UncompressedBufferFrames * bytesPerFrame;

// a virtual interface to any CD image type
class FileInterface
{
public:
      // bufferFrames is the number of frames you want
      // available at any time.
   virtual ~FileInterface() 
   { 
      if (bufferFrames != 0) 
      {
      if (fileBuffer) 
         delete[] fileBuffer;
      } 
   }

		// opens the file
   virtual void openFile(const std::string& str)
      throw(Exception);

		// seeks to the time cdt in the file and sets the buffer pointer
   inline void seek(const CDTime& cdt)
      throw(Exception);

		// returns the pointer to the last frame seeked.
   inline unsigned char* getBuffer() const 
   {
      if (cacheMode == oldMode)
         return bufferPointer;
      else if (cacheMode == newMode)
         return *holdout;
   }

		// returns the length of the CD
   inline CDTime getCDLength() {return CDLength;}

      // sets up stuffs for pregap
   FileInterface& setPregap(const CDTime& gapLength, const CDTime& gapTime);

      // returns the last seeked time
   inline CDTime getSeekTime() {return seekTime;}

      // returns the name of the file
   inline std::string getFileName() const {return fileName;}

   enum CacheMode
   {
      oldMode,
      newMode
   };

   inline FileInterface& setCacheMode(const CacheMode m)
   { cacheMode = m; return *this; }

protected:
		// when seek(CDTime) seeks to a frame out of the buffer, this
		// function buffers more data and sets the bufferPointer to
      // the requested time
   virtual void seekUnbuffered(const CDTime& cdt)
      throw(std::exception, Exception) = 0;

		// the number of frames buffered (0 for RAR)
   unsigned long bufferFrames;
		// the buffer to the cached data
   unsigned char* fileBuffer;
		// pointer to the last seeked frame
   unsigned char* bufferPointer;
		// the length of the CD
   CDTime CDLength;
		// the start position of the file buffer
   CDTime bufferPos;
		// the end position of the file buffer
   CDTime bufferEnd;
      // the last seeked time of this CD
   CDTime seekTime;
		// the name of the cd image
   std::string fileName;
	  // time of the pregap
   CDTime pregapTime;
      // length of the pregap
   CDTime pregapLength;

   CacheMode cacheMode;

      // the extra cache =)
   TimeCache<Frame> cache;
   Frame holdout;

protected:
		// this ensures that the buffer is at least as big as it's required (i.e. .BZ files
		// need at least 10 frames buffered).
		// for RAR, requiredFrames = 0
   FileInterface(const unsigned long requestedFrames, 
      const unsigned long requiredFrames);
};

#ifdef WINDOWS
// virtual interface to compressed files with index files (like .bz.index or .Z.table)
class CompressedFileInterface : public FileInterface
{
public:
   CompressedFileInterface(const unsigned long bf, const unsigned long pf)
      : FileInterface(bf, pf), compressedFrames(pf)
   { 
      compressedDataBuffer = 
         new unsigned char[(pf + 1) * bytesPerFrame];
   }

   virtual ~CompressedFileInterface() 
      {delete [] compressedDataBuffer;}

   virtual unsigned char* getCompressedBuffer() {return compressedDataBuffer;}

   virtual void compressData(char* uncompressedData,
                             char* compressedData,
                             unsigned int inputLen,
                             unsigned int& outputLen)
      throw(Exception) = 0;
   virtual void decompressData(char* uncompressedData,
                               char* compressedData,
                               unsigned int inputLen,
                               unsigned int& outputLen)
      throw(Exception) = 0;

   virtual std::string toTable(const std::vector<unsigned long>& table,
                               const std::vector<unsigned long>& sizes) = 0;

   unsigned long getCompressedFrames() {return compressedFrames;}

protected:
   virtual void seekUnbuffered(const CDTime& cdt)
      throw(std::exception, Exception);
   
   unsigned char* compressedDataBuffer;
   std::vector<unsigned long> lookupTable;
   unsigned long compressedFrames;

private:
   CompressedFileInterface();
};

// interface to the ZTable type files
class ZTableFileInterface : public CompressedFileInterface
{
public:
   ZTableFileInterface(const unsigned long bf)
      : CompressedFileInterface(bf, ZTableBufferFrames) {}
   virtual ~ZTableFileInterface() {}
   virtual void openFile(const std::string& str)
      throw(Exception);
   
   virtual void compressData(char* uncompressedData,
                             char* compressedData,
                             unsigned int inputLen,
                             unsigned int& outputLen)
      throw(Exception);
   virtual void decompressData(char* uncompressedData,
                               char* compressedData,
                               unsigned int inputLen,
                               unsigned int& outputLen)
      throw(Exception);

   virtual std::string toTable(const std::vector<unsigned long>& table,
                               const std::vector<unsigned long>& sizes);
protected:

private:
   ZTableFileInterface();
};

// interface to the BZIndex type of files
class BZIndexFileInterface : public CompressedFileInterface
{
public:
   BZIndexFileInterface(const unsigned long bf)
      : CompressedFileInterface(bf, BZIndexBufferFrames) {}
   virtual ~BZIndexFileInterface() {}
   virtual void openFile(const std::string& str)
      throw(Exception);
   
   virtual void compressData(char* uncompressedData,
                             char* compressedData,
                             unsigned int inputLen,
                             unsigned int& outputLen)
      throw(Exception);

   virtual void decompressData(char* uncompressedData,
                               char* compressedData,
                               unsigned int inputLen,
                               unsigned int& outputLen)
      throw(Exception);

   virtual std::string toTable(const std::vector<unsigned long>& table,
                               const std::vector<unsigned long>& sizes);
protected:

private:
   BZIndexFileInterface();
};
#endif

// interface to uncompressed files
class UncompressedFileInterface : public FileInterface
{
public:
   UncompressedFileInterface(const unsigned long bf)
      : FileInterface(bf, UncompressedBufferFrames) 
   {}
   
   virtual ~UncompressedFileInterface() {}

protected:
   virtual void seekUnbuffered(const CDTime& cdt)
      throw(std::exception, Exception);

private:
   UncompressedFileInterface();
};

#ifdef WINDOWS
// interface to RAR files.  All the data members are static so it doesn't
// decompress the file twice for normal data and CDDA data
class RARFileInterface : public FileInterface
{
public:
   RARFileInterface(const unsigned long bf)
      : FileInterface(bf, 0)
   {
         // dont use the LRU cache (the data is already in memory)
      cacheMode = oldMode;
   }

   virtual void openFile(const std::string& str)
      throw(Exception);
   virtual ~RARFileInterface() {toastStaticVars(); fileBuffer = NULL;}
protected:
   
   virtual void seekUnbuffered(const CDTime& cdt)
      throw(std::exception, Exception) {}
private:
   static void toastStaticVars() 
      {alreadyUncompressed = false; free(theFile); theFile = NULL; length = 0;}
   static void setUncompressed(const unsigned long len) 
      {alreadyUncompressed = true; length = len;}


   RARFileInterface();
		// whether or not it's already decompressed
   static bool alreadyUncompressed;
		// the file data uncompressed in memory
   static unsigned char* theFile;
		// the length of the file
   static unsigned long length;
};
#endif

// i optimized this function so the CDTimes don't need to use the timeConvert() function
// with all the + operations.  If the data is buffered, it should return very quickly.
// This only ensures that one frame is available at bufferPointer, but that should be
// enough for most things...
inline void FileInterface::seek(const CDTime& cdt)
   throw(Exception)
{
	using namespace std;
   seekTime = cdt;
   if (seekTime >= pregapTime)
      seekTime -= pregapLength;


   if (seekTime < CDLength)
   {
      if (cacheMode == newMode)
      {
            // check in the global cache first
         if (cache.find(seekTime, holdout))
         {
            return;
         }
      }

         // see if its in the file cache.
         // in both of these cases, bufferPointer should point to the data
         // requested
      if ( (seekTime >= bufferPos) &&
         ( (seekTime.getAbsoluteFrame() + 1) <= (bufferEnd.getAbsoluteFrame()) ) )
      {
         bufferPointer =
            fileBuffer + (seekTime.getAbsoluteByte() - bufferPos.getAbsoluteByte());
      }
      else
      {
         seekUnbuffered(seekTime);
      }
         // insert the new data into the cache
      if (cacheMode == newMode)
      {
         holdout = bufferPointer;
         cache.insert(seekTime, holdout);
      }
   }
   else
   {
      Exception e("Seek past end of disc\r\n");
      THROW(e);
   }
}

  // the factory method for creating a FileInterface based only on the filename
FileInterface* FileInterfaceFactory(const std::string& filename,
                                    std::string& extension);

#endif
