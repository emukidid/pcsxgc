#include <stdio.h>
#include <math.h>
#include <gccore.h>
#include <ogcsys.h>
#include <stdlib.h>
#include <fat.h>
#include <unistd.h>
#include <sys/dir.h>

#include <string.h>
#ifdef USE_GUI
#include "../gui/GUI.h"
#define MAXLINES 16
#define PRINT GUI_print
#else
#define MAXLINES 30
#define PRINT printf
#endif

#ifdef USE_GUI
#define CLEAR() GUI_clear()
#else
#define CLEAR() printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n")
#endif

static char buffer[96];
typedef struct {
	char name[MAXPATHLEN];
	int  size;
	int  attr;
} dir_ent;

char* textFileBrowser(char* directory){
	// Set everything up to read
	DIR_ITER* dp = diropen(directory);
	if(!dp){ return NULL; }
	struct stat fstat;
	char filename[MAXPATHLEN];
	int num_entries = 2, i = 0;
	dir_ent* dir = malloc( num_entries * sizeof(dir_ent) );
	// Read each entry of the directory
	while( dirnext(dp, filename, &fstat) == 0 ){
		// Make sure we have room for this one
		if(i == num_entries){
			++num_entries;
			dir = realloc( dir, num_entries * sizeof(dir_ent) ); 
		}
		strcpy(dir[i].name, filename);
		dir[i].size   = fstat.st_size;
		dir[i].attr   = fstat.st_mode;
		++i;
	}
	
	dirclose(dp);
	
	int currentSelection = (num_entries > 2) ? 2 : 1;
	while(1){
		CLEAR();
		sprintf(buffer, "browsing %s:\n\n", directory);
		PRINT(buffer);
		int i = MIN(MAX(0,currentSelection-7),MAX(0,num_entries-14));
		int max = MIN(num_entries, MAX(currentSelection+7,14));
		for(; i<max; ++i){
			if(i == currentSelection)
				sprintf(buffer, "*");
			else    sprintf(buffer, " ");
			sprintf(buffer, "%s\t%-32s\t%s\n", buffer,
			        dir[i].name, (dir[i].attr&S_IFDIR) ? "DIR" : "");
			PRINT(buffer);
		}
		
		/*** Wait for A/up/down press ***/
		while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN));
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_UP)   currentSelection = (--currentSelection < 0) ? num_entries-1 : currentSelection;
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) currentSelection = (currentSelection + 1) % num_entries;
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_A){
			if(dir[currentSelection].attr & S_IFDIR){
				char newDir[MAXPATHLEN];
				sprintf(newDir, "%s/%s", directory, dir[currentSelection].name);
				free(dir);
				CLEAR();
				sprintf(buffer,"MOVING TO %s.\nPress B to continue.\n",newDir);
				PRINT(buffer);
				while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_B));
				return textFileBrowser(newDir);
			} else {
				char* newDir = malloc(MAXPATHLEN);
				sprintf(newDir, "%s/%s", directory, dir[currentSelection].name);
				free(dir);
				CLEAR();
				sprintf(buffer,"SELECTING %s.\nPress B to continue.\n",newDir);
				PRINT(buffer);
				while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_B));
				return newDir;
			}
		}
		/*** Wait for up/down button release ***/
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN)));
	}
}


