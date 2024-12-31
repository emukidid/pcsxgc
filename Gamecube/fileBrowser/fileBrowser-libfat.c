/**
 * WiiSX - fileBrowser-libfat.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 *
 * fileBrowser for any devices using libfat
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *                emukidid@gmail.com
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


#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/dir.h>
#include <dirent.h>
#include "fileBrowser.h"
#include <sdcard/gcsd.h>
#include <iso9660.h>
#include <di/di.h>
#include <ogc/dvd.h>
#include "m2loader.h"


#ifdef HW_RVL
#include <sdcard/wiisd_io.h>
#include <ogc/usbstorage.h>
static DISC_INTERFACE* frontsd = &__io_wiisd;
static DISC_INTERFACE* usb = &__io_usbstorage;
static DISC_INTERFACE* dvd = &__io_wiidvd;
static DISC_INTERFACE* carda = &__io_gcsda;
static DISC_INTERFACE* cardb = &__io_gcsdb;
#else
static DISC_INTERFACE* dvd = &__io_gcdvd;
static DISC_INTERFACE* carda = &__io_gcsda;
static DISC_INTERFACE* cardb = &__io_gcsdb;
static DISC_INTERFACE* sd2sp2 = &__io_gcsd2;
static DISC_INTERFACE* gcloader = &__io_gcode;
static DISC_INTERFACE* m2loader = &__io_m2ldr;
#endif

fileBrowser_file topLevel_libfat_Default =
	{ "sd:/wiisx/isos", // file name
	  0, // sector
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };

fileBrowser_file topLevel_libfat_USB =
	{ "usb:/wiisx/isos", // file name
	  0, // sector
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };

fileBrowser_file topLevel_DVD =
	{ "dvd:/", // file name
	  0, // sector
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };

fileBrowser_file saveDir_libfat_Default =
	{ "sd:/wiisx/saves",
	  0,
	  0,
	  0,
	  FILE_BROWSER_ATTR_DIR
	 };

fileBrowser_file saveDir_libfat_USB =
	{ "usb:/wiisx/saves",
	  0,
	  0,
	  0,
	  FILE_BROWSER_ATTR_DIR
	 };
	  
fileBrowser_file biosDir_libfat_Default =
	{ "sd:/wiisx/bios",
	  0,
	  0,
	  0,
	  FILE_BROWSER_ATTR_DIR
	 };

fileBrowser_file biosDir_libfat_USB =
	{ "usb:/wiisx/bios",
	  0,
	  0,
	  0,
	  FILE_BROWSER_ATTR_DIR
	 };
	 
fileBrowser_file biosDir_DVD =
	{ "dvd:/",
	  0,
	  0,
	  0,
	  FILE_BROWSER_ATTR_DIR
	 };


int fileBrowser_libfat_readDir(fileBrowser_file* file, fileBrowser_file** dir){

	DIR* dp = opendir( file->name );
	if(!dp) return FILE_BROWSER_ERROR;
	struct dirent *entry;
	struct stat fstat;
	
	// Set everything up to read
	int num_entries = 2, i = 0;
	*dir = malloc( num_entries * sizeof(fileBrowser_file) );
	// Read each entry of the directory
	while( (entry = readdir(dp)) != NULL ){
		// Make sure we have room for this one
		if(i == num_entries){
			++num_entries;
			*dir = realloc( *dir, num_entries * sizeof(fileBrowser_file) ); 
		}
		snprintf((*dir)[i].name, 255, "%s/%s", file->name, entry->d_name);
		stat((*dir)[i].name,&fstat);
		(*dir)[i].offset = 0;
		(*dir)[i].size   = fstat.st_size;
		(*dir)[i].attr   = (fstat.st_mode & _IFDIR) ?
		                     FILE_BROWSER_ATTR_DIR : 0;
		++i;
	}
	
	closedir(dp);

	return num_entries;
}

int fileBrowser_libfat_open(fileBrowser_file* file) {
  struct stat fileInfo;
  if(!stat(&file->name[0], &fileInfo)){
    file->offset = 0;
    file->discoffset = 0;
    file->size = fileInfo.st_size;
    return 0;
  }
  return FILE_BROWSER_ERROR_NO_FILE;
}

int fileBrowser_libfat_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	if(type == FILE_BROWSER_SEEK_SET) file->offset = where;
	else if(type == FILE_BROWSER_SEEK_CUR) file->offset += where;
	else file->offset = file->size + where;

	return 0;
}

int fileBrowser_libfat_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	FILE* f = fopen( file->name, "rb" );
	if(!f) return FILE_BROWSER_ERROR;

	fseek(f, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, f);
	if(bytes_read > 0) file->offset += bytes_read;

	fclose(f);
	return bytes_read;
}

int fileBrowser_libfat_writeFile(fileBrowser_file* file, void* buffer, unsigned int length){
	FILE* f = fopen( file->name, "wb" );
	if(!f) return FILE_BROWSER_ERROR;

	fseek(f, file->offset, SEEK_SET);
	int bytes_read = fwrite(buffer, 1, length, f);
	if(bytes_read > 0) file->offset += bytes_read;

	fclose(f);
	return bytes_read;
}

/* call fileBrowser_libfat_init as much as you like for all devices
    - returns 0 on device not present/error
    - returns 1 on ok
*/
int fileBrowser_libfat_init(fileBrowser_file* f){

	int res = 0;

#ifdef HW_RVL
  	if(f->name[0] == 's') {      //SD
		if((res = fatMountSimple ("sd", frontsd))) {
				res = 1;
		}
		else if(!res && fatMountSimple ("sd", carda)) {
			res = 1;
		}
		else if(!res && fatMountSimple ("sd", cardb)) {
			res = 1;
		}
 	}
	else if(f->name[0] == 'u') {	// USB
		int retries = 3;
		for(int i = 0; i < retries; i++) {
			if(fatMountSimple ("usb", usb)) {
				res = 1;
				break;
			}
			// my USB devices really seem to need this.
			sleep(1);
			if(res) break;
		}
	}
	else if(f->name[0] == 'd') {	// DVD
		if(ISO9660_Mount("dvd", dvd)) {
			res = 1;
		}
	}
	return res;
#else
	if(f->name[0] == 's') {
		if(m2loader->startup(m2loader)) {
			res = fatMountSimple ("sd", m2loader);
		}
		if(!res && gcloader->startup(gcloader)) {
			res = fatMountSimple ("sd", gcloader);
		}
		if(!res && sd2sp2->startup(sd2sp2)) {
			res = fatMountSimple ("sd", sd2sp2);
		}
		if(!res && carda->startup(carda)) {
			res = fatMountSimple ("sd", carda);
		}
		if(!res && cardb->startup(cardb)) {
			res = fatMountSimple ("sd", cardb);
		}
	}
	else if(f->name[0] == 'd') {	// DVD
		if(ISO9660_Mount("dvd", dvd)) {
			res = 1;
		}
	}
	return res; 				// Already mounted
#endif
}

int fileBrowser_libfat_deinit(fileBrowser_file* f){
  	if(f->name[0] == 's') {      //SD
		fatUnmount("sd");
 	}
#ifdef HW_RVL
	else if(f->name[0] == 'u') {	// USB
		fatUnmount("usb");
	}
#endif
	else if(f->name[0] == 'd') {	// DVD
		ISO9660_Unmount("dvd");
	}
	return 0;
}


/* Special for ISO,CDDA,SUB file reading only 
 * - Holds the same fat file handle to avoid fopen/fclose
 */

static FILE* fd;

int fileBrowser_libfatROM_deinit(fileBrowser_file* f){
	if(fd) {
		fclose(fd);
	}
	
	fd = NULL;
	return 0;
}

int fileBrowser_libfatROM_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
    if(!fd) fd = fopen( file->name, "rb");

	fseek(fd, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, fd);
	if(bytes_read > 0) {
  	file->offset += bytes_read;
	}

	return bytes_read;
}

