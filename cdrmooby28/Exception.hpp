/************************************************************************

Copyright mooby 2002

CDRMooby2 Exception.hpp
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/

#ifdef WINDOWS
#pragma warning(disable:4786)
#endif

#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP

#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#ifdef WINDOWS
#include <FL/fl_ask.H>
#endif

// the exception data for the plugin.  stores the line number too
// for easy debugging
class Exception
{
public:
   Exception() : line(0) {}

   Exception(const std::string& str)
      : line(0) {error.push_back(str);}

   inline void setLine(const u32 l) {line = l;}
   inline void setFile(const std::string& str) {file = str;}
   inline void addText(const std::string& str) {error.push_back(str);}
   inline std::string text() {std::ostringstream str; str << *this;  return str.str();}

   inline friend std::ostream& operator<<(std::ostream& o, const Exception& m);

private:
   u32 line;
   std::string file;
   std::vector<std::string> error;
};

inline std::ostream& operator<<(std::ostream& o, const Exception& m)
{
   std::vector<std::string>::size_type index;
   for(index = 0; index < m.error.size(); index++)
      o << m.error[index]<< std::endl;
   o << "On line: " << m.line << std::endl
     << "In file: " << m.file << std::endl;
   return o;
}

#define THROW(e) e.setLine(__LINE__); e.setFile(__FILE__); moobyMessage(e.text().c_str()); throw(e);

#endif
