#ifndef CONFIG_FUNCTIONS_HPP
#define CONFIG_FUNCTIONS_HPP
#ifdef WINDOWS
#include "ConfigCallbacks.hpp"

#include "Utils.hpp"

// based on source generated from FLUID.
// this is the window that pops up when you use the Configure option

class ConfigWindow
{
public:
   ConfigWindow() 
   {}

   void makeWindow();
   void show() { w->show(); }
   Fl_Window* w;
   Fl_Check_Button* repeatAllButton;
   Fl_Check_Button* repeatOneButton;
   Fl_Check_Button* playOneButton;
   Fl_Box* autorunBox;
};

class RunConfig
{
public:
   RunConfig() 
   {
      w.makeWindow();
      w.show(); 
      Fl::run(); 
#ifdef __LINUX__
      Fl::wait(); 
#endif
   }

private:
   ConfigWindow w;
};
#endif
#endif
