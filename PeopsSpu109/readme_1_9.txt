P.E.Op.S. PSX Sound emulation plugin
---------------------------------------------------------------------------

The P.E.Op.S. spu plugins are based on Pete's DSound/OSS plugins
for Windows and Linux. It is compatible with PSX emus (using the standard
psx emu plugin interface), as well as the ZiNc emu (for playing the qsound
data and the psx spu sounds used in some games).

---------------------------------------------------------------------------

Windows requirements/installation:
----------------------------------

DirectSound, and a sound card which can do 44100 stereo/mono
16 Bit sample playing.

And of course a fast enuff pc to run the psx games at the
"right" speed (50 or 60 fps). That's very important for
XA audio, otherwise there will be cracks and/or noise.
Use the "fps limitation" option of the gpu plugin to
play your games at the right speed!

Installation:
Just copy the file spuPeopsDSound.dll into your \plugin(s) directory, 
(in FPSE \plugin\psemu), that's all.

Linux requirements/installation:
--------------------------------

Well, if the PSX emu has its own "plugin" directory, you 
simply have to copy the file "libspuPeopsOSS.so.x.x.x" into it :)
The files "spuPeopsOSS.cfg" and "cfgPeopsOSS" should be 
copied together into another directory:

There's a text file called "spuPeopsOSS.cfg" and an executable 
called "cfgPeopsOSS". Copy both of them together into

a) a main emu sub-directory named "cfg" (recommended) or
b) directly into the main emu directory or
c) your home directory

If the main emu has a gui to select the plugins, there will 
be propably a "Configure" button for the plugin. Use it... a 
window with all possible options will appear. The options are 
stored into the "spuPeopsOSS.cfg" text file.

So, if there is no gui, you can use a simple text editor to 
change the options.

ZiNc requirements/installation:
-------------------------------

For ZiNc, copy the DLL into the main emu's directory and rename 
it, for example to "sound.znc" for the Windows version of 
ZiNc, or "libsoundznc.so" in Linux). 

Please note that the Windows version of ZiNc has no native GUI, so
if you want to change the spu plugin's configuration (like the reverb
or interpolation settings), you will need a freeware psx emu (like 
ePSXe) to start up the plugin's config window. Since the configuration
is stored globally in the Windows registry, the same settings will
apply to ZiNc as well.

----------------------------------------------------------------------------

Configuration:

There ain't much options in the spu plugin... only
a rough volume control, an option to turn off XA music and
some misc special options.

I've tried to combine a relative large sound buffer with
a technique to get short reaction times... the result should
be no stuttering when the emu gets very busy (for example
while heavily accessing the cdrom drive), but also no delays
when starting to play a sound sample. Well, at least it
works fine on my system :)

Some advice: the plugin is supporting the spu irq function,
so you should turn off the "SPU irq hack" (or similar option)
in the main emu with most games.

1) General settings
-------------------

Mode:
The mode setting affects how the plugin will feed the sound data
to your sound card. With "SpuAsync" the spu plugin will run 
completely synchron to the main emu (so don't ask me why the mode
is called "async", ehehe), which will usually produce the best 
timing, at the cost that if some other emu component (like the gpu
or cdr part) is consuming much cpu time, the sound will break up.
"Timer event mode": that mode is needed on some systems to fix 
stuttering effects. It also helps with very timing critical games, 
so you should enable the option, if your system is fast enough 
to handle it. This mode is MSWindows only.
The "thread mode will be a little bit faster, and it's still OK 
with most games, so you can use it on slower PCs.
(note: if the plugin is used with ZiNc, it will always use the
"Timer" mode internally, so this option doesn't matter in ZN
games)

Volume:
You can choose between "Low", "Medium", "Loud" or "Loudest"
depending on your speakers.
Of couse you can also adjust the volume with the Windows/Linux
sound mixer ;P
(note: if the plugin is used with ZiNc, it will always use the
"Loudest" mode internally, otherwise the ZN qsound music would
be at a wrong volume)

Reverb: 
There are three different settings: no reverb (fastest mode,
but the sound will be very thin sometimes), fake reverb (easy reverb
mode) and psx reverb (fully emulated psx reverb effects - thanx to
Neill Corlett). Choose whatever you like best.

Interpolation:
Four different modes: "no interplation" will be fastest, but
the sound can have some scratching effects. "Simple interpolation" will
smooth the sound somewhat, still the sound will be thin (or, some 
people claim: "clean"). The "gaussian interpolation" (added by kode54)
will prolly be the best choice for most users. But also the "cubic" mode
(added by Eric) is very nice, it offers a better treble than the gaussian
one (imho).


2.) XA Audio
-------------------

If a game needs to play XA for compatibility, but for some reasons
it sounds too bad, you can turn it off... yup. It's _not_ the same 
option as  the one in the main emulator: you can just disable 
the XA audio output! If XA is enabled in the main emu, XA audio data 
will still be decoded... you just can't hear it anymore.

As mentioned, the XA decoding is done by the main emu, so if 
there are bugs (unwanted noise even if the game speed is right),
send a mail to the main emu team, not to my mail address ;)

If you are playing XA at the wrong speed (not using the
fps limitation of the gpu plugin), you will get some noise,
because XA will get cut off. Or you enable the "change XA speed" 
option, that way all XA sounds will get played, but at a faster
speed... if the fps is not constant, the sound will get very weird,
though.

(note: if the plugin is used with ZiNc, the XA settings will not 
matter at all)

3.) Misc
-------------------

- "SPU IRQ - wait for CPU action": an option for playing 
  Metal Gear Solid and Valkyrie Profile... you also have to enable
  the "High compatibility mode" with that game.
  Ususally you can leave that option enabled with all games,
  only a few ones (like Chrono Cross or Legend of Mana) will
  produce sound glitches.
  (note: ZiNc doesn't care for this option)

- "SPU IRQ - handle irqs in decoded sound buffer areas": well,
  it seems that psx spu irqs can be raised when you sneeze loudly
  near you console ;) Anyway, this option will take care when 
  the spu irq address is located in one of the decoded sound buffers.
  ePSXe handles this (at least with some games) by itself, but other
  emus may need this option enabled. Only known game using such
  IRQ handling: Crash Team Racing. Maybe you will find another one,
  though... if a game doesn't work (right), try toggling this option.

- "Mono sound" mode: if enabled, all sound output will be mono. Doesn't
  sound that great, of course, but it can be helpful on slower systems/
  sound cards, since only half of the usual data has to get transfered.
  (note: with ZiNc no "mono" mode is supported)

- "Developer debug mode" (MS Windows only): a window will pop up 
  when the spu gets started, showing the current spu usage of 
  the game.
  I have enabled that mode for interested users, but keep in
  mind that the debug mode will slow things a bit down, so
  prolly you will need a fast cpu and video card. Also don't use
  the debug window in a fullscreen window mode, and make
  sure that your desktop has at least a 800x600 resolution.
  Most useful is the debug window if you are trying to improve
  the plugin yourself :)
  (note: with ZiNc, ah yes, you prolly can finish this note youself...)

- "Sound recording window" (MS Windows only): another small 
  window will be displayed on spu startup (don't try to use it in fullscreen
  mode), you can use it to easily record the psx sounds into standard
  WAV sound files.

----------------------------------------------------------------------------

For version infos read the "version.txt" file.

And, peops, have fun!

Pete Bernert

----------------------------------------------------------------------------

P.E.Op.S. page on sourceforge: https://sourceforge.net/projects/peops/

P.E.Op.S. spu developers:

Pete Bernert	http://www.pbernert.com
linuzappz	http://www.pcsx.net
kode54		http://home.earthlink.net/~kode54/

----------------------------------------------------------------------------

Disclaimer/Licence:

This plugin is under GPL... check out the license.txt file in the /src
directory for details.

----------------------------------------------------------------------------
 