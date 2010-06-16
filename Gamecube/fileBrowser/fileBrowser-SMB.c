/**
 * Wii64 - fileBrowser-SMB.c
 * Copyright (C) 2010 emu_kidid
 * 
 * fileBrowser module for Samba based shares
 *
 * WiiSX homepage: http://www.emulatemii.com
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
#include <network.h>
#include <smb.h>
#include "fileBrowser.h"
#include "fileBrowser-libfat.h"

/* SMB Globals */
int net_initialized = 0;
int smb_initialized = 0;

extern char smbUserName[];
extern char smbPassWord[];
extern char smbShareName[];
extern char smbIpAddr[];

fileBrowser_file topLevel_SMB =
	{ "smb:/", // file name
	  0ULL,      // discoffset (u64)
	  0,         // offset
	  0,         // size
	  FILE_BROWSER_ATTR_DIR
	};
 
// Init the GC/Wii net interface (wifi/bba/etc)
void init_network() {
 
  char ip[16];
  int res = 0;
  
  if(net_initialized) {
    return;
  }
  
  res = if_config(ip, NULL, NULL, true);
  if(!res) {
    net_initialized = 1;
  }
  else {
    net_initialized = 0;
  }
}

// Connect to the share specified in settings.cfg
void init_samba() {
  
  int res = 0;
  
  if(smb_initialized) {
    return;
  }
  res = smbInit(&smbUserName[0], &smbPassWord[0], &smbShareName[0], &smbIpAddr[0]);
  if(res) {
    smb_initialized = 1;
  }
  else {
    smb_initialized = 0;
  }
}

	
int fileBrowser_SMB_readDir(fileBrowser_file* ffile, fileBrowser_file** dir){	
   
  // We need all the settings filled out
  if(!strlen(&smbUserName[0]) || !strlen(&smbPassWord[0]) || !strlen(&smbShareName[0]) || !strlen(&smbIpAddr[0])) {
    return -1;
  }
  
  if(!net_initialized) {       //Init if we have to
    init_network();
    if(!net_initialized) {
      return net_initialized; //fail
    }
  } 
  
  if(!smb_initialized) {       //Connect to the share
    init_samba();
    if(!smb_initialized) {
      return smb_initialized; //fail
    }
  }
		
	// Call the corresponding FAT function
	return fileBrowser_libfat_readDir(ffile, dir);
}

int fileBrowser_SMB_open(fileBrowser_file* file) {
  return fileBrowser_libfat_open(file);
}

int fileBrowser_SMB_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	return fileBrowser_libfat_seekFile(file,where,type);
}

int fileBrowser_SMB_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	return fileBrowser_libfatROM_readFile(file,buffer,length);
}

int fileBrowser_SMB_init(fileBrowser_file* file){
	return 0;
}

int fileBrowser_SMB_deinit(fileBrowser_file* file) {
  /*if(smb_initialized) {
    smbClose("smb");
    smb_initialized = 0;
  }*/
	return fileBrowser_libfatROM_deinit(file);
}

