/**
 * WiiSX - fileBrowser.h
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * Standard protoypes for accessing files from anywhere
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

#ifndef FILE_BROWSER_H
#define FILE_BROWSER_H

#include <stdint.h>

#define FILE_BROWSER_MAX_PATH_LEN 128

#define FILE_BROWSER_ATTR_DIR     0x10

#define FILE_BROWSER_ERROR         -1
#define FILE_BROWSER_ERROR_NO_FILE -2

#define FILE_BROWSER_SEEK_SET 1
#define FILE_BROWSER_SEEK_CUR 2
#define FILE_BROWSER_SEEK_END 3

#define FILE_BROWSER_ISO_FILE_PTR  0
#define FILE_BROWSER_CDDA_FILE_PTR 1
#define FILE_BROWSER_SUB_FILE_PTR  2

typedef struct {
	char         name[FILE_BROWSER_MAX_PATH_LEN];
	uint64_t discoffset; // Only necessary for DVD
	unsigned int offset; // Keep track of our offset in the file
	unsigned int size;
	unsigned int attr;
} fileBrowser_file;

// When you invoke a fileBrowser for ISOs, it should be invoked with this
extern fileBrowser_file* isoFile_topLevel;

// Set this to directory for memory cards & save states
extern fileBrowser_file* saveFile_dir;

// Set this to directory for the bios file(s)
extern fileBrowser_file* biosFile_dir;

// This is the currently loaded CD image
extern fileBrowser_file isoFile;
// This is the currently selected bios file
extern fileBrowser_file* biosFile;

// -- isoFile function pointers --
/* Must be called before any using other functions */
extern int (*isoFile_init)(fileBrowser_file*);

/* open checks if a file exists and fills out the fileBrowser_file* */
extern int (*isoFile_open)(fileBrowser_file*);

/* readDir functions should return the number of directory entries
     or an error of the given file pointer and fill out the file array */
extern int (*isoFile_readDir)(fileBrowser_file*, fileBrowser_file**);

/* readFile returns the status of the read and reads if it can
     arguments: file*, buffer, length */
extern int (*isoFile_readFile)(fileBrowser_file*, void*, unsigned int);

/* seekFile returns the status of the seek and seeks if it can
     arguments: file*, offset, seek type */
extern int (*isoFile_seekFile)(fileBrowser_file*, unsigned int, unsigned int);

/* Should be called after done using other functions */
extern int (*isoFile_deinit)(fileBrowser_file*);

// -- saveFile function pointers --
extern int (*saveFile_init)(fileBrowser_file*);

/* Checks whether the file exists, and fills out any inconsistent info */
//extern int (*saveFile_exists)(fileBrowser_file*);

extern int (*saveFile_readFile)(fileBrowser_file*, void*, unsigned int);

extern int (*saveFile_writeFile)(fileBrowser_file*, void*, unsigned int);

extern int (*saveFile_deinit)(fileBrowser_file*);

// -- biosFile function pointers --
extern int (*biosFile_init)(fileBrowser_file*);

extern int (*biosFile_open)(fileBrowser_file*);

extern int (*biosFile_readFile)(fileBrowser_file*, void*, unsigned int);

extern int (*biosFile_deinit)(fileBrowser_file*);

#endif

