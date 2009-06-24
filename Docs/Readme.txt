 Pcsx - Pc Psx Emulator
 ----------------------

Contents
--------

1) General
2) How it works
3) Supported games
4) TroubleShoot
5) Credits

--------------------------------------------------------------------------------

1) General
   -------

Pcsx is a PSX emulator. What that really means? It means that it emulates the
way that a PSX works and tries to translate PSX machine language to PC language.
That is very hard to be done and we can't speak for 100% success in any time.
The hardware would always be better than the software so if you wanna play games
then better get a real PSX. Pcsx is capable to run most of the games, but a few
others still don't. We are working on it ;). The members of the team are very 
advanced coders and they will continue the work on the Pcsx as far as they have 
enough time to work on it. Real life comes first and sometimes it is much more 
demanding.

 The team of Pcsx

--------------------------------------------------------------------------------

2) How it works
   ------------

Steps:
------
 1) Put a bios on bios directory (recommended: scph1001.bin) (optional)
 2) Put plugins in plugin directory
 3) Open the emu 
 4) Configure plugins from configuration menu and Memcards from Mcd Manager
 5) Restart
 6) Put a cd in your cdrom drive
 7) Press Run CD and the game might work ;P

Quick Keys:
----------
 F1: Save State
 F2: Increase State Num
 F3: Load State
 F4: Show State-Pic
 F5: Sio Irq Dis/Enable
 F6: Black&White Mdecs Dis/Enable
 F7: Dis/Enable Xa
 F8: Makes a Snapshot
 The 2 next keys are only implemented if the cdrom plugin doesn't support
 the CDRgetStatus interface.
 F9: Press to simulate an open case (to change cdroms in game)
 F10: And this to close it

Cpu Options:
-----------
 * Disable Xa Decoding:
    Disables xa sound, if you don't want sound this might
    speed up a little movies (mostly) and the game itself.

 * Sio Irq Always Enabled:
    Enable this only when pads or memcards doesn't works,
    but usually brokes compatibility with them.

 * Spu Irq Always Enabled:
 	May help your game to work, but usually doesn't helps.
 
 * Black & White Movies:
    Speed up for slow machines on movies.

 * Disable CD-DA:
    Will disable CD audio.

 * Enable Console Output:
    Displays the psx text output.

 * Enable Interpreter Cpu:
    Enables interpretive emulation (recomiler by default),
	it may be more compatible, but it's slower.

 * Psx System Type:
    Autodetect: Try to autodetect the system.
    NSTC/PAL: Specify it for yourself.

Internal HLE Bios:
-----------------
 If you select this instead of a regular bios (ie. "scph1001.bin")
 Pcsx will emulate the bios (this might not work in all cases but it is
 quite advanced), you can try it and see which one works better, notice
 that the HLE bios might be faster than a regular one and may make some
 games work better.

NetPlay:
-------
 You must select a plugin in the NetPlay Configuration, note that the plugin
 must support the v2 of the api. If you don't want to use it simply select
 the Disabled one, refer to the plugin's doc for more info.

--------------------------------------------------------------------------------

3) Supported Games
   ---------------

Here is a small list with games that pcsx support. Notice that it might have
some glitches on sound or gfx but it is considered playable. :)

 Crash Bandicoot 1
 Time crisis
 Mickey Wild adventure
 Coolboarders 3
 Street fighter EX+a
 Street fighter EX2 plus
 Breath of fire 3
 Breath of fire 4
 Quake II
 Alone in the Dark 4
 Tekken 3

and probably lots more.
Check www.emufanatics.com or www.ngemu.com for compatibility lists.

------------------------------------------------------------------------------------------------

4) Troubleshoot
   ------------

 -QUE: My favourite game doesn't work
 -AN:  Wait for the next release, or get another emu ;)
 -QUE: Can I have a bios image?
 -AN : No
 -QUE: CD audio is crappy...
 -AN : If it doesn't work then disable it :)

------------------------------------------------------------------------------------------------

5) Credits
   -------

PCSX is the work of the following people:

main coder:
 Linuzappz   , e-mail: linuzappz@pcsx.net
co-coders:
 Shadow      , e-mail: shadow@pcsx.net
ex-coders:
 Pete Bernett, e-mail: psswitch@online.de
 NoComp      , e-mail: NoComp@mailcity.com
 Nik3d       , e-mail:
webmaster:
 Akumax      , e-mail: akumax@pcsx.net

Team would like to thanks:
--------------------------
Ancient   :Shadow's small bother for beta testing pcsx and bothering me to correct it :P
Roor      :for his help on cd-rom decoding routine :)
Duddie,
Tratax,
Kazzuya   :for the great work on psemu,and for the XA decoder :)
Calb      :for nice chats and coding hits ;)  We love you calb ;)
Twin      :for the bot to #pcsx on efnet, for coding hints and and :)
Lewpy     :Man this plugin is quite fast! 
Psychojak :For beta testing Pcsx and donating a great amount of games to Shadow ;)
JNS       :For adding PEC support to Pcsx
Antiloop  :Great coder :)
Segu      :He knows what for :)
Null      :He also knows :)
Bobbi
cdburnout :For putting Pcsx news to their site and for hosting..
D[j]      :For hosting us ;)
Now3d     :For nice chats,info and blabla :)
Ricardo Diaz:Linux version may never have been released without his help
Nuno felicio:Beta testing, suggestions and a portugues readme
Shunt     :Coding hints and nice chats :)
Taka      :Many fixes to Pcsx
jang2k    :Lots of fixes to Pcsx :)

and last but not least:
-----------------------
James   : for all the friendly support
Dimitris: for being good friend to me..
Joan: The woman that make me nuts the last couple months...

------------------------------------------------------------------------------------------------

Official website: http:/www.pcsx.net

If I forgot someone beat me :P
"What you feel is what you are,and what you are is beatiful."
Log off
Shadow/ Linuzappz

