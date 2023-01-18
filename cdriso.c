/***************************************************************************
 *   Copyright (C) 2007 PCSX-df Team                                       *
 *   Copyright (C) 2009 Wei Mingzhi                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

#include "./Gamecube/DEBUG.h"
#include "./Gamecube/MEM2.h"
#include "PsxCommon.h"
#include "plugins.h"
#include "CdRom.h"
#include "cdriso.h"

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <ogc/lwp.h>
#include <sys/time.h>
#define PLAY_STACK_SIZE 1024 // MEM: I could get away with a smaller stack
static char  play_stack[PLAY_STACK_SIZE];
#define PLAY_PRIORITY 100
#endif

static FILE *cdHandle = NULL;
static FILE *cddaHandle = NULL;
static FILE *subHandle = NULL;

static boolean subChanMixed = FALSE;
static boolean subChanRaw = FALSE;

static unsigned char* readaheadBuffer = (char*)CDREADAHEAD_LO;

static unsigned char cdbuffer[DATA_SIZE];
static unsigned char subbuffer[SUB_FRAMESIZE];

static unsigned char sndbuffer[CD_FRAMESIZE_RAW * 10];

#define CDDA_FRAMETIME			(1000 * (sizeof(sndbuffer) / CD_FRAMESIZE_RAW) / 75)


#define MODE1_DATA_SIZE			2048

static boolean isMode1ISO = FALSE;


#ifdef _WIN32
static HANDLE threadid;
#else
static lwp_t threadid;
#endif
static unsigned int initial_offset = 0;
static volatile boolean playing = FALSE;
static boolean cddaBigEndian = FALSE;
static volatile unsigned int cddaCurOffset = 0;

char* CALLBACK CDR__getDriveLetter(void);
long CALLBACK CDR__configure(void);
long CALLBACK CDR__test(void);
void CALLBACK CDR__about(void);
long CALLBACK CDR__setfilename(char *filename);
long CALLBACK CDR__getStatus(struct CdrStat *stat);

extern void *hCDRDriver;

struct trackinfo {
	enum {DATA=1, CDDA} type;
	u8 start[3];		// MSF-format
	u8 length[3];		// MSF-format
};

#define MAXTRACKS 100 /* How many tracks can a CD hold? */

static int numtracks = 0;
static struct trackinfo ti[MAXTRACKS];

// get a sector from a msf-array
unsigned int msf2sec(u8 *msf) {
	return ((msf[0] * 60 + msf[1]) * 75) + msf[2];
}

void sec2msf(unsigned int s, u8 *msf) {
	msf[0] = s / 75 / 60;
	s = s - msf[0] * 75 * 60;
	msf[1] = s / 75;
	s = s - msf[1] * 75;
	msf[2] = s;
}

// divide a string of xx:yy:zz into m, s, f
static void tok2msf(char *time, char *msf) {
	char *token;

	token = strtok(time, ":");
	if (token) {
		msf[0] = atoi(token);
	}
	else {
		msf[0] = 0;
	}

	token = strtok(NULL, ":");
	if (token) {
		msf[1] = atoi(token);
	}
	else {
		msf[1] = 0;
	}

	token = strtok(NULL, ":");
	if (token) {
		msf[2] = atoi(token);
	}
	else {
		msf[2] = 0;
	}
}

#ifndef _WIN32
static long GetTickCount(void) {
	static time_t		initial_time = 0;
	struct timeval		now;

	gettimeofday(&now, NULL);

	if (initial_time == 0) {
		initial_time = now.tv_sec;
	}

	return (now.tv_sec - initial_time) * 1000L + now.tv_usec / 1000L;
}
#endif


u16 *iso_play_cdbuf;
u16 iso_play_bufptr;


// this thread plays audio data
#ifdef _WIN32
static void playthread(void *param)
#else
static void *playthread(void *param)
#endif
{

	long			d, t, i, s;
	unsigned char	tmp;
	int sec;

	t = GetTickCount();

	iso_play_cdbuf = 0;
	iso_play_bufptr = 0;

	while (playing) {
#ifdef PROFILE
	start_section(CDDA_SECTION);
#endif
		d = t - (long)GetTickCount();
		if (d <= 0) {
			d = 1;
		}
		else if (d > CDDA_FRAMETIME) {
			d = CDDA_FRAMETIME;
		}
#ifdef _WIN32
		Sleep(d);
#else
		usleep(d * 1000);
#endif

		t = GetTickCount() + CDDA_FRAMETIME;

		if (subChanMixed) {
			s = 0;

			for (i = 0; i < sizeof(sndbuffer) / CD_FRAMESIZE_RAW; i++) {
				// read one sector
				d = fread(sndbuffer + CD_FRAMESIZE_RAW * i, 1, CD_FRAMESIZE_RAW, cddaHandle);
				if (d < CD_FRAMESIZE_RAW) {
					break;
				}

				s += d;

				fread( subbuffer, 1, SUB_FRAMESIZE, cddaHandle );
			}
		}
		else {
			s = fread(sndbuffer, 1, sizeof(sndbuffer), cddaHandle);

			sec = cddaCurOffset / CD_FRAMESIZE_RAW;

			if (subHandle != NULL) {
				fseek(subHandle, sec * SUB_FRAMESIZE, SEEK_SET);
				fread(subbuffer, 1, SUB_FRAMESIZE, subHandle);
			}
		}

		if (s == 0) {
			playing = FALSE;
			fclose(cddaHandle);
			cddaHandle = NULL;
			initial_offset = 0;
			break;
		}

		if (!cdr.Muted && playing) {
			if (cddaBigEndian) {
				for (i = 0; i < s / 2; i++) {
					tmp = sndbuffer[i * 2];
					sndbuffer[i * 2] = sndbuffer[i * 2 + 1];
					sndbuffer[i * 2 + 1] = tmp;
				}
			}

			// wipe data track
			if( subHandle || subChanMixed ) {
				if( ti[ ((struct SubQ *) subbuffer)->TrackNumber ].type == DATA )
					memset( sndbuffer, 0, s );
			}

			SPU_playCDDAchannel((short *)sndbuffer, s);
		}


		cddaCurOffset += s;

	
		// BIOS CD Player: Fast forward / reverse seek
		if( cdr.FastForward ) {
			// ~+0.25 sec
			cddaCurOffset += CD_FRAMESIZE_RAW * 75 * 3;

#if 0
			// Bad idea: too much static
			if( subChanInterleaved )
				fseek( cddaHandle, s * (CD_FRAMESIZE_RAW + SUB_FRAMESIZE), SEEK_SET );
			else
				fseek( cddaHandle, s * CD_FRAMESIZE_RAW, SEEK_SET );
#endif
		}
		else if( cdr.FastBackward ) {
			// ~-0.25 sec
			cddaCurOffset -= CD_FRAMESIZE_RAW * 75 * 3;
			if( cddaCurOffset & 0x80000000 ) {
				cddaCurOffset = 0;
				cdr.FastBackward = 0;

				playing = 0;
				fclose(cddaHandle);
				cddaHandle = NULL;
				initial_offset = 0;
				break;
			}

#if 0
			// Bad idea: too much static
			if( subChanInterleaved )
				fseek( cddaHandle, s * (CD_FRAMESIZE_RAW + SUB_FRAMESIZE), SEEK_SET );
			else
				fseek( cddaHandle, s * CD_FRAMESIZE_RAW, SEEK_SET );
#endif
		}

	
		// Vib Ribbon: decoded buffer IRQ
		iso_play_cdbuf = (u16*)sndbuffer;
		iso_play_bufptr = 0;
#ifdef PROFILE
	end_section(CDDA_SECTION);
#endif
	}
#ifdef _WIN32
	_endthread();
#else
	//pthread_exit(0);
	return NULL;
#endif
}

// stop the CDDA playback
static void stopCDDA() {
	if (!playing) {
		return;
	}

	playing = FALSE;
#ifdef _WIN32
	WaitForSingleObject(threadid, INFINITE);
#else
	LWP_JoinThread(threadid, NULL);
#endif

	if (cddaHandle != NULL) {
		fclose(cddaHandle);
		cddaHandle = NULL;
	}

	initial_offset = 0;
}

// start the CDDA playback
static void startCDDA(unsigned int offset) {
	if (playing) {
		if (initial_offset == offset) {
			return;
		}
		stopCDDA();
	}

	cddaHandle = fopen(GetIsoFile(), "rb");
	if (cddaHandle == NULL) {
		return;
	}

	initial_offset = offset;
	cddaCurOffset = initial_offset;
	fseek(cddaHandle, initial_offset, SEEK_SET);

	playing = TRUE;

#ifdef _WIN32
	threadid = (HANDLE)_beginthread(playthread, 0, NULL);
#else
	LWP_CreateThread(&threadid, playthread, NULL, play_stack, PLAY_STACK_SIZE, PLAY_PRIORITY);
#endif
}

// this function tries to get the .toc file of the given .bin
// the necessary data is put into the ti (trackinformation)-array
static int parsetoc(const char *isofile) {
	char			tocname[MAXPATHLEN];
	FILE			*fi;
	char			linebuf[256], dummy[256], name[256];
	char			*token;
	char			time[20], time2[20];
	unsigned int	t;

	numtracks = 0;

	// copy name of the iso and change extension from .bin to .toc
	strncpy(tocname, isofile, sizeof(tocname));
	tocname[MAXPATHLEN - 1] = '\0';
	if (strlen(tocname) >= 4) {
		strcpy(tocname + strlen(tocname) - 4, ".toc");
	}
	else {
		return -1;
	}

	if ((fi = fopen(tocname, "r")) == NULL) {
		// try changing extension to .cue (to satisfy some stupid tutorials)
		strcpy(tocname + strlen(tocname) - 4, ".cue");
		if ((fi = fopen(tocname, "r")) == NULL) {
			// if filename is image.toc.bin, try removing .bin (for Brasero)
			strcpy(tocname, isofile);
			t = strlen(tocname);
			if (t >= 8 && strcmp(tocname + t - 8, ".toc.bin") == 0) {
				tocname[t - 4] = '\0';
				if ((fi = fopen(tocname, "r")) == NULL) {
					return -1;
				}
			}
			else {
				return -1;
			}
		}
	}

	memset(&ti, 0, sizeof(ti));
	cddaBigEndian = TRUE; // cdrdao uses big-endian for CD Audio

	// parse the .toc file
	while (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
		// search for tracks
		strncpy(dummy, linebuf, sizeof(linebuf));
		token = strtok(dummy, " ");

		if (token == NULL) continue;

		if (!strcmp(token, "TRACK")) {
			// get type of track
			token = strtok(NULL, " ");
			numtracks++;

			if (!strncmp(token, "MODE2_RAW", 9)) {
				ti[numtracks].type = DATA;
				sec2msf(2 * 75, ti[numtracks].start); // assume data track on 0:2:0

				// check if this image contains mixed subchannel data
				token = strtok(NULL, " ");
				if (token != NULL && !strncmp(token, "RW_RAW", 6)) {
					subChanMixed = TRUE;
					subChanRaw = TRUE;
				}
			}
			else if (!strncmp(token, "AUDIO", 5)) {
				ti[numtracks].type = CDDA;
			}
		}
		else if (!strcmp(token, "DATAFILE")) {
			if (ti[numtracks].type == CDDA) {
				sscanf(linebuf, "DATAFILE \"%[^\"]\" #%d %8s", name, &t, time2);
				t /= CD_FRAMESIZE_RAW + (subChanMixed ? SUB_FRAMESIZE : 0);
				t += 2 * 75;
				sec2msf(t, (u8 *)&ti[numtracks].start);
				tok2msf((char *)&time2, (char *)&ti[numtracks].length);
			}
			else {
				sscanf(linebuf, "DATAFILE \"%[^\"]\" %8s", name, time);
				tok2msf((char *)&time, (char *)&ti[numtracks].length);
			}
		}
		else if (!strcmp(token, "FILE")) {
			sscanf(linebuf, "FILE \"%[^\"]\" #%d %8s %8s", name, &t, time, time2);
			tok2msf((char *)&time, (char *)&ti[numtracks].start);
			t /= CD_FRAMESIZE_RAW + (subChanMixed ? SUB_FRAMESIZE : 0);
			t += msf2sec(ti[numtracks].start) + 2 * 75;
			sec2msf(t, (u8 *)&ti[numtracks].start);
			tok2msf((char *)&time2, (char *)&ti[numtracks].length);
		}
	}

	fclose(fi);

	return 0;
}

// this function tries to get the .cue file of the given .bin
// the necessary data is put into the ti (trackinformation)-array
static int parsecue(const char *isofile) {
	char			cuename[MAXPATHLEN];
	FILE			*fi;
	char			*token;
	char			time[20];
	char			*tmp;
	char			linebuf[256], dummy[256];
	unsigned int	t;

	numtracks = 0;

	// copy name of the iso and change extension from .bin to .cue
	strncpy(cuename, isofile, sizeof(cuename));
	cuename[MAXPATHLEN - 1] = '\0';
	if (strlen(cuename) >= 4) {
		strcpy(cuename + strlen(cuename) - 4, ".cue");
	}
	else {
		return -1;
	}

	if ((fi = fopen(cuename, "r")) == NULL) {
		return -1;
	}

	// Some stupid tutorials wrongly tell users to use cdrdao to rip a
	// "bin/cue" image, which is in fact a "bin/toc" image. So let's check
	// that...
	if (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
		if (!strncmp(linebuf, "CD_ROM_XA", 9)) {
			// Don't proceed further, as this is actually a .toc file rather
			// than a .cue file.
			fclose(fi);
			return parsetoc(isofile);
		}
		fseek(fi, 0, SEEK_SET);
	}

	memset(&ti, 0, sizeof(ti));

	while (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
		strncpy(dummy, linebuf, sizeof(linebuf));
		token = strtok(dummy, " ");

		if (token == NULL) {
			continue;
		}

		if (!strcmp(token, "TRACK")){
			numtracks++;

			if (strstr(linebuf, "AUDIO") != NULL) {
				ti[numtracks].type = CDDA;
			}
			else if (strstr(linebuf, "MODE1/2352") != NULL || strstr(linebuf, "MODE2/2352") != NULL) {
				ti[numtracks].type = DATA;
			}
		}
		else if (!strcmp(token, "INDEX")) {
			tmp = strstr(linebuf, "INDEX");
			if (tmp != NULL) {
				tmp += strlen("INDEX") + 3; // 3 - space + numeric index
				while (*tmp == ' ') tmp++;
				if (*tmp != '\n') sscanf(tmp, "%8s", time);
			}

			tok2msf((char *)&time, (char *)&ti[numtracks].start);

			t = msf2sec(ti[numtracks].start) + 2 * 75;
			sec2msf(t, ti[numtracks].start);

			// If we've already seen another track, this is its end
			if (numtracks > 1) {
				t = msf2sec(ti[numtracks].start) - msf2sec(ti[numtracks - 1].start);
				sec2msf(t, ti[numtracks - 1].length);
			}
		}
	}

	fclose(fi);

	// Fill out the last track's end based on size
	if (numtracks >= 1) {
		fseek(cdHandle, 0, SEEK_END);
		t = ftell(cdHandle) / 2352 - msf2sec(ti[numtracks].start) + 2 * 75;
		sec2msf(t, ti[numtracks].length);
	}

	return 0;
}

// this function tries to get the .ccd file of the given .img
// the necessary data is put into the ti (trackinformation)-array
static int parseccd(const char *isofile) {
	char			ccdname[MAXPATHLEN];
	FILE			*fi;
	char			linebuf[256];
	unsigned int	t;

	numtracks = 0;

	// copy name of the iso and change extension from .img to .ccd
	strncpy(ccdname, isofile, sizeof(ccdname));
	ccdname[MAXPATHLEN - 1] = '\0';
	if (strlen(ccdname) >= 4) {
		strcpy(ccdname + strlen(ccdname) - 4, ".ccd");
	}
	else {
		return -1;
	}

	if ((fi = fopen(ccdname, "r")) == NULL) {
		return -1;
	}

	memset(&ti, 0, sizeof(ti));

	while (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
		if (!strncmp(linebuf, "[TRACK", 6)){
			numtracks++;
		}
		else if (!strncmp(linebuf, "MODE=", 5)) {
			sscanf(linebuf, "MODE=%d", &t);
			ti[numtracks].type = ((t == 0) ? CDDA : DATA);
		}
		else if (!strncmp(linebuf, "INDEX 1=", 8)) {
			sscanf(linebuf, "INDEX 1=%d", &t);
			sec2msf(t + 2 * 75, ti[numtracks].start);

			// If we've already seen another track, this is its end
			if (numtracks > 1) {
				t = msf2sec(ti[numtracks].start) - msf2sec(ti[numtracks - 1].start);
				sec2msf(t, ti[numtracks - 1].length);
			}
		}
	}

	fclose(fi);

	// Fill out the last track's end based on size
	if (numtracks >= 1) {
		fseek(cdHandle, 0, SEEK_END);
		t = ftell(cdHandle) / 2352 - msf2sec(ti[numtracks].start) + 2 * 75;
		sec2msf(t, ti[numtracks].length);
	}

	return 0;
}

// this function tries to get the .mds file of the given .mdf
// the necessary data is put into the ti (trackinformation)-array
static int parsemds(const char *isofile) {
	char			mdsname[MAXPATHLEN];
	FILE			*fi;
	unsigned int	offset, extra_offset, l, i;
	unsigned short	s;

	numtracks = 0;

	// copy name of the iso and change extension from .mdf to .mds
	strncpy(mdsname, isofile, sizeof(mdsname));
	mdsname[MAXPATHLEN - 1] = '\0';
	if (strlen(mdsname) >= 4) {
		strcpy(mdsname + strlen(mdsname) - 4, ".mds");
	}
	else {
		return -1;
	}

	if ((fi = fopen(mdsname, "rb")) == NULL) {
		return -1;
	}

	memset(&ti, 0, sizeof(ti));

	// check if it's a valid mds file
	fread(&i, 1, sizeof(unsigned int), fi);
	i = SWAP32(i);
	if (i != 0x4944454D) {
		// not an valid mds file
		fclose(fi);
		return -1;
	}

	// get offset to session block
	fseek(fi, 0x50, SEEK_SET);
	fread(&offset, 1, sizeof(unsigned int), fi);
	offset = SWAP32(offset);

	// get total number of tracks
	offset += 14;
	fseek(fi, offset, SEEK_SET);
	fread(&s, 1, sizeof(unsigned short), fi);
	s = SWAP16(s);
	numtracks = s;

	// get offset to track blocks
	fseek(fi, 4, SEEK_CUR);
	fread(&offset, 1, sizeof(unsigned int), fi);
	offset = SWAP32(offset);

	// skip lead-in data
	while (1) {
		fseek(fi, offset + 4, SEEK_SET);
		if (fgetc(fi) < 0xA0) {
			break;
		}
		offset += 0x50;
	}

	// check if the image contains mixed subchannel data
	fseek(fi, offset + 1, SEEK_SET);
	subChanMixed = (fgetc(fi) ? TRUE : FALSE);

	// read track data
	for (i = 1; i <= numtracks; i++) {
		fseek(fi, offset, SEEK_SET);

		// get the track type
		ti[i].type = ((fgetc(fi) == 0xA9) ? CDDA : DATA);
		fseek(fi, 8, SEEK_CUR);

		// get the track starting point
		ti[i].start[0] = fgetc(fi);
		ti[i].start[1] = fgetc(fi);
		ti[i].start[2] = fgetc(fi);

		if (i > 1) {
			l = msf2sec(ti[i].start);
			sec2msf(l - 2 * 75, ti[i].start); // ???
		}

		// get the track length
		fread(&extra_offset, 1, sizeof(unsigned int), fi);
		extra_offset = SWAP32(extra_offset);

		fseek(fi, extra_offset + 4, SEEK_SET);
		fread(&l, 1, sizeof(unsigned int), fi);
		l = SWAP32(l);
		sec2msf(l, ti[i].length);

		offset += 0x50;
	}

	fclose(fi);
	return 0;
}

// this function tries to get the .sub file of the given .img
static int opensubfile(const char *isoname) {
	char		subname[MAXPATHLEN];

	// copy name of the iso and change extension from .img to .sub
	strncpy(subname, isoname, sizeof(subname));
	subname[MAXPATHLEN - 1] = '\0';
	if (strlen(subname) >= 4) {
		strcpy(subname + strlen(subname) - 4, ".sub");
	}
	else {
		return -1;
	}

	subHandle = fopen(subname, "rb");
	if (subHandle == NULL) {
		return -1;
	}

	return 0;
}

long CALLBACK ISOinit(void) {
	assert(cdHandle == NULL);
	assert(subHandle == NULL);

	return 0; // do nothing
}

long CALLBACK ISOshutdown(void) {
	if (cdHandle != NULL) {
		fclose(cdHandle);
		cdHandle = NULL;
	}
	if (subHandle != NULL) {
		fclose(subHandle);
		subHandle = NULL;
	}
	stopCDDA();
	return 0;
}

static void PrintTracks(void) {
	int i;

	for (i = 1; i <= numtracks; i++) {
		SysPrintf(_("Track %.2d (%s) - Start %.2d:%.2d:%.2d, Length %.2d:%.2d:%.2d\n"),
			i, (ti[i].type == DATA ? "DATA" : "AUDIO"),
			ti[i].start[0], ti[i].start[1], ti[i].start[2],
			ti[i].length[0], ti[i].length[1], ti[i].length[2]);
	}
}

// This function is invoked by the front-end when opening an ISO
// file for playback
long CALLBACK ISOopen(void) {
	u32 modeTest = 0;

	if (cdHandle != NULL) {
		return 0; // it's already open
	}

	cdHandle = fopen(GetIsoFile(), "rb");
	if (cdHandle == NULL) {
		return -1;
	}

	SysPrintf(_("Loaded CD Image: %s"), GetIsoFile());

	cddaBigEndian = FALSE;
	subChanMixed = FALSE;
	subChanRaw = FALSE;
	isMode1ISO = FALSE;

	if (parseccd(GetIsoFile()) == 0) {
		SysPrintf("[+ccd]");
	}
	else if (parsemds(GetIsoFile()) == 0) {
		SysPrintf("[+mds]");
	}
	else if (parsecue(GetIsoFile()) == 0) {
		SysPrintf("[+cue]");
	}
	else if (parsetoc(GetIsoFile()) == 0) {
		SysPrintf("[+toc]");
	} else {
		//guess whether it is mode1/2048
		fseek(cdHandle, 0, SEEK_END);
		if(ftell(cdHandle) % 2048 == 0) {
			fseek(cdHandle, 0, SEEK_SET);
			fread(&modeTest, 4, 1, cdHandle);
			if(SWAP32(modeTest)!=0xffffff00) isMode1ISO = TRUE;
		}
		fseek(cdHandle, 0, SEEK_SET);
	}

	if (!subChanMixed && opensubfile(GetIsoFile()) == 0) {
		SysPrintf("[+sub]");
	}

	SysPrintf(".\n");

	PrintTracks();

	return 0;
}

long CALLBACK ISOclose(void) {
	if (cdHandle != NULL) {
		fclose(cdHandle);
		cdHandle = NULL;
	}
	if (subHandle != NULL) {
		fclose(subHandle);
		subHandle = NULL;
	}
	stopCDDA();
	return 0;
}

// return Starting and Ending Track
// buffer:
//  byte 0 - start track
//  byte 1 - end track
long CALLBACK ISOgetTN(unsigned char *buffer) {
	buffer[0] = 1;

	if (numtracks > 0) {
		buffer[1] = numtracks;
	}
	else {
		buffer[1] = 1;
	}

	return 0;
}

// return Track Time
// buffer:
//  byte 0 - frame
//  byte 1 - second
//  byte 2 - minute
long CALLBACK ISOgetTD(unsigned char track, unsigned char *buffer) {
	if( track == 0 ) {
		unsigned int pos, size;
		unsigned char time[3];

		// Vib Ribbon: return size of CD
		// - ex. 20 min, 22 sec, 66 fra
		pos = ftell( cdHandle );
		fseek( cdHandle, 0, SEEK_END );
		size = ftell( cdHandle );
		fseek( cdHandle, pos, SEEK_SET );

		// relative -> absolute time (+2 seconds)
		size += 150 * 2352;

		sec2msf( size / 2352, time );
		buffer[2] = time[0];
		buffer[1] = time[1];
		buffer[0] = time[2];
	}
	else if (numtracks > 0 && track <= numtracks) {
		buffer[2] = ti[track].start[0];
		buffer[1] = ti[track].start[1];
		buffer[0] = ti[track].start[2];
	}
	else {
		buffer[2] = 0;
		buffer[1] = 2;
		buffer[0] = 0;
	}

	return 0;
}

// decode 'raw' subchannel data ripped by cdrdao
static void DecodeRawSubData(void) {
	unsigned char subQData[12];
	int i;

	memset(subQData, 0, sizeof(subQData));

	for (i = 0; i < 8 * 12; i++) {
		if (subbuffer[i] & (1 << 6)) { // only subchannel Q is needed
			subQData[i >> 3] |= (1 << (7 - (i & 7)));
		}
	}

	memcpy(&subbuffer[12], subQData, 12);
}

static int readAheadMisses = 0;
static int readAhead = 0;
static unsigned int readAheadOffset = 0;

void setReadAhead(int r) {
	print_gecko("Set read-ahead %i\r\n", r);
	//readAhead = r;
	readAheadMisses = 0;
}

// read track
// time: byte 0 - minute; byte 1 - second; byte 2 - frame
// uses bcd format
long CALLBACK ISOreadTrack(unsigned char *time) {
	if (cdHandle == NULL) {
		return -1;
	}
#ifdef PROFILE
	start_section(CDR_SECTION);
#endif

	if (subChanMixed) {
		fseek(cdHandle, MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2])) * (CD_FRAMESIZE_RAW + SUB_FRAMESIZE), SEEK_SET);
		print_gecko("subchanmixed read\r\n");
		fread(cdbuffer, 1, DATA_SIZE, cdHandle);
		fread(subbuffer, 1, SUB_FRAMESIZE, cdHandle);

		if (subChanRaw) DecodeRawSubData();
	}
	else {
		if(isMode1ISO) {
			print_gecko("isMode1ISO read\r\n");
			fseek(cdHandle, MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2])) * MODE1_DATA_SIZE, SEEK_SET);
			fread(cdbuffer + 12, 1, MODE1_DATA_SIZE, cdHandle);
			memset(cdbuffer, 0, 12); //not really necessary, fake mode 2 header
			cdbuffer[0] = (time[0]);
			cdbuffer[1] = (time[1]);
			cdbuffer[2] = (time[2]);
			cdbuffer[3] = 1; //mode 1
		} else {
			//print_gecko("!isMode1ISO read from %08X\r\n", offset);
			unsigned int offset = MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2])) * CD_FRAMESIZE_RAW;
			if(readAheadMisses > 2) setReadAhead(0);
			if(readAhead) {
				if(offset >= readAheadOffset && offset+CD_FRAMESIZE_RAW < readAheadOffset+CDREADAHEAD_SIZE) {
					// return some cached data
					memcpy(cdbuffer, &readaheadBuffer[offset-readAheadOffset], CD_FRAMESIZE_RAW);
				}
				else {
					readAheadMisses++;
					// read a bunch.
					readAheadOffset = offset;
					fseek(cdHandle, offset, SEEK_SET);
					fread(readaheadBuffer, 1, CDREADAHEAD_SIZE, cdHandle);
					memcpy(cdbuffer, &readaheadBuffer[0], CD_FRAMESIZE_RAW);
					print_gecko("read into cache %08X\r\n", offset);
				}
				
			}
			else {
				readAheadOffset = 0;
				fseek(cdHandle, offset, SEEK_SET);
				fread(cdbuffer, 1, DATA_SIZE, cdHandle);
			}
		}

		if (subHandle != NULL) {
			print_gecko("subHandle read\r\n");
			fseek(subHandle, MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2])) * SUB_FRAMESIZE, SEEK_SET);
			fread(subbuffer, 1, SUB_FRAMESIZE, subHandle);

			if (subChanRaw) DecodeRawSubData();
		}
	}
#ifdef PROFILE
	end_section(CDR_SECTION);
#endif
	return 0;
}

// return readed track
unsigned char * CALLBACK ISOgetBuffer(void) {
	return cdbuffer+12;
}

// plays cdda audio
// sector: byte 0 - minute; byte 1 - second; byte 2 - frame
// does NOT uses bcd format
long CALLBACK ISOplay(unsigned char *time) {
	if (SPU_playCDDAchannel != NULL) {
		if (subChanMixed) {
			startCDDA(MSF2SECT(time[0], time[1], time[2]) * (CD_FRAMESIZE_RAW + SUB_FRAMESIZE));
		}
		else {
			startCDDA(MSF2SECT(time[0], time[1], time[2]) * CD_FRAMESIZE_RAW);
		}
	}
	return 0;
}

// stops cdda audio
long CALLBACK ISOstop(void) {
	stopCDDA();
	return 0;
}

// gets subchannel data
unsigned char* CALLBACK ISOgetBufferSub(void) {
	if (subHandle != NULL || subChanMixed) {
		return subbuffer;
	}

	return NULL;
}

long CALLBACK ISOgetStatus(struct CdrStat *stat) {
	u32 sect;
#ifdef PROFILE
	start_section(CDR_SECTION);
#endif
	CDR__getStatus(stat);
	
	if (playing) {
		stat->Status |= 0x80;
	}
	
	// relative -> absolute time
	sect = cddaCurOffset / CD_FRAMESIZE_RAW + 150;
	sec2msf(sect, (u8 *)stat->Time);
	
	// BIOS - boot ID (CD type)
	stat->Type = ti[1].type;
#ifdef PROFILE
	end_section(CDR_SECTION);
#endif
	return 0;
}

// read CDDA sector into buffer
long CALLBACK ISOreadCDDA(unsigned char m, unsigned char s, unsigned char f, unsigned char *buffer) {
	unsigned char msf[3] = {m, s, f};
	unsigned char *p;
#ifdef PROFILE
	start_section(CDR_SECTION);
#endif
	msf[0] = itob(msf[0]);
	msf[1] = itob(msf[1]);
	msf[2] = itob(msf[2]);

	if (ISOreadTrack(msf) != 0) return -1;

	p = ISOgetBuffer();
	if (p == NULL) return -1;

	memcpy(buffer, p - 12, CD_FRAMESIZE_RAW); // copy from the beginning of the sector

	if (cddaBigEndian) {
		int i;
		unsigned char tmp;

		for (i = 0; i < CD_FRAMESIZE_RAW / 2; i++) {
			tmp = buffer[i * 2];
			buffer[i * 2] = buffer[i * 2 + 1];
			buffer[i * 2 + 1] = tmp;
		}
	}
#ifdef PROFILE
	end_section(CDR_SECTION);
#endif
	return 0;
}

void cdrIsoInit(void) {
	CDR_init = ISOinit;
	CDR_shutdown = ISOshutdown;
	CDR_open = ISOopen;
	CDR_close = ISOclose;
	CDR_getTN = ISOgetTN;
	CDR_getTD = ISOgetTD;
	CDR_readTrack = ISOreadTrack;
	CDR_getBuffer = ISOgetBuffer;
	CDR_play = ISOplay;
	CDR_stop = ISOstop;
	CDR_getBufferSub = ISOgetBufferSub;
	CDR_getStatus = ISOgetStatus;
	CDR_readCDDA = ISOreadCDDA;

	CDR_getDriveLetter = CDR__getDriveLetter;
	CDR_configure = CDR__configure;
	CDR_test = CDR__test;
	CDR_about = CDR__about;
	CDR_setfilename = CDR__setfilename;

	numtracks = 0;
}

int cdrIsoActive(void) {
	return (cdHandle != NULL);
}
