I've placed lu_zero's linux SDL/FPSE Makefiles here,
because they didn't work on my system :(
if you want to use them, copy Makedep, Makefile and 
Makefile.nodep into the src directory.

- Pete

===================================================

I fixed the mk.x11 so is possible to use it to compile the x11,
next step will be setting up a configure script.

- lu

===================================================

Also yokota's mingw changes can be found in this directory, check out the "mingw*.patch" file.

New in the gpu114 patch:

* changed macro definitions _MINGW into __MINGW32__.
  (gpuPeopsSoft.rc, stdafx.h, mk.mgw)
* added strip command into link option. (src\makes\mk.mgw)
* changed a newline position. (src\Makefile)


A tiny compile memo is here:

-----
[Requirements]
* MinGW-2.0.0-3.exe (http://mingw.sourceforge.net)
* UnxUtils.zip (http://unxutils.sourceforge.net)
* brcc32.exe, rw32core.dll (BCC 5.5: http://www.borland.com)
* nasm-0.98.35-win32.zip (http://nasm.sourceforge.net)
* afxres.h 
 (http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/ffdshow/ffvfw/src/cygwin/afxres.h)
* dx70_mgw.zip (http://alleg.sourceforge.net)
* vfw.h, msacm.h, mmreg.h (BCC 5.5: http://www.borland.com)
* libvfw_avi32.a, libvfw_cap32.a, libvfw_ms32.a 
 (multilib.zip: http://mywebpage.netscape.com/PtrPck/multimedia.htm)

[Build]
Open command prompt on src dir, and type as this: "set 
PATH=C:\MinGW\bin;%PATH%;", "set INCLUDE=C:\MinGW\include", "make 
(option)".

Or, create "setgcc.bat" on system dir and fill it like below. And open 
command prompt on src dir and type as this: "setgcc", "make (option)".
	prompt $e[;37;1;47m$p$g
	cls
	set PATH=C:\MinGW\bin;%PATH%;
	set INCLUDE=C:\MinGW\include
	doskey /insert
-----

- yokota

===================================================