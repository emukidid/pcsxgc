/************************************************************************

Copyright mooby 2002

CDRMooby2 SubchannelData.hpp
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/


#ifndef SUBCHANNELDATA_HPP
#define SUBCHANNELDATA_HPP

#include "CDTime.hpp"
#include "Exception.hpp"
#include "Utils.hpp"
#include "TimeCache.hpp"

#include <fstream>
#include <map>

// so the funny thing is the subchannel is actually 98 bytes long, but
// the first two are sync/identifier blocks, so 96 is ok for this.
// that and CloneCD Sub files don't write those two blocks... 
const int SubchannelFrameSize = 96;


// one subchannel frame holds 96 bytes of subchannel data
class SubchannelFrame
{
public:
   SubchannelFrame() 
   {
      subData = new unsigned char[SubchannelFrameSize];
      memset(subData, 0, SubchannelFrameSize);
   }

   SubchannelFrame(const CDTime& time)
   {
      subData = new unsigned char[SubchannelFrameSize];
      memset(subData, 0, SubchannelFrameSize);
      setTime(time);
   }

   SubchannelFrame(const SubchannelFrame& r) 
   {
      subData = new unsigned char[SubchannelFrameSize];
      memcpy(subData, r.subData, SubchannelFrameSize);
   }

   ~SubchannelFrame() { if (subData) delete [] subData;}

   SubchannelFrame& operator=(const SubchannelFrame& r)
   {
      memcpy(subData, r.subData, SubchannelFrameSize); 
      return *this;
   }

   inline void clear() {memset(subData, 0, SubchannelFrameSize);}


   // in case it's not clear, here i'm faking the Q subchannel data.
   // i'll explain...   All integers should be in BCD format
   inline SubchannelFrame& setTime(const CDTime& time)
   {
      CDTime localTime(time - CDTime(0,2,0));

         // the control and Q mode bits.  this subchannel frame
         // is mode 1
      subData[12] = 0x41;
         // track number 
      subData[13] = 0x01;
         // index... 0 is reserved (pause) so 1 is ok
      subData[14] = 0x01;
         // track relative address - time of this sector relative to the track start
      memcpy(subData + 15, (localTime).getMSFbuf(msfbcd), 3);
         // zero, cause that's what it is.
      subData[18] = 0x00;
         // absolute frame address, from the start of the disc
      memcpy(subData + 19, (time).getMSFbuf(msfbcd), 3);
      return *this;
   }

   inline unsigned char* getBuffer() const {return subData;}

   unsigned char* subData;
};


// virtual base class for storing subchannel data
class SubchannelData
{
public:
   SubchannelData()  {}
   virtual void openFile(const std::string& file) 
      throw(Exception) = 0;
   virtual void seek(const CDTime& cdt) 
      throw(Exception) = 0;
   inline unsigned char* get(void) const {return sf.subData;}
   virtual ~SubchannelData() {}
protected:
   SubchannelFrame sf;
};

// disables the subchannel data outright
class DisabledSubchannelData : public SubchannelData
{
public:
   DisabledSubchannelData() 
   {
      delete [] sf.subData;
      sf.subData = NULL;
   }
   virtual void openFile(const std::string& file) 
      throw(Exception) {}
   virtual void seek(const CDTime& cdt) throw(Exception)
      {}
   virtual ~DisabledSubchannelData() {}
};

// makes up data if there is no subchannel data file
class NoSubchannelData : public SubchannelData
{
public:
   NoSubchannelData() {}
   virtual void openFile(const std::string& file) 
      throw(Exception) {}
   virtual void seek(const CDTime& cdt) throw(Exception)
      {sf.setTime(cdt);}
   virtual ~NoSubchannelData() {}
};

// reads a CloneCD SUB file for the subchannel data.
// the .SUB file stores all subchannel data for a disc,
// each block is the last 96 bytes of the full 98 byte data.
class SUBSubchannelData : public SubchannelData
{
public:
   SUBSubchannelData();
   virtual void openFile(const std::string& file) 
      throw(Exception);
   virtual void seek(const CDTime& cdt)
      throw(Exception);
   virtual ~SUBSubchannelData() {}
private:
   std::ifstream subFile;

         // the extra cache =)
   TimeCache<SubchannelFrame> cache;
   bool enableCache;
};

/* 
 reads an SBI file for subchannel data, caches that info
 internally and makes up any data not in that cache.
 
   the first 4 characters of the file are "SBI" and a null character '\0'
 
   each record is like this:
      3 bytes: the data in MSF/BCD format
      1 byte: a switch
          1 - all 10 Q channel subframe data starting
              at subframe[12]
          2 - 3 bytes of the track relative address
              at subframe[15]
          3 - 3 bytes of the track absolute address, 
               at subframe[19]
*/
class SBISubchannelData : public SubchannelData
{
public:
   SBISubchannelData() {}
   virtual void openFile(const std::string& file) 
      throw(Exception);
   virtual void seek(const CDTime& cdt)
      throw(Exception);
   virtual ~SBISubchannelData() {}
private:
   std::map<CDTime, SubchannelFrame> subMap;
};

/*
 reads an M3S file for subchannel data, caches that info
 internally and makes up any data not in that cache.

  M3s stores all the subchannel data for minute 3 of the disc
  in 16 byte blocks.  You only need the first 10 of the 
  16 bytes, read into subframe[12] to be OK though. once you
  hit minute 4, there's no more data.
*/
class M3SSubchannelData : public SubchannelData
{
public:
   M3SSubchannelData() {}
   virtual void openFile(const std::string& file) 
      throw(Exception);
   virtual void seek(const CDTime& cdt)
      throw(Exception);
   virtual ~M3SSubchannelData() {}
private:
   std::map<CDTime, SubchannelFrame> subMap;
};

// determines what kind of subchannel data is available based only
// on file names
SubchannelData* SubchannelDataFactory(const std::string& fileroot);

#endif
