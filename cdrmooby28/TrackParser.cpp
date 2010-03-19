/************************************************************************

Copyright mooby 2002

CDRMooby2 TrackParser.cpp
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/

#pragma warning(disable:4786)

#include "TrackParser.hpp"
#include "Utils.hpp"

using namespace std;


TrackParser* TrackParserFactory(const std::string& filename,
                                const FileInterface* fi)
{
      // try to open a track listing sheet
   std::string thisFile;
   if ( (thisFile = CCDParser::fileExists(filename)) != std::string())
      return new CCDParser(thisFile);
   else if ( (thisFile = CueParser::fileExists(filename)) != std::string())
      return new CueParser(thisFile);
   else
      return new NullParser(fi->getFileName());
}

   // just opens the file for parsing
TrackParser::TrackParser(const std::string& filename)
   : cuename(filename), pregapLength(CDTime(0,0,0))
{
   if (!filename.empty()) theCueSheet.open(filename.c_str());
}


   // post processing on the list
void TrackParser::postProcess(const CDTime& CDLength)
{
   vector<TrackInfo>::size_type index;

   CDTime thisCDLength(CDLength);

      // and if there's a pregap flag, add that time
   thisCDLength += pregapLength;

   // calculate the track lengths, except for the last track
   // which needs CDLength
   // The track length is the start time of the next track - the start time of this track -
   //   1 frame
   if (tiv.size() > 0)
   {
      for(index = 0; index < tiv.size() - 1; index++)
      {
         tiv[index].trackLength = tiv[index + 1].trackStart - 
            tiv[index].trackStart;
      }
         // finally, at the end of the disc, there's a 2 second gap as well...
      tiv[index].trackLength = thisCDLength - CDTime(0,2,0) - tiv[index].trackStart;
   }
   // if there is no cue sheet, just make a single track with length CDLength.
   else
   {
      tiv.insert(tiv.begin(), TrackInfo(thisCDLength - CDTime(0,2,0)));
   }

	  // set the ending time
   for(index = 0; index < tiv.size(); index++)
   {
      // there's an extra 2 seconds not accounted for somewhere....
      tiv[index].trackStart += CDTime(0,2,0);
      // and the track end is 1 frame less...
      tiv[index].trackEnd    = tiv[index].trackStart + tiv[index].trackLength - CDTime(0,0,1);
   }

   // insert the total length at tiv[0]
   TrackInfo track0;
   track0.trackEnd = tiv[tiv.size()-1].trackEnd + CDTime(0,0,1);
   track0.trackStart = track0.trackEnd;
   track0.trackLength = track0.trackEnd;
   track0.trackNumber = 0;
   tiv.insert(tiv.begin(), track0);
}

std::ostream& operator<<(std::ostream& o, const TrackParser& cp)
{
   vector<TrackInfo>::size_type index;
   for (index = 0; index < cp.tiv.size(); index++)
   {
      o << cp.tiv[index] << endl;
   }
   return o;
}


NullParser::NullParser(const std::string& filename)
      : TrackParser(filename)
{}

// parses a CUE file
void CueParser::parse() throw(Exception)
{
   if (!theCueSheet)
   {
      // if there's a file error here, then there's either a file error
      // or there's no cue sheet.  in either case, we'll ignore it.
      // a cue sheet is nice, but not necessary
      return;
   }

   bool doneReading = false;

   theCueSheet.exceptions(ios::eofbit|ios::badbit|ios::failbit);
   string thisLine;

   try
   {
      TrackInfo thisTrack;

      getline(theCueSheet, thisLine);
      doneReading = true;
			// the file name is whatever is in the " marks
      std::string::size_type firstpos = thisLine.find('"');
      std::string::size_type lastpos = thisLine.rfind('"');
      cuefilename = thisLine.substr(firstpos + 1, lastpos - firstpos - 1);

      while(theCueSheet)
      {
         getline(theCueSheet, thisLine);
         string firstWord = word(thisLine, 1);
         if (firstWord == "TRACK")
         {
            thisTrack.trackNumber = atoi(word(thisLine,2).c_str());
            doneReading = false;
         }
         else if (firstWord == "PREGAP")
         {
            pregapLength = CDTime(word(thisLine,2));
         }
         else if (firstWord == "INDEX")
         {
            // we need INDEX 01
            if (atoi(word(thisLine,2).c_str()) == 1)
            {
               thisTrack.trackStart = CDTime(word(thisLine,3));
               thisTrack.trackStart += pregapLength;
               tiv.push_back(thisTrack);
               thisTrack = TrackInfo();
               doneReading = true;
            }
         }
         else
         {
            // whatever, we'll just skip this...
         }
      }
   }
   catch(std::exception& e)
   {
      if (!doneReading)
      {
         Exception exc(string("Error reading cue sheet ") + cuename);
         exc.addText(string(e.what()));
         THROW(exc);
      }
   }   
}

std::string CueParser::fileExists(const std::string& file)
{
   {
      std::ifstream is; 
      std::string cueName = file + std::string(".cue");
      is.open(cueName.c_str());
      if (is)
      {
         return cueName;
      }
   }
   return string();
}

// parses a CCD file
void CCDParser::parse() throw(Exception)
{
   if (!theCueSheet)
   {
      // if there's a file error here, then there's either a file error
      // or there's no cue sheet.  in either case, we'll ignore it.
      // a cue sheet is nice, but not necessary
      return;
   }

   bool doneReading = false;
   theCueSheet.exceptions(ios::eofbit|ios::badbit|ios::failbit);
   string thisLine;

   try
   {
      TrackInfo thisTrack;

      doneReading = false;

			// the file name is whatever the .ccd is changed to .img
      cuefilename = cuename.substr(0,cuename.rfind('.')) + std::string(".img");

      while(theCueSheet)
      {
         getline(theCueSheet, thisLine);
         string firstWord = word(thisLine, 1);
         if (firstWord == "[TRACK")
         {
            thisTrack.trackNumber = atoi(word(thisLine,2).c_str());
         }
         else if (firstWord == "INDEX")
         {
            // we need INDEX 01
            if (atoi(word(thisLine,2).c_str()) == 1)
            {
                  // the number after the = sign is the 
                  // absolute frame
               std::string frame(thisLine.substr(thisLine.find('=')+1));
               thisTrack.trackStart = CDTime(atoi(frame.c_str()), CDTime::abFrame);
               tiv.push_back(thisTrack);
               thisTrack = TrackInfo();
               doneReading = true;
            }
         }
         else
         {
            // whatever, we'll just skip this...
         }
      }
   }
   catch(std::exception& e)
   {
      if (!doneReading)
      {
         Exception exc(string("Error reading cue sheet ") + cuename);
         exc.addText(string(e.what()));
         THROW(exc);
      }
   }   
}

std::string CCDParser::fileExists(const std::string& file)
{
   {
      std::ifstream is; 
      std::string ccdName = file + std::string(".ccd");
      is.open(ccdName.c_str());
      if (is)
      {
         return ccdName;
      }
   }
   {
      std::ifstream is; 
      std::string ccdName = file + std::string(".CCD");
      is.open(ccdName.c_str());
      if (is)
      {
         return ccdName;
      }
   }
   return std::string();
}
