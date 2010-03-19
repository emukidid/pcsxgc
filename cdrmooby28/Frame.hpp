#ifndef FRAME_HPP
#define FRAME_HPP

#include "Utils.hpp"

// one frame of CD data, 2352 bytes
class Frame
{
public:
      // note that the data is uninitialized w/default constructor
   Frame()
   {
      data = new unsigned char[bytesPerFrame];
   }

   Frame(const unsigned char* d)
   {
      data = new unsigned char[bytesPerFrame];
      memcpy(data, d, bytesPerFrame);
   }

   Frame(const Frame& r)
   {
      data = new unsigned char[bytesPerFrame];
      memcpy(data, r.data, bytesPerFrame);
   }

   ~Frame() { delete [] data; }

   Frame& operator=(const Frame& r)
   {
      memcpy(data, r.data, bytesPerFrame);
      return *this;
   }

   Frame& operator=(const unsigned char* buf)
   {
      memcpy(data, buf, bytesPerFrame);
      return *this;
   }

   Frame& operator=(const char* buf)
   {
      memcpy(data, buf, bytesPerFrame);
      return *this;
   }
   
   int operator==(const Frame& r)
   {
      return memcmp(data, r.data, bytesPerFrame);
   }

   unsigned char* operator*() const
   {
      return data;
   }

private:
   unsigned char* data;
};



#endif
