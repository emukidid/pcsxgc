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
#include "plugins.h"
#include "PlugCD.h"
#include "PsxCommon.h"

extern void SysPrintf(char *fmt, ...);

// gets track 
long getTN(unsigned char* buffer)
{
	int numtracks = getNumTracks();

	//SysPrintf("start getTn()\r\n");

 	if (-1 == numtracks)
 	{
		buffer[0]=buffer[1]=1;
    	return -1;
 	}

 	buffer[0]=CD.tl[0].num;
 	buffer[1]=numtracks;

	//printf("getnumtracks %d %d\n", (int)buffer[0], (int)buffer[1]);
	//SysPrintf("end getTn()\r\n");
   return 0;
}

// if track==0 -> return total length of cd
// otherwise return start in bcd time format
long getTD(int track, unsigned char* buffer)
{
	// SysPrintf("start getTD()\r\n");

	if (track > CD.numtracks)
	{
	// printf("getTD bad %2d\n", track);
		return -1;
	}

   if (track == 0)
   {
      buffer[0] = CD.tl[CD.numtracks-1].end[0];
      buffer[1] = CD.tl[CD.numtracks-1].end[1];
      buffer[2] = CD.tl[CD.numtracks-1].end[2];
   }
	else
	{
		buffer[0] = CD.tl[track-1].start[0];
		buffer[1] = CD.tl[track-1].start[1];
		buffer[2] = CD.tl[track-1].start[2];
	}
	// printf("getTD %2d %02d:%02d:%02d\n", track, (int)buffer[0],
	//       (int)buffer[1], (int)buffer[2]);

	// bcd encode it
	buffer[0] = intToBCD(buffer[0]);
	buffer[1] = intToBCD(buffer[1]);
	buffer[2] = intToBCD(buffer[2]);
	// SysPrintf("end getTD()\r\n");
	return 0;
}

// opens a binary cd image and calculates its length
void openBin(const char* filename)
{
	long size, blocks;
	struct stat fileInfo;
	CD.cd = fopen(filename, "rb");
		
	if (CD.cd == 0)
	{
		SysPrintf("Error opening cd\n");
		while(1);
		return;
	}
	stat(filename, &fileInfo);
	size = fileInfo.st_size;
	SysPrintf("size of CD in MB = %d\r\n",size/1048576);
	
	rc = fseek(CD.cd, 0, SEEK_SET);
	blocks = size / 2352;
	
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
	
	//CD.bufferSize = 0;
    CD.bufferPos = 0x7FFFFFFF;
  //  CD.status = 0x00;
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

// Given the name of a cue sheet, parse its track info
void openCue(const char* filename)
{
	// Get the filesize and open the file
	struct stat cueInfo;
	stat(filename, &cueInfo);
	FILE* cue = fopen(filename, "r");
	if(!cue){ SysPrintf("Failed to open %s\n", filename); while(1); }
	
	// Create a buffer large enough to hold the text and a terminating null
	char* cueText = malloc(cueInfo.st_size+1);
	if(!cueText){
		SysPrintf("Failed to malloc %dB\n", cueInfo.st_size+1);
		fclose(cue); while(1);
	}
	
	// Read in the text and end the string with a null
	fread(cueText, 1, cueInfo.st_size, cue);
	cueText[cueInfo.st_size] = 0;
	fclose(cue);
	
	// FIXME: Make sure we don't have an old tl we're leaking
	CD.tl = malloc(sizeof(Track));
	CD.numtracks = 1;
	int num_tracks_seen = 0;
	char bin_filename[80];
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
	char relative[80];
	sprintf(relative, "%s/%s", CDConfiguration.dn, bin_filename);
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
	
	// Actually open the data for reading
	// FIXME: Make sure I'm not leaving an old cd open
	CD.cd = fopen(bin_filename, "rb");
	if(!CD.cd){
		SysPrintf("Failed to open %s for reading\n", bin_filename);
		while(1);
	}
}

// new file types should be added here and in the CDOpen function
void newCD(const char * filename)
{
	const char* ext = filename + strlen(filename) - 4;
	SysPrintf("Opening file with extension %s\n", ext);
	
	if( !strcmp(ext, ".cue") ){ // FIXME: Play nicely with case
		CD.type = Cue;
		openCue(filename);
	} else {
		CD.type = Bin;
		openBin(filename);
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
	if (CD.cd == 0) {
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
		rc = fseek(CD.cd, CD.sector, SEEK_SET);
		rc = fread(CD.buffer, BUFFER_SIZE, 1, CD.cd);
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
	char str[256];
	
	SysPrintf("start CDR_open()\r\n");
	
	// newCD("/cd/cd.bin");
	strcpy(str, CDConfiguration.dn);
	strcat(str, "/");
	strcat(str, CDConfiguration.fn);
	
	newCD(str);
	
	SysPrintf("end CDR_open()\r\n");
	return 0;
}

char* textFileBrowser(char*);
long CDR__init(void) {
	SysPrintf("start CDR_init()\r\n");
	//strcpy(CDConfiguration.dn, "/PSXISOS");
	//strcpy(CDConfiguration.fn, "GAME.ISO");
	char* filename = textFileBrowser("/PSXISOS");
	char* last_slash = strrchr(filename, '/');
	*last_slash = 0; // Separate the strings
	if(!filename) {
	  SysPrintf("/PSXISOS directory doesn't exist!\n");
	  while(1);
  }
	strcpy(CDConfiguration.dn, filename);
	strcpy(CDConfiguration.fn, last_slash+1);
	free(filename);
	SysPrintf("end CDR_init()\r\n");
	return 0;
}

long CDR__shutdown(void) {
	return 0;
}

long CDR__close(void) {
	SysPrintf("start CDR_close()\r\n");
	fclose(CD.cd);
	free(CD.tl);
	SysPrintf("end CDR_close()\r\n");
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
	if (CD.cd != 0)
		seekSector(BCDToInt(time[0]), BCDToInt(time[1]), BCDToInt(time[2]));
	return PSE_CDR_ERR_SUCCESS;
}

/* called after the read should be finished, and the data is needed */
unsigned char *CDR__getBuffer(void) {
	if (CD.cd == 0)
		return NULL;
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
