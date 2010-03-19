#ifndef CONFIG_CALLBACKS_HPP
#define CONFIG_CALLBACKS_HPP

#ifdef WINDOWS
void bzCompress(Fl_Button*, void*);
void bzDecompress(Fl_Button*, void*);
void zCompress(Fl_Button*, void*);
void zDecompress(Fl_Button*, void*);
void CDDAVolume(Fl_Value_Slider*, void*);
void cacheSize(Fl_Value_Slider*, void*);
void newCaching(Fl_Check_Button*, void*);
void subEnable(Fl_Check_Button*, void*);
void repeatAllCDDA(Fl_Check_Button*, void* val = NULL);
void repeatOneCDDA(Fl_Check_Button*, void* val = NULL);
void playOneCDDA(Fl_Check_Button*, void* val = NULL);
void configOK(Fl_Return_Button*, void*);
void chooseAutorunImage(Fl_Button*, void*);
void clearAutorunImage(Fl_Button*, void*);
#endif

#endif
