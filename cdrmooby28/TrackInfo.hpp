/************************************************************************

Copyright mooby 2002

CDRMooby2 TrackInfo.hpp
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/


#ifndef TRACKINFO_HPP
#define TRACKINFO_HPP

#include "CDTime.hpp"

#include <iomanip>

// This class stores all the information we need to know about one track.
// It's just a glorified struct.
class TrackInfo
{
public:
   TrackInfo() : trackNumber(0) {}

   TrackInfo(const CDTime& tl)
      : trackNumber(0), trackStart(0, CDTime::abFrame), trackLength(tl) {}

   inline friend std::ostream& operator<<(std::ostream& o, const TrackInfo& ti)
   {
      o << std::setw(2) << ti.trackNumber << ' ' << ti.trackStart.getMSF() 
        << ' ' << ti.trackEnd.getMSF() << ' ' << ti.trackLength.getMSF() << std::endl;
      return o;
   }

   u32 trackNumber;
   CDTime trackStart;
   CDTime trackEnd;
   CDTime trackLength;
};

#endif
