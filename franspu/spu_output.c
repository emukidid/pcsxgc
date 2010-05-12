#if 0
#include "franspu.h"
#include <minimal.h>
#include <pthread.h>

#ifndef NOSOUND
#define OSS_MODE_STEREO	1
#define OSS_MODE_MONO	0
#define OSS_SPEED_44100	44100
static int oss_audio_fd = -1;

/* Sound Thread variables */
#define DEFAULT_SAMPLE_NUM_BUFF 16 								// Number of sound buffers
pthread_t gp2x_sound_thread=0;									// Thread for gp2x_sound_thread_play()
volatile int gp2x_sound_thread_exit=0;								// Flag to end gp2x_sound_thread_play() thread
volatile int gp2x_sound_buffer=0;								// Current sound buffer
short gp2x_sound_buffers[32768*DEFAULT_SAMPLE_NUM_BUFF];					// Sound buffers
volatile int sndlen=32768;									// Current sound buffer length
#endif

/* Sound Thread*/
static void *gp2x_sound_thread_play(void *none)
{
#ifndef NOSOUND

	int nbuff=gp2x_sound_buffer;								// Number of the sound buffer to play
	do {
		write(oss_audio_fd, (const void *)&gp2x_sound_buffers[32768*nbuff], sndlen); 	// Play the sound buffer
		ioctl(oss_audio_fd, SOUND_PCM_SYNC, 0); 					// Synchronize Audio
		if (nbuff!=gp2x_sound_buffer) {							// Try to follow the write sound buffer
			nbuff++; if (nbuff==DEFAULT_SAMPLE_NUM_BUFF) nbuff=0;			// Increase the sound buffer to play
		}
	} while(!gp2x_sound_thread_exit);							// Until the end of the sound thread
#endif
	return NULL;
}

/* Mute Sound Thread*/
void gp2x_sound_thread_mute(void)
{
#ifndef NOSOUND

	memset(gp2x_sound_buffers,0,32768*DEFAULT_SAMPLE_NUM_BUFF*2);
	sndlen=32768;
#endif
}

/* Start Sound Core */
void SetupSound(void)
{
#ifndef NOSOUND
 	int pspeed=44100;
 	int pstereo;
 	int format;
 	int fragsize = 0;
 	int myfrag;
 	int oss_speed, oss_stereo;
 	int L, R;

	pstereo=OSS_MODE_MONO;

 	oss_speed = pspeed;
 	oss_stereo = pstereo;

 	if((oss_audio_fd=open("/dev/dsp",O_WRONLY,0))==-1)
  	{
   		printf("Sound device not available!\n");
   		return;
  	}

 	if(ioctl(oss_audio_fd,SNDCTL_DSP_RESET,0)==-1)
  	{
   		printf("Sound reset failed\n");
   		return;
  	}

 	format = AFMT_S16_LE;
 	if(ioctl(oss_audio_fd,SNDCTL_DSP_SETFMT,&format) == -1)
  	{
   		printf("Sound format not supported!\n");
   		return;
  	}

 	if(format!=AFMT_S16_LE)
  	{
   		printf("Sound format not supported!\n");
   		return;
  	}

 	if(ioctl(oss_audio_fd,SNDCTL_DSP_STEREO,&oss_stereo)==-1)
  	{
   		printf("Stereo mode not supported!\n");
   		return;
  	}

 	if(ioctl(oss_audio_fd,SNDCTL_DSP_SPEED,&oss_speed)==-1)
  	{
   		printf("Sound frequency not supported\n");
   		return;
  	}
 	if(oss_speed!=pspeed)
 	{
   		printf("Sound frequency not supported\n");
   		return;
  	}

 	L=(((100*0x50)/100)<<8)|((100*0x50)/100);          
 	ioctl(oss_audio_fd, SOUND_MIXER_WRITE_PCM, &L); 

	/* Initialize sound thread */
	gp2x_sound_thread_mute();
	gp2x_sound_thread_exit=0;
	pthread_create( &gp2x_sound_thread, NULL, gp2x_sound_thread_play, NULL);

#endif
}

/* Stop Sound Core */
void RemoveSound(void)
{
#ifndef NOSOUND
 	gp2x_sound_thread_exit=1;
 	gp2x_timer_delay(1000L);
 	if(oss_audio_fd != -1 )
  	{
   		close(oss_audio_fd);
   		oss_audio_fd = -1;
  	}
#endif
}

/* Feed Sound Data */
void SoundFeedStreamData(unsigned char* pSound,long lBytes)
{
#ifndef NOSOUND
 	if(oss_audio_fd == -1) return;
	int nbuff=gp2x_sound_buffer; nbuff++; if (nbuff==DEFAULT_SAMPLE_NUM_BUFF) nbuff=0;	// Number of the sound buffer to write
	memcpy(&gp2x_sound_buffers[32768*nbuff],pSound,lBytes);					// Write the sound buffer
	sndlen=lBytes;										// Update the sound buffer length
	gp2x_sound_buffer=nbuff;								// Update the current sound buffer
#endif
}
#endif
