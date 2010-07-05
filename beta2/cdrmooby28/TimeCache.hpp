#ifndef TIMECACHE_HPP
#define TIMECACHE_HPP

#include "CDTime.hpp"
#include <map>
#include <list>

/*
 * This is an LRU cache for any type of file data this plugin can store.
 * It's a map of time to data...
 */

template <class Data>
class TimeCache
{
public:
   TimeCache() : maxSize(10) {}

   TimeCache(size_t size) { setMaxSize(size); }

   TimeCache& setMaxSize(size_t size)
   { 
      maxSize = size; 
      if (maxSize < 1)
         maxSize = 1;
      while (cache.size() > maxSize)
         popOne(); 
      return *this; 
   }

   size_t getMaxSize() const
   { return maxSize; }

      // return true if found (and set Data d)
   bool find (const CDTime& time, Data& d)
   {
         // try to find the data in the cache first
         // if it's there, return it and set it as the most recent
      typename CacheMap::iterator itr = cache.find(time);
      if (itr != cache.end())
      {
         d = (*itr).second.first;
            // splice will move the item in the LRUList to the front and
            // keep the iterator valid
         order.splice(order.begin(), order, 
            (*itr).second.second, (*itr).second.second); 
         return true;
      }
      else
      {
         return false;
      }
   }

      // insert this as the LRU data
   void insert(const CDTime& time, const Data& d)
   {
         // are we full?  push out the oldest data
      while (cache.size() >= maxSize)
         popOne();

      order.push_front(time);
      cache[time] = CacheItem(d, order.begin());
   }

private:
      // remove the oldest item from the cache
   void popOne()
   {
      CDTime toast = order.back();
      order.pop_back();
      cache.erase(cache.find(toast));
   }

   // maxSizeimum size of the cache
   size_t maxSize;

      // the list is ordered from first to last starting with
      // the most recently used data item.
   typedef std::list<CDTime> LRUList;
   LRUList order;

      // the map simply holds the time and data pairs
   typedef std::pair<Data, LRUList::iterator> CacheItem;
   typedef std::map<CDTime, CacheItem > CacheMap;
   CacheMap cache;
};

#endif // TIMECACHE_HPP
