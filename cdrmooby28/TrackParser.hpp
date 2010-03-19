/************************************************************************

Copyright mooby 2002

CDRMooby2 TrackParser.hpp
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/


#ifndef TRACKPARSER_HPP
#define TRACKPARSER_HPP

#include <string>
#include <fstream>
#include <vector>

#include "TrackInfo.hpp"
#include "FileInterface.hpp"

// TrackParser is the base class for reading CUE and CCD-type time sheets.
// It also sets up the list of tracks and times
class TrackParser
{
public:
      // attempts to open the cue file 'filename'
   TrackParser(const std::string& filename);

   virtual ~TrackParser() {}
      // parses the cue file, if any.  throws when there's a read error
   virtual void parse() throw(Exception) = 0;

      // this needs the total CD length to correctly determine
      // the start times and length of each track
   void postProcess(const CDTime& CDLength);

      // accessors
      // returns the track listing
      // track[0] is the total length
      // track[1...n] is the length of that track
   inline std::vector<TrackInfo> getTrackList() const {return tiv;}
      // returns the name of the CD as determined by the cue sheet (if any)
   inline std::string getCDName() const {return cuefilename;}

   friend std::ostream& operator<<(std::ostream& o, const TrackParser& cp);

   CDTime getPregapLength() const { return pregapLength; }

private:
   TrackParser(const TrackParser& r);
   TrackParser& operator=(const TrackParser& r);

protected:
		// the file itself
   std::ifstream theCueSheet;
		// the name of the .cue or .ccd file
   std::string cuename;
		// the name if the image file given in the cue sheet 
   std::string cuefilename;
		// track info vector with start and end times and such...
   std::vector<TrackInfo> tiv;
      // length of the pregap (if any)
   CDTime pregapLength;
};

// this is for no track list
class NullParser : public TrackParser
{
public:
   NullParser(const std::string& filename = std::string());

   virtual ~NullParser() {}

   virtual void parse() throw(Exception) {}
};

// this parses CDRWin CUE files
class CueParser : public TrackParser
{
public:
   CueParser(const std::string& filename)
      : TrackParser(filename)
   {}

   virtual ~CueParser() {}

	   // this is the only real function here - parses the cue sheet into
		// the internal data members
   virtual void parse() throw(Exception);
      
      // returns a string with a CUE file name if that file exists
   static std::string fileExists(const std::string& file);
};

// this parses CloneCD CCD files
class CCDParser : public TrackParser
{
public:
   CCDParser(const std::string& filename)
      : TrackParser(filename)
   {}

   virtual ~CCDParser() {}

	   // this is the only real function here - parses the cue sheet into
		// the internal data members
   virtual void parse() throw(Exception);

      // returns a string with a CCD file name if that file exists
   static std::string fileExists(const std::string& file);
};

// factory method for building track parser
TrackParser* TrackParserFactory(const std::string& filename, const FileInterface* fi);

#endif
