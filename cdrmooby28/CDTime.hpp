/************************************************************************

Copyright mooby 2002

CDRMooby2 CDTime.hpp
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/



#ifndef CDTIME_HPP
#define CDTIME_HPP

#include "Utils.hpp"
#include "Exception.hpp"

#include <iomanip>
#include <iostream>
extern "C" {
  #include <gccore.h>
  #include <string.h>
  #include <stdint.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
}

// MSFTime is a container for minute, second, and frame
class MSFTime
{
public:
		// default 0:0:0
   MSFTime() : minute(0), sec(0), frame(0) {}

		// various constructors...
   MSFTime(const unsigned char m, const unsigned char s, const unsigned char f)
      : minute(m), sec(s), frame(f) {}

   MSFTime(const unsigned char* msf, const TDTNFormat format)
      : minute(0), sec(0), frame(0)
   {setMSF(msf, format);}

   MSFTime(const MSFTime& r) : minute(r.minute), sec(r.sec), frame(r.frame) {}

   inline void setMSF(const unsigned char m, const unsigned char s, const unsigned char f)
   {minute = m; sec = s; frame = f;}

		// sets this MSF time based on format.  format tells the order of the
		// frame, min and sec in the char* and whether or not it's integer or BCD
   inline void setMSF(const unsigned char* msf, const TDTNFormat format)
   {
      if (format == msfbcd)
      {
         minute = BCDToInt(msf[0]);
         sec = BCDToInt(msf[1]);
         frame = BCDToInt(msf[2]);
      }
      else if (format == msfint)
      {
         minute = msf[0];
         sec = msf[1];
         frame = msf[2];
      }
      else if (format == fsmbcd)
      {
         minute = BCDToInt(msf[2]);
         sec = BCDToInt(msf[1]);
         frame = BCDToInt(msf[0]);
      }
      else if (format == fsmint)
      {
         minute = msf[2];
         sec = msf[1];
         frame = msf[0];
      }
   }
   
   inline unsigned char m() const
   {return minute;}

   inline unsigned char s() const
   {return sec;}

   inline unsigned char f() const
   {return frame;}

   inline friend std::ostream& operator<<(std::ostream& o, const MSFTime& m);
   inline friend std::istream& operator>>(std::istream& i, MSFTime& m);

      // Note that this does NOT return a reference
   inline unsigned char operator[](const size_t index) const
      throw(Exception)
   {
      switch(index)
      {
      case 0: return minute;
      case 1: return sec;
      case 2: return frame;
      default:
         {
            Exception e("Index out of range");
            THROW(e);
         }
      }
   }

private:
   unsigned char minute;
   unsigned char sec;
   unsigned char frame;
};

// CDTime contains all the data conversions, operations, and types needed for 
// calculating any type of time used in this plugin
class CDTime
{
public:
		// CDTime holds the time in 3 ways - msf, absoluteByte, and absoluteFrame.
		// This enum tells what conversions have been done on the data.
	enum conversionType
	{
		msf      = 0x01,
		abByte   = 0x02,
		abFrame  = 0x04,
      allTypes = 0x07
	};

	CDTime()
		: conversions(0), absoluteByte(0), absoluteFrame(0)
	{}

   CDTime(const CDTime& r)
      : conversions(r.conversions), MSF(r.MSF), absoluteByte(r.absoluteByte), absoluteFrame(r.absoluteFrame)
   {}

	CDTime(const unsigned char m, const unsigned char s, const unsigned char f)
		: conversions(msf), MSF(m, s, f), absoluteByte(0), absoluteFrame(0)
   {convertTime();}

	CDTime(const unsigned char* msftime, const TDTNFormat format)
		: conversions(msf), MSF(msftime, format), absoluteByte(0), absoluteFrame(0)
   {
      convertTime();
   }

   CDTime(const MSFTime& msftime)
		: conversions(msf), MSF(msftime), absoluteByte(0), absoluteFrame(0)
   {convertTime();}

	// converts a time like 00:00:00 to a CD time
   CDTime(const std::string& str)
      : conversions(msf), absoluteByte(0), absoluteFrame(0)
   {
      MSF.setMSF(atoi(str.substr(0,2).c_str()), atoi(str.substr(3, 2).c_str()), 
         atoi(str.substr(6, 2).c_str()));
      convertTime();
   }

	  // constructor for absoluteByte and absoluteFrame
   CDTime(const u32 absoluteAddr, const conversionType ct);

	// math and comparison operators
   inline bool operator<(const CDTime& r) const;
   inline bool operator>(const CDTime& r) const;
   inline bool operator==(const CDTime& r) const;
   inline bool operator!=(const CDTime& r) const;
   inline bool operator<=(const CDTime& r) const;
   inline bool operator>=(const CDTime& r) const;

   // using the default operator=
   inline CDTime& operator+=(const CDTime& r);
   inline CDTime& operator-=(const CDTime& r);
   inline CDTime operator+(const CDTime& r) const;
   inline CDTime operator-(const CDTime& r) const;

	// access and set operators
   inline MSFTime getMSF() const;
   inline CDTime& setMSF(const MSFTime& r);

	// this one is special - it converts the MSFTime into the TDTNFormat format
	// and stores it in an internal buffer, then returns a pointer to that buffer.
	// This is really useful for GetTD.
   inline unsigned char* getMSFbuf(const TDTNFormat format = msfint) const;

   inline u32 getAbsoluteByte() const;
   inline CDTime& setAbsoluteByte(const u32 ab);

   inline u32 getAbsoluteFrame() const;
   inline CDTime& setAbsoluteFrame(const u32 af);
   
   inline friend std::ostream& operator<<(std::ostream& o, const CDTime& cdt);
   inline friend std::istream& operator>>(std::istream& i, CDTime& m);

private:
		// based on what conversions have already been done on the data, convertTime
		// sets whatever data members have not been set yet.
   inline void convertTime()
      throw(Exception);

      // Data members

		// this holds what conversions that have already been done on the data (with bitmasks)
	unsigned char conversions;

		// the time data
   MSFTime MSF;
	u32 absoluteByte;
	u32 absoluteFrame;
   // used for converting time to unsigned char[3]
   unsigned char MSFbuf[3];
};

inline std::ostream& operator<<(std::ostream& o, const MSFTime& m)
{
   o << std::setfill('0') << std::setw(2) << (int)m.m() 
      << ':' << std::setw(2) << (int)m.s() 
      << ':' << std::setw(2) << (int)m.f();
   return o;
}

inline std::istream& operator>>(std::istream& i, MSFTime& m)
{
   std::ios::iostate state = i.exceptions();
   i.exceptions(std::ios::failbit|std::ios::badbit|std::ios::eofbit);
   try
   {
      char c;
      int minute, sec, frame;
      i >> minute >> c >> sec >> c >> frame;
      m.minute = minute;
      m.sec = sec;
      m.frame = frame;
   }
   catch(...)
   {
      i.setstate(std::ios::failbit);
   }
   i.exceptions(state);
   return i;
}

inline CDTime::CDTime(const u32 absoluteAddr, const conversionType ct)
   : conversions(0), absoluteByte(0), absoluteFrame(0)
{
   if (ct == abByte)
   {
      conversions = ct;
      absoluteByte = absoluteAddr;
      convertTime();
   }
   else if (ct == abFrame)
   {
      conversions = ct;
      absoluteFrame = absoluteAddr;
      convertTime();
   }
}

inline bool CDTime::operator<(const CDTime& r) const
{
   return absoluteByte < r.absoluteByte;
}

inline bool CDTime::operator>(const CDTime& r) const
{
   return absoluteByte > r.absoluteByte;
}

inline bool CDTime::operator==(const CDTime& r) const
{
   return absoluteByte == r.absoluteByte;
}

inline bool CDTime::operator!=(const CDTime& r) const
{
   return absoluteByte != r.absoluteByte;
}

inline bool CDTime::operator<=(const CDTime& r) const
{
   return absoluteByte <= r.absoluteByte;
}

inline bool CDTime::operator>=(const CDTime& r) const
{
   return absoluteByte >= r.absoluteByte;
}

inline CDTime& CDTime::operator+=(const CDTime& r)
{
   absoluteByte += r.absoluteByte;
   conversions = abByte;
   convertTime();
   return *this;
}

inline CDTime& CDTime::operator-=(const CDTime& r)
{
   absoluteByte -= r.absoluteByte;
   conversions = abByte;
   convertTime();
   return *this;
}

inline CDTime CDTime::operator+(const CDTime& r) const
{
   CDTime temp(*this);
   return (temp += r);
}

inline CDTime CDTime::operator-(const CDTime& r) const
{
   CDTime temp(*this);
   return (temp -= r);
}

inline MSFTime CDTime::getMSF() const
{
   return MSF;
}

inline CDTime& CDTime::setMSF(const MSFTime& r)
{
   MSF = r;
   conversions = msf;
   convertTime();
   return *this;
}

inline unsigned char* CDTime::getMSFbuf(const TDTNFormat format) const
{
   CDTime& thisCDTime = const_cast<CDTime&>(*this);
   if (format == msfbcd)
   {
      thisCDTime.MSFbuf[0] = intToBCD(MSF.m());
      thisCDTime.MSFbuf[1] = intToBCD(MSF.s());
      thisCDTime.MSFbuf[2] = intToBCD(MSF.f());
   }
   else if (format == fsmbcd)
   {
      thisCDTime.MSFbuf[0] = intToBCD(MSF.f());
      thisCDTime.MSFbuf[1] = intToBCD(MSF.s());
      thisCDTime.MSFbuf[2] = intToBCD(MSF.m());
   }
   else if (format == fsmint)
   {
      thisCDTime.MSFbuf[0] = MSF.f();
      thisCDTime.MSFbuf[1] = MSF.s();
      thisCDTime.MSFbuf[2] = MSF.m();
   }
   else if (format == msfint)
   {
      thisCDTime.MSFbuf[0] = MSF.m();
      thisCDTime.MSFbuf[1] = MSF.s();
      thisCDTime.MSFbuf[2] = MSF.f();
   }
   return (unsigned char*)&MSFbuf;
}


inline u32 CDTime::getAbsoluteByte() const
{
   return absoluteByte;
}

inline CDTime& CDTime::setAbsoluteByte(const u32 ab)
{
   absoluteByte = ab;
   conversions = abByte;
   convertTime();
   return *this;
}

inline u32 CDTime::getAbsoluteFrame() const
{
   return absoluteFrame;
}

inline CDTime& CDTime::setAbsoluteFrame(const u32 af)
{
   absoluteFrame = af;
   conversions = abFrame;
   convertTime();
   return *this;
}


// set all the times for the conversions that have not been done yet.
// at the end of the function, all the conversions should be done.
inline void CDTime::convertTime()
   throw(Exception)
{
      // if no times are set, throw an error
   if (conversions == 0)
   {
      Exception e("Cannot perform time conversion");
      THROW(e);
   }
   if (conversions & msf)
   {
      if (! (conversions & abByte))
      {
         absoluteByte = MSF.m() * bytesPerMinute + 
            MSF.s() * bytesPerSecond + 
            MSF.f() * bytesPerFrame;
      }
      if (! (conversions & abFrame))
      {
         absoluteFrame = MSF.m() * framesPerMinute +
            MSF.s() * framesPerSecond +
            MSF.f();
      }
   }
   else if (conversions & abByte)
   {
      if (! (conversions & msf))
      {
         unsigned char m = absoluteByte / bytesPerMinute;
         unsigned char s = (absoluteByte - m * bytesPerMinute) / bytesPerSecond;
         unsigned char f = (absoluteByte - m * bytesPerMinute - s * bytesPerSecond) /
            bytesPerFrame;
         MSF.setMSF(m, s, f);
      }
      if (! (conversions & abFrame))
      {
         absoluteFrame = absoluteByte / bytesPerFrame;
      }
   }
   else if (conversions & abFrame)
   {
      if (! (conversions & msf))
      {
         unsigned char m = absoluteFrame / framesPerMinute;
         unsigned char s = (absoluteFrame - m * framesPerMinute) / framesPerSecond;
         unsigned char f = absoluteFrame - m * framesPerMinute -  s * framesPerSecond;
         MSF.setMSF(m, s, f);
      }
      if (! (conversions & abByte))
      {
         absoluteByte = absoluteFrame * bytesPerFrame;
      }
   }
   else
   {
      Exception e("Unknown conversion type");
      THROW(e);
   }
   conversions |= allTypes;
}

inline std::ostream& operator<<(std::ostream& o, const CDTime& cdt)
{
   if (cdt.conversions == 0)
   {
      o << "Time not set" << std::endl;
      return o;
   }
   if (cdt.conversions & CDTime::msf)
   {
      o << "MSF: " << cdt.MSF << std::endl;
   }
   if (cdt.conversions & CDTime::abByte)
   {
      o << "Absolute byte: " << cdt.absoluteByte << std::endl;
   }
   if (cdt.conversions & CDTime::abFrame)
   {
      o << "Absolute frame: " << cdt.absoluteFrame << std::endl;
   }
   return o;
}


inline std::istream& operator>>(std::istream& i, CDTime& cdt)
{
   i >> cdt.MSF;
   cdt.convertTime();
}
#endif

