/************************************************************************

Copyright mooby 2002
Copyright tehpola 2010

CDRMooby2 CDDAData.cpp
http://mooby.psxfanatics.com

  This file is protected by the GNU GPL which should be included with
  the source code distribution.

************************************************************************/

#ifdef WINDOWS
#pragma warning(disable:4786)
#endif

#include "CDDAData.hpp"
#include "Preferences.hpp"

using namespace std;

extern Preferences prefs;
extern std::string programName;


#ifdef __GAMECUBE__

static sem_t audioReady;

static void AudioReady(s32 voice){
	LWP_SemPost(audioReady);
}

static void* CDDAThread(void* userData){
#if 0
  PlayCDDAData* data = static_cast<PlayCDDAData*>(userData);
	
	while(data->live){
		// Wait for the beginning of the first track
		LWP_SemWait(data->firstAudio);
		
		s32 voice = ASND_GetFirstUnusedVoice();
		bool first = true;
		
		while(data->live && data->playing){
			data->theCD->seek(data->CDDAPos);
			void* buffer = data->theCD->getBuffer();
			
			// Wait for the sound system to request more audio
			LWP_SemWait(audioReady);
			
			if(first){
				s32 vol = static_cast<s32>(data->volume * MAX_VOLUME);
				// Set up the audio stream for the first time
				ASND_SetVoice(voice, VOICE_STEREO_16BIT, 44100, 0,
				              buffer, bytesPerFrame, vol, vol, AudioReady);
				ASND_Pause(0);
				first = false;
			} else {
				ASND_AddVoice(voice, buffer, bytesPerFrame);
			}
			
			data->CDDAPos += CDTime(0,0,1);
			
			if(data->CDDAPos == data->CDDAEnd){
				if(data->repeat){
					data->CDDAPos = data->CDDAStart;
				} else {
					data->endOfTrack = true;
					break;
				}
			}
		}
		
		ASND_StopVoice(voice);
	}
#endif
	return NULL;
}
#endif // __GAMECUBE__

// this callback repeats one track over and over
int CDDACallbackRepeat(  void *inputBuffer, void *outputBuffer,
                     unsigned long framesPerBuffer,
                     /*PaTimestamp outTime,*/ void *userData )
{
   unsigned int i;
/* Cast data passed through stream to our structure type. */
   PlayCDDAData* data = (PlayCDDAData*)userData;
   short* out = (short*)outputBuffer;
    
   data->theCD->seek(data->CDDAPos);
   short* buffer = (short*)data->theCD->getBuffer();
   
   buffer += data->frameOffset;

   double volume = data->volume;

      // buffer the data
   for( i=0; i<framesPerBuffer; i++ )
   {
    /* Stereo channels are interleaved. */
      *out++ = (short)(*buffer++ * volume);              /* left */
      *out++ = (short)(*buffer++ * volume);             /* right */
      data->frameOffset += 4;

         // at the end of a frame, get the next one
      if (data->frameOffset == bytesPerFrame)
      {
         data->CDDAPos += CDTime(0,0,1);

            // when at the end of this track, loop to the start
            // of this track
         if (data->CDDAPos == data->CDDAEnd)
         {
            data->CDDAPos = data->CDDAStart;
         }

         data->theCD->seek(data->CDDAPos);
         data->frameOffset = 0;
         buffer = (short*)data->theCD->getBuffer();
      }
   }
   return 0;
}

// this callback plays through one track once and stops
int CDDACallbackOneTrackStop(  void *inputBuffer, void *outputBuffer,
                     unsigned long framesPerBuffer,
                     /*PaTimestamp outTime,*/ void *userData )
{
   unsigned int i;
/* Cast data passed through stream to our structure type. */
   PlayCDDAData* data = (PlayCDDAData*)userData;
   short* out = (short*)outputBuffer;
   short* buffer;

      // seek to the current CDDA read position
   if (!data->endOfTrack)
   {
      data->theCD->seek(data->CDDAPos);
      buffer = (short*)data->theCD->getBuffer();
   }
   else
   {
      buffer = (short*)data->nullAudio;
   }

   buffer += data->frameOffset;

   double volume = data->volume;

      // buffer the data
   for( i=0; i<framesPerBuffer; i++ )
   {
    /* Stereo channels are interleaved. */
      *out++ = (short)(*buffer++ * volume);              /* left */
      *out++ = (short)(*buffer++ * volume);             /* right */
      data->frameOffset += 4;

         // at the end of a frame, get the next one
      if (data->frameOffset == bytesPerFrame)
      {
         data->CDDAPos += CDTime(0,0,1);

            // when at the end of this track, use null audio
         if (data->CDDAPos == data->CDDAEnd)
         {
            data->endOfTrack = true;
            buffer = (short*)data->nullAudio;
            data->CDDAPos -= CDTime(0,0,1);
            data->frameOffset = 0;
         }
            // not at end of track, just do normal buffering
         else
         {
            data->theCD->seek(data->CDDAPos);
            data->frameOffset = 0;
            buffer = (short*)data->theCD->getBuffer();
         }
      }
   }
   return 0;
}

PlayCDDAData::PlayCDDAData(const std::vector<TrackInfo> ti, CDTime gapLength) 
   : stream(NULL), 
     frameOffset(0), theCD(NULL), trackList(ti), playing(false),
     repeat(false), endOfTrack(false), pregapLength(gapLength)
{
   memset(nullAudio, 0, sizeof(nullAudio));
   volume = atof(prefs.prefsMap[volumeString].c_str());
   if (volume < 0) volume = 0;
   else if (volume > 1) volume = 1;
   
#ifdef __GAMECUBE__
   live = true;
   LWP_SemInit(&audioReady, 1, 1);
   LWP_SemInit(&firstAudio, 0, 1);
   LWP_CreateThread(&audioThread, CDDAThread, this,
                    audioStack, audioStackSize, audioPriority);
#endif
}

PlayCDDAData::~PlayCDDAData()
{
	if (playing) stop(); 
#ifdef WINDOWS
	Pa_Terminate();
#elif defined(__GAMECUBE__)
	live = false;
	LWP_SemPost(firstAudio);
	LWP_SemDestroy(audioReady);
	LWP_SemDestroy(firstAudio);
	LWP_JoinThread(audioThread, NULL);
#endif
	delete theCD;
}

// initialize the CDDA file data and initalize the audio stream
void PlayCDDAData::openFile(const std::string& file, int type) 
{
#ifdef WINDOWS
   PaError err;
#endif
   std::string extension;
   theCD = FileInterfaceFactory(file, extension, type);
   theCD->setPregap(pregapLength, trackList[2].trackStart);
#ifdef WINDOWS
   err = Pa_Initialize();
   if( err != paNoError )
   {
      Exception e(string("PA Init error: ") + string(Pa_GetErrorText( err )));
      THROW(e);
   }
#endif
      // disable extra caching on the file interface
   theCD->setCacheMode(FileInterface::oldMode);
}
   
// start playing the data
int PlayCDDAData::play(const CDTime& startTime)
{
   CDTime localStartTime = startTime;
   
      // if play was called with the same time as the previous call,
      // dont restart it.  this fixes a problem with FPSE's play call.
      // of course, if play is called with a different track, 
      // stop playing the current stream.
   if (playing)
   {
      if (startTime == InitialTime)
      {
         return 0;
      }
      else
      {
         stop();
      }
   }

   InitialTime = startTime;

   // make sure there's a valid option chosen
   if ((prefs.prefsMap[repeatString] != repeatOneString) &&
       (prefs.prefsMap[repeatString] != repeatAllString) &&
       (prefs.prefsMap[repeatString] != playOneString))
   {
      prefs.prefsMap[repeatString] = repeatAllString;
      prefs.write();
   }

      // figure out which track to play to set the end time
   if ( (prefs.prefsMap[repeatString] == repeatOneString) ||
        (prefs.prefsMap[repeatString] == playOneString))
   {
      unsigned int i = 1;
      while ( (i < (trackList.size() - 1)) && (startTime > trackList[i].trackStart) )
      {
         i++;
      }
         // adjust the start time if it's blatantly off from the start time...
      if (localStartTime > trackList[i].trackStart)
      {
         if ( (localStartTime - trackList[i].trackStart) > CDTime(0,2,0))
         {
            localStartTime = trackList[i].trackStart;
         }
      }
      else
      {
         if ( (trackList[i].trackStart - localStartTime) > CDTime(0,2,0))
         {
            localStartTime = trackList[i].trackStart;
         }
      }
      CDDAStart = localStartTime;
      CDDAEnd = trackList[i].trackStart + trackList[i].trackLength;
   }

   else if (prefs.prefsMap[repeatString] == repeatAllString)
   {
      CDDAEnd = trackList[trackList.size() - 1].trackStart +
         trackList[trackList.size() - 1].trackLength;
      CDDAStart = trackList[2].trackStart;
      if (localStartTime > CDDAEnd)
      {
         localStartTime = CDDAStart;
      }
   }

         // set the cdda position, start and end times
   CDDAPos = localStartTime;

   endOfTrack = false;
   
   playing = true;

      // open a stream - pass in this CDDA object as the user data.
      // depending on the play mode, use a different callback
#ifdef WINDOWS
   PaError err;
   
   if (prefs.prefsMap[repeatString] == repeatAllString)
      err = Pa_OpenDefaultStream(&stream, 0, 2, paInt16, 44100, 5880, 
                                 0, CDDACallbackRepeat, this);
   else if (prefs.prefsMap[repeatString] == repeatOneString)
      err = Pa_OpenDefaultStream(&stream, 0, 2, paInt16, 44100, 5880, 
                                 0, CDDACallbackRepeat, this);
   else if (prefs.prefsMap[repeatString] == playOneString)
      err = Pa_OpenDefaultStream(&stream, 0, 2, paInt16, 44100, 5880, 
                                 0, CDDACallbackOneTrackStop, this);

   if( err != paNoError )
   {
     return 0;
   }
  
      // start the stream.  the CDDACallback will automatically get 
      // called to buffer the audio
   err = Pa_StartStream( stream );

   if( err != paNoError )
   {
     return 0;
   }
#elif (__GAMECUBE__)
   LWP_SemPost(firstAudio);
#endif
   
   return 0;
}

// close the stream - nice and simple
int PlayCDDAData::stop()
{
   if (playing)
   {
#ifdef WINDOWS
      PaError err = Pa_CloseStream( stream );
      if( err != paNoError )
      {  
         Exception e(string("PA Close Stream error: ") + string(Pa_GetErrorText( err )));
         THROW(e);
      }
#endif
      playing = false;
   }
   return 0;
}

