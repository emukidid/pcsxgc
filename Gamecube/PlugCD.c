/* 	super basic CD plugin for PCSX Gamecube
	by emu_kidid based on the DC port

	TODO: Fix Missing CDDA support(?)
*/
#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <gccore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../plugins.h"
#include "PlugCD.h"
#include "../PsxCommon.h"

extern void SysPrintf(char *fmt, ...);

// Gets track number
long getTN(unsigned char* buffer)
{
	int numtracks = getNumTracks();

 	if (-1 == numtracks) {
		buffer[0]=buffer[1]=1;
    return -1;
 	}

 	buffer[0]=CD.tl[0].num;
 	buffer[1]=numtracks;
  return 0;
}

// if track==0 -> return total length of cd
// otherwise return start in bcd time format
long getTD(int track, unsigned char* buffer)
{
	if (track > CD.numtracks) {
		return -1;  //bad
	}

  if (track == 0) {
    buffer[0] = CD.tl[CD.numtracks-1].end[0];
    buffer[1] = CD.tl[CD.numtracks-1].end[1];
    buffer[2] = CD.tl[CD.numtracks-1].end[2];
  }
	else {
		buffer[0] = CD.tl[track-1].start[0];
		buffer[1] = CD.tl[track-1].start[1];
		buffer[2] = CD.tl[track-1].start[2];
	}

	// bcd encode it
	buffer[0] = intToBCD(buffer[0]);
	buffer[1] = intToBCD(buffer[1]);
	buffer[2] = intToBCD(buffer[2]);
	return 0;
}

void addBinTrackInfo()
{
	CD.tl = realloc(CD.tl, (CD.numtracks + 1) * sizeof(Track));
	CD.tl[CD.numtracks].end[0] = CD.tl[0].end[0];
	CD.tl[CD.numtracks].end[1] = CD.tl[0].end[1];
	CD.tl[CD.numtracks].end[2] = CD.tl[0].end[2];
	CD.tl[CD.numtracks].start[0] = CD.tl[0].start[0];
	CD.tl[CD.numtracks].start[1] = CD.tl[0].start[1];
	CD.tl[CD.numtracks].start[2] = CD.tl[0].start[2];
}

// opens a binary cd image and calculates its length
void openBin(fileBrowser_file* file)
{
	long blocks;

	isoFile_seekFile(file,0,SEEK_SET);
	blocks = file->size / 2352;

	// put the track length info in the track list
	CD.tl = (Track*) malloc(sizeof(Track));

	CD.tl[0].type = Mode2;
	CD.tl[0].num = 1;
	CD.tl[0].start[0] = 0;
	CD.tl[0].start[1] = 0;
	CD.tl[0].start[2] = 0;
	CD.tl[0].end[2] = blocks % 75;
	CD.tl[0].end[1] = ((blocks - CD.tl[0].end[2]) / 75) % 60;
	CD.tl[0].end[0] = (((blocks - CD.tl[0].end[2]) / 75) - CD.tl[0].end[1]) / 60;

	CD.tl[0].start[1] += 2;
	CD.tl[0].end[1] += 2;

	normalizeTime(CD.tl[0].end);

	CD.numtracks = 1;
  CD.bufferPos = 0x7FFFFFFF;
}

// Given a cue sheet fileBrowser_file pointer, parse its track info
void openCue(fileBrowser_file* cueFile)
{

	// Create a buffer large enough to hold the text and a terminating null
	char* cueText = malloc(cueFile->size +1);
	if(!cueText) {
		SysPrintf("Failed to malloc %dB\n", cueFile->size +1);
		while(1);
	}

	// Read in the text and end the string with a null
	isoFile_readFile(cueFile,cueText,cueFile->size);
	cueText[cueFile->size] = 0;
	isoFile_deinit(cueFile);

	// FIXME: Make sure we don't have an old tl we're leaking
	CD.tl = malloc(sizeof(Track));
	CD.numtracks = 1;
	int num_tracks_seen = 0;
	char bin_filename[256];
	// Get the first token
	char* token = strtok(cueText, " \t\n\r");
	// nxttok() is just shorthand for using strtok to get the next
	#define nxttok() strtok(NULL, " \t\n\r")
	// Read and parse the cue sheet
	while(token){
		// Check against keywords we're intested in and handle if necessary
		if( !strcmp(token, "FILE") ){
			// Read in the (first) token for the bin filename
			char* temp = nxttok();
			if(temp[0] != '"')
				// The filename isn't quoted, just strcpy it to bin_filename
				strncpy(bin_filename, temp, 80);
			else {
				// Read in the quoted string tokens to bin_filename
				char* dst = bin_filename;
				do {
					if(!(*(++temp))){ temp = nxttok(); *(dst++) = ' '; }
					*(dst++) = *temp;
				} while(*temp != '"');
				*(dst-1) = 0;
			}
			// FIXME: Byteswap based on BINARY vs MOTOROLA?
			/* file_type = */ nxttok();

		} else if( !strcmp(token, "TRACK") ){
			if(++num_tracks_seen > CD.numtracks)
				CD.tl = realloc(CD.tl, ++CD.numtracks * sizeof(Track));
			/* track_num = */ nxttok();

			char* track_type = nxttok();
			if( !strcmp(track_type, "AUDIO") )
				CD.tl[CD.numtracks-1].type = Audio;
			else if( !strcmp(track_type, "MODE1/2352") )
				CD.tl[CD.numtracks-1].type = Mode1;
			else if( !strcmp(track_type, "MODE2/2352") )
				CD.tl[CD.numtracks-1].type = Mode2;
			else
				CD.tl[CD.numtracks-1].type = unknown;

		} else if( !strcmp(token, "INDEX") ){
			nxttok(); // Read and discard the "01"
			char* track_start = nxttok(); // mm:ss:ff format
			SysPrintf("Track beginning at %s\n", track_start);
			track_start[2] = track_start[4] = 0; // Null out the ':'
			// Parse the start index to the start value
			CD.tl[CD.numtracks-1].start[0] = atoi(track_start+0);
			CD.tl[CD.numtracks-1].start[1] = atoi(track_start+3);
			CD.tl[CD.numtracks-1].start[2] = atoi(track_start+6);
			// If we've already seen another track, this is its end
			if(CD.numtracks > 1){
				CD.tl[CD.numtracks-2].end[0] = CD.tl[CD.numtracks-1].start[0];
				CD.tl[CD.numtracks-2].end[1] = CD.tl[CD.numtracks-1].start[1];
				CD.tl[CD.numtracks-2].end[2] = CD.tl[CD.numtracks-1].start[2];
			}

		}
		// Get the next token
		token = nxttok();
	}
	#undef nxttok
	// Free the buffer
	free(cueText);

	// Create a string with the bin filename based on the cue's path
	char relative[256];
	sprintf(relative, "%s/%s", isoFile_topLevel->name, bin_filename);
	// Determine relative vs absolute path and get the stat
	struct stat binInfo;
	if( stat(relative, &binInfo) ){
		SysPrintf("Failed to open %s\n", relative);
		// The relative path failed, try absolute
		if( stat(bin_filename, &binInfo) ){
			SysPrintf("Failed to open %s\n", bin_filename);
			while(1);
		}
	} else strcpy(bin_filename, relative);
	// Fill out the last track's end based on size
	unsigned int blocks = binInfo.st_size / 2352;
	CD.tl[CD.numtracks-1].end[2] = blocks % 75;
	CD.tl[CD.numtracks-1].end[1] = ((blocks - CD.tl[CD.numtracks-1].end[2]) / 75) % 60;
	CD.tl[CD.numtracks-1].end[0] = (((blocks - CD.tl[CD.numtracks-1].end[2]) / 75)
	                             - CD.tl[CD.numtracks-1].end[1]) / 60;
	normalizeTime(CD.tl[CD.numtracks-1].end);

	// setup the isoFile to now point to the .iso
	if(isoFile) free(isoFile);
	isoFile = malloc(sizeof(fileBrowser_file));
	strcpy(isoFile->name,bin_filename);
	isoFile->offset = 0;
	isoFile->size   = binInfo.st_size;
	printf("Loaded bin from cue: %s size: %d\n", isoFile->name, isoFile->size);

}

// new file types should be added here and in the CDOpen function
void newCD(fileBrowser_file *file)
{
	const char* ext = file->name + strlen(file->name) - 4;
	SysPrintf("Opening file with extension %s size: %d\n", ext, file->size);
	
	if(( !strcmp(ext, ".cue") ) || ( !strcmp(ext, ".CUE") )){
		CD.type = Cue;
		openCue(file);  //if we open a .cue, the isoFile global will be redirected
	} else {
		CD.type = Bin;
		openBin(file);  //setup tracks
		addBinTrackInfo();
	}

	CD.bufferPos = 0x7FFFFFFF;
	seekSector(0,2,0);
}

// return the sector address - the buffer address + 12 bytes for offset.
unsigned char* getSector(int subchannel)
{
	return CD.buffer + (CD.sector - CD.bufferPos) + ((subchannel) ? 0 : 12);
}

// returns the number of tracks
char getNumTracks()
{
	if (!isoFile) {
	    return -1;
	}
	return CD.numtracks;
}

void readit(const unsigned char m, const unsigned char s, const unsigned char f)
{

	// fakie ISO support.  iso is just cd-xa data without the ecc and header.
	// read in the same number of sectors then space it out to look like cd-xa
	/* if (CD.type == Iso)
	{
		unsigned char temptime[3];
		long tempsector = (( (m * 60) + (s - 2)) * 75 + f) * 2048;
		fs_seek(CD.cd, tempsector, SEEK_SET);
		fs_read(CD.buffer, sizeof(unsigned char), 2048*BUFFER_SECTORS, CD.cd);

		// spacing out the data here...
		for(tempsector = BUFFER_SECTORS - 1; tempsector >= 0; tempsector--)
		{
			memcpy(&CD.buffer[tempsector*2352 + 24],&CD.buffer[tempsector*2048], 2048);

			// two things - add the m/s/f flags in case anyone is looking
			// and change the xa mode to 1
			temptime[0] = m;
			temptime[1] = s;
			temptime[2] = f + (unsigned char)tempsector;

			normalizeTime(temptime);
			CD.buffer[tempsector*2352+12] = intToBCD(temptime[0]);
			CD.buffer[tempsector*2352+12+1] = intToBCD(temptime[1]);
			CD.buffer[tempsector*2352+12+2] = intToBCD(temptime[2]);
			CD.buffer[tempsector*2352+12+3] = 0x01;
		}
	}
	else */
	{
	  isoFile_seekFile(isoFile,CD.sector, FILE_BROWSER_SEEK_SET);
	  if(CD.sector + BUFFER_SIZE > isoFile->size)
      isoFile_readFile(isoFile,CD.buffer,isoFile->size-CD.sector); //read till the end
    else
      isoFile_readFile(isoFile,CD.buffer,BUFFER_SIZE);            //read buffer size
	}

	CD.bufferPos = CD.sector;
}


void seekSector(const unsigned char m, const unsigned char s, const unsigned char f)
{
	// calc byte to search for
	int sector = (( (m * 60) + (s - 2)) * 75 + f);
	CD.sector = sector * 2352;
	
	// is it cached?
	if ((CD.sector >= CD.bufferPos) && (CD.sector < (CD.bufferPos + BUFFER_SIZE)) ) 
	{
	    return;
	}
	// not cached - read a few blocks into the cache
	else
	{
		readit(m,s,f);
	}
}

long CDR__open(void)
{
  if(isoFile)
	  newCD(isoFile);
	return 0;
}

long CDR__init(void) {
	return 0;
}

long CDR__shutdown(void) {
	return 0;
}

long CDR__close(void) {
  if(CD.tl)
	  free(CD.tl);
	return 0;
}

long CDR__getTN(unsigned char *buffer) {
  return getTN(buffer);
}

long CDR__getTD(unsigned char track, unsigned char *buffer) {

	unsigned char temp[3];
	int result = getTD((int)track, temp);

	if (result == -1) return -1;

	buffer[1] = temp[1];
	buffer[2] = temp[0];
	return 0;
}

/* called when the psx requests a read */
long CDR__readTrack(unsigned char *time) {
	if (isoFile) {
		seekSector(BCDToInt(time[0]), BCDToInt(time[1]), BCDToInt(time[2]));
		return PSE_CDR_ERR_SUCCESS;
	}
	else
	  return PSE_CDR_ERR_FAILURE;
}

/* called after the read should be finished, and the data is needed */
unsigned char *CDR__getBuffer(void) {
	if (!isoFile)
		return NULL;
	else
    return getSector(0);
}

unsigned char *CDR__getBufferSub(void) {
  return getSector(1);
}

/*
long (CALLBACK* CDRconfigure)(void);
long (CALLBACK* CDRtest)(void);
void (CALLBACK* CDRabout)(void);
long (CALLBACK* CDRplay)(unsigned char *);
long (CALLBACK* CDRstop)(void);
*/
