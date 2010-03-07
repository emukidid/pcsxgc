/**
 * Wii64 - fileBrowser-DVD.c
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * fileBrowser module for ISO9660 DVD Discs
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address:  emukidid@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/


#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/dvd.h>
#include <ogc/machine/processor.h>
#include "fileBrowser.h"
#include "../gc_dvd.h"
#ifdef HW_RVL
#include <di/di.h>
#endif

/* DVD Globals */
int dvd_init = 0;

fileBrowser_file topLevel_DVD =
	{ "\\", // file name
	  0ULL,      // discoffset (u64)
	  0,         // offset
	  0,         // size
	  FILE_BROWSER_ATTR_DIR
	};
 
int fileBrowser_DVD_readDir(fileBrowser_file* ffile, fileBrowser_file** dir){	
  
  int num_entries = 0, ret = 0;
  
  if(dvd_get_error() || !dvd_init) { //if some error
    ret = init_dvd();
    if(ret) {    //try init
      return ret; //fail
    }
    dvd_init = 1;
  } 
		
	// Call the corresponding DVD function
	num_entries = dvd_read_directoryentries(ffile->discoffset,ffile->size);
	
	// If it was not successful, just return the error
	if(num_entries <= 0) return FILE_BROWSER_ERROR;
	
	// Convert the DVD "file" data to fileBrowser_files
	*dir = malloc( num_entries * sizeof(fileBrowser_file) );
	int i;
	for(i=0; i<num_entries; ++i){
		strcpy( (*dir)[i].name, &DVDToc.file[i].name[0] );
		(*dir)[i].discoffset = (uint64_t)(((uint64_t)DVDToc.file[i].sector)*2048);
		(*dir)[i].offset = 0;
		(*dir)[i].size   = DVDToc.file[i].size;
		(*dir)[i].attr	 = 0;
		if(DVDToc.file[i].flags == 2)//on DVD, 2 is a dir
			(*dir)[i].attr   = FILE_BROWSER_ATTR_DIR; 
		if((*dir)[i].name[strlen((*dir)[i].name)-1] == '/' )
			(*dir)[i].name[strlen((*dir)[i].name)-1] = 0;	//get rid of trailing '/'
	}
		
	if(strlen((*dir)[0].name) == 0)
		strcpy( (*dir)[0].name, ".." );
	
	return num_entries;
}

int fileBrowser_DVD_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	if(type == FILE_BROWSER_SEEK_SET) file->offset = where;
	else if(type == FILE_BROWSER_SEEK_CUR) file->offset += where;
	else file->offset = file->size + where;
	return 0;
}

int fileBrowser_DVD_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	int bytesread = read_safe(buffer,file->discoffset+file->offset,length);
	if(bytesread > 0)
		file->offset += bytesread;
	return bytesread;
}

int fileBrowser_DVD_init(fileBrowser_file* file){
	return 0;
}

int fileBrowser_DVD_deinit(fileBrowser_file* file) {
	return 0;
}

