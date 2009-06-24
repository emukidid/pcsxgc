P.E.Op.S. Open Source Soft driver for freeware PSX emus/ZN emus
---------------------------------------------------------------
 
The P.E.Op.S. soft gpu plugin is based on Pete's soft gpus
for Windows and Linux.

----------------------------------------------------------------------------

Requirements:

Windows:
--------
Any 2 MB video card which can handle DirectDraw with 16 bit color
depth should work fine... only 16 and 32 bit color depths are 
supported, if you are using a 24 bit desktop, switch to a fullscreen
mode because in Window mode you will get funny colors.

Note: some (older) gfx cards can't handle the 16 bit mode properly
(screwed display), you have to use the 32 bit mode if that's happening.

Linux:
------
The X enviroment is needed, and a gfx card which can handle 15, 16
or 32 bit color depths. Version 1.4 is linked statically with 
libXxf86vm.a to change the video mode in fullscreen displays.
If that's causing problems on your system, simply edit the Makefile
and build a version without video mode changing support by running
"make".
Also, there is a special FPSE linux version, which is needing the
SDL libraries. To build that version, use the makefiles in the
"makes" sub-directory.
Don't try to use the SDL version with ePSXe or PCSX!
You can also compile a ePSXe/PCSX Linux DGA2 version of the soft 
plugin, just take a look at the makefiles :)

BeOS:
-----
It's enough to have SDL 1.2.x installed.

----------------------------------------------------------------------------

Installation:

Windows: copy the file "gpuPeopsSoft.dll" into the main emu 
plugins directory.

Linux: copy the file "libgpuPeopsSoftX.so.1.0.x" or the file 
"libgpuPeopsSDL.so" (FPSE SDL version) into the main emu plugins 
directory. Copy the file "gpuPeopsSoftX.cfg" into the main
emu "cfg" directory, or into the main emu directory, or into your
home directory.

Notes for use with ZiNc:
You have to use the "--renderer=..." command line switch to load 
the plugin (or rename the plugin DLL to "renderer.znc" and copy it
into the main ZiNc directory). For configuration you can specify 
the Peops .cfg with the ZiNc "--use-renderer-cfg-file=..." command
line switch, or (Windows only) simply don't specify a .cfg file, 
then automatically the settings in the Windows registry will be used.
Of course that will mean that you will need some Win psx emu to 
configure the plugin.

----------------------------------------------------------------------------

Configuration:

The SoftGPU has not much options... because it doesn't need them :)

0) Rendering device
--------------------
If you have more than one gfx card installed (I have for example
a GeForce and Voodoo3 PCI), you can select the card you want to use
with the "Select device" button.
Note: that option is not available in the Linux config file.

1) Resolution & Colors
-----------------------
Well, that section should be self-explaining.
Please note: some cards (like the 3dfx V2/V3) can't do a 32 bit
color depth, or rendering in a window (V2). Selecting an option
which your card can't handle will cause error message boxes or
crashes or something like that... yup, sorry

If you want speed (who doesn't): the 16 bit color modes are faster 
than the 32 bit ones... and with this SoftGpu you will _not_ get
a better image quality by choosing 32 Bit modes, so I really suggest
to use 16 bit colors.

Enabling "unstreched display" will show the unstreched psx display
centered inside the window.

Also available: certain 2xSai/Scale2x modes, which will enhance the 
display (mainly in 2D games), at the cost of speed (you will need a 
powerful cpu to use them). 
Note: 16 bit desktop/fullscreen color modes will be faster with 2xSaI, 
so I suggest to not use 32 bit color modes. Big thanx for the 2xSaI 
modes goes to Derek Liauw, the available Scale2x modes are copyrighted
by Andrea Mazzoleni (http://scale2x.sourceforge.net).

Note: in the Linux version you have to take care that you not only
set the fullscreen option, you also have to configure the width/heigth
of the output window in a way that it matches your possible desktop 
sizes. 

Another way to enhance the display is to activate dithering. There are
3 dithering modes avaiable: 

- No dithering:  
  g-shaded polygons will not look very smooth, but it's the fastest mode.
- Game depended: 
  will activate dithering on g-shaded textures, if the game is turning
  on dithering.
- Always:
  will dither all g-shaded polygons, all the time. That one usually is
  looking best, but it's also the slowest mode.        

You can toggle between the three dither modes with the in-game gpu menu
(see below).


2) Framerate limit/Frame skipping
----------------------------------
You can activate FPS limitation if your game is running to fast. You can
use "Auto detect FPS limit", if you are not sure, what limit would be best 
to use. Or you just type in a FPS rate that suits you. PAL games  
use 50 FPS, non-PAL games 60 FPS.
And if things are getting too slow... you can try Frame skipping. 
Tip for best results on slow systems: turn on both...
You can also enable the in-game menu right from the game start
(it's showing the fps and lets you change some gpu options while 
playing).
Of course you still can use the "DEL" key for showing/hiding the menu.
Btw, all gpu hotkeys are described below. If you want to change the 
keys, you can use the small "..." button in the Windows version.
"Transparent FPS display" will just draw the fps menu text, without
a filled box around it.


3) Special game fixes
------------------------
Some gfx glitches are caused by the main emu core or because I've
not found out (yet) how certain things are activated on a real
psx gpu. But you can minimize bad effects with certain games by
using the internal gpu patches.... push the "..." button to see
(and activate) the list of available fixes (Windows/Linux) or set 
the proper bits directly in the config file (Linux).


4) Movie recording (Windows version only)
-----------------------------------------
You can start/stop recording by using the 'recording' hotkey.
A avi file will get created in the \Demo sub-directory.


5) Misc
-----------
Windows only:

"Use system memory" will use your PC main memory as video buffer, not 
the Vram of your gfx card. Result: usually much slower, but the (maybe
unwanted) fullscreen filtering on nVidia cards is disabled.

"Stop screen saver" will disabled screen savers and power management
while running the gpu. A note from syo: W95 and NT4 are not supported
and will likely crash your system.

"Wait vsync" will force the gpu to sync the buffer swaps with the
vsync of your monitor. That will be slower, but it could fix "smearing"
effects. You can define an hotkey to toggle the "Wait vsync option",
if it's activated, the fps menu will display a "V" character.

"Debug mode" will enable several debug keys (currently only one is 
available, though, toggling the display between the 'normal' screen 
and a full vram view). Please note: not all cards will be able to do this mode.


Keys
----
<F8>   save complete psx vram as a bitmap to the 'Snap' sub directory - 
       if you want to mail me a snapshot, please ask before doing 
       it...

<ALT>+<ENTER> switch between window/fullscreen mode (Windows version only, and with most gfx card drivers your desktop color depth need
to be the same as the configured fullscreen color depth)

<NUMPAD *> start/stop avi file recording (Windows version only)

<INSERT> show/hide the gpu version (if no FPS is displayed) or an help text (if the FPS menu is displayed, Windows version only)

<DEL>  show/hide FPS and option menu

  How it works: Hit <DEL> and the Framerate und the menu will appear. It
  looks like: 'FPS XXXX.X   FL< FS DI GF A V Rec'  

  What does it all mean? Here's the legend:

  FPS: frames per second, higher means better :)
  FL : Frame rate limiter
  FS : Frame skipping, higher means faster
  DI : The three available dithering modes
  GF : Game fixes
  A/M: Analog pad mode/PSX Mouse mode... that's an information from the emu core, telling you if the pad emulation is doing Mouse mode ('M'), Analog mode ('A') or Digital mode (no 'A' and no 'M')
  V  : "Wait vsync" is enabled (Windows version only)
  Rec: Avi file recording in progress (Windows version only)

  If a '*' character is beneath an option, the option is active, otherwise
  inactive. 
  There is a '<' sign you can move with the <PAGE UP> or <PAGE DOWN> keys
  towards an option you want to toggle. 
  Just hit the <END>/<HOME> key to switch the selected option on or off. Changes will
  be done immediatly, you can see how the framerate is affected if an option
  is on or off.
  If you are using a manual speed limit, and the in-game menu cursor is set to FL, you
  can adjust the manual limit value by hitting <SHIFT>+ <END>/<HOME>.
  If you have found a setting that suits your game just hide the menu by 
  pressing <DEL> again.
  I don't store changed options permanently, you have to do that still in the
  main configuration dialog.


----------------------------------------------------------------------------

Some tips:

- use a 16 bit desktop color depth if you want to play in Window mode
  or use a 16 bit fullscreen mode to get higher speed
- activating the frame skipping option may be causing glitches...
  turn it off before sending me mails.
- on nVidia cards you will notice blurred (or 'smoothed') screens in 
  the Windows version.
  Well, some people enjoy that, others do not. You can turn off the
  auto-smoothing by disabling hardware accelerated DirectDraw. Go
  to the DirectX applet in your Windows Control Panel, start it and
  select the DirectDraw tab. Here you can turn acceleration off...
  the GPU will become slightly slower, though. And don't forget
  to re-activate the DD option, else you will have troubles to
  start DirectX based PC games. Or simply try the "system memory"
  option...
 
----------------------------------------------------------------------------

For version infos read the "version.txt" file.

And, peops, have fun!

Pete Bernert

----------------------------------------------------------------------------

P.E.Op.S. page on sourceforge: https://sourceforge.net/projects/peops/

P.E.Op.S. developer:

Pete Bernert	http://www.pbernert.com
Lewpy		http://lewpy.psxemu.com/
lu_zero		http://brsk.virtualave.net/lu_zero/
linuzappz	http://www.pcsx.net
Darko Matesic	http://mrdario.tripod.com
syo		http://www.geocities.co.jp/SiliconValley-Bay/2072/

----------------------------------------------------------------------------

Disclaimer/Licence:

This plugin is under GPL... check out the license.txt file in the /src
directory for details.

----------------------------------------------------------------------------
 

