# Microsoft Developer Studio Project File - Name="cdrmooby2" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=cdrmooby2 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cdrmooby2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cdrmooby2.mak" CFG="cdrmooby2 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cdrmooby2 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cdrmooby2 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "cdrmooby2"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cdrmooby2 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CDRMOOBY2_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "$(fltkdir)" /I "./bzip" /I "./zlib" /I "$(portaudiodir)/include" /I "./unrar" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CDRMOOBY2_EXPORTS" /D "SILENT" /D "UNRAR" /D "_USE_ASM" /FD /c
# SUBTRACT CPP /X
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 fltk.lib PAStaticDS.lib dsound.lib winmm.lib wsock32.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:none /machine:I386 /libpath:"$(fltkdir)/lib" /libpath:"$(portaudiodir)/lib"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "cdrmooby2 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CDRMOOBY2_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /G5 /MTd /W3 /Gm /GX /Zi /Od /I "$(fltkdir)" /I "./bzip" /I "./zlib" /I "$(portaudiodir)/include" /I "./unrar" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CDRMOOBY2_EXPORTS" /D "_USE_ASM" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 fltk.lib PAStaticDS.lib dsound.lib winmm.lib wsock32.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /debug /machine:I386 /libpath:"$(fltkdir)\lib" /libpath:"$(fltkdir)/lib" /libpath:"$(portaudiodir)/lib"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "cdrmooby2 - Win32 Release"
# Name "cdrmooby2 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\About.cpp
# End Source File
# Begin Source File

SOURCE=.\CDDAData.cpp
# End Source File
# Begin Source File

SOURCE=.\ConfigCallbacks.cpp
# End Source File
# Begin Source File

SOURCE=.\ConfigFunctions.cpp
# End Source File
# Begin Source File

SOURCE=.\FileInterface.cpp

!IF  "$(CFG)" == "cdrmooby2 - Win32 Release"

# ADD CPP /I "./bzip ./zlib"

!ELSEIF  "$(CFG)" == "cdrmooby2 - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Globals.cpp
# End Source File
# Begin Source File

SOURCE=.\Open.cpp
# End Source File
# Begin Source File

SOURCE=.\Preferences.cpp
# End Source File
# Begin Source File

SOURCE=.\PS2Open.cpp
# End Source File
# Begin Source File

SOURCE=.\SubchannelData.cpp
# End Source File
# Begin Source File

SOURCE=.\TrackParser.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CDDAData.hpp
# End Source File
# Begin Source File

SOURCE=.\CDInterface.hpp
# End Source File
# Begin Source File

SOURCE=.\CDTime.hpp
# End Source File
# Begin Source File

SOURCE=.\ConfigCallbacks.hpp
# End Source File
# Begin Source File

SOURCE=.\ConfigFunctions.hpp
# End Source File
# Begin Source File

SOURCE=.\defines.h
# End Source File
# Begin Source File

SOURCE=.\Exception.hpp
# End Source File
# Begin Source File

SOURCE=.\externs.h
# End Source File
# Begin Source File

SOURCE=.\FileInterface.hpp
# End Source File
# Begin Source File

SOURCE=.\Frame.hpp
# End Source File
# Begin Source File

SOURCE=.\Preferences.hpp
# End Source File
# Begin Source File

SOURCE=.\SubchannelData.hpp
# End Source File
# Begin Source File

SOURCE=.\TimeCache.hpp
# End Source File
# Begin Source File

SOURCE=.\TrackInfo.hpp
# End Source File
# Begin Source File

SOURCE=.\TrackParser.hpp
# End Source File
# Begin Source File

SOURCE=.\Utils.hpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\cdrmooby2.def
# End Source File
# End Group
# Begin Group "bzip"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bzip\blocksort.c
# End Source File
# Begin Source File

SOURCE=.\bzip\bzlib.c
# End Source File
# Begin Source File

SOURCE=.\bzip\bzlib.h
# End Source File
# Begin Source File

SOURCE=.\bzip\bzlib_private.h
# End Source File
# Begin Source File

SOURCE=.\bzip\compress.c
# End Source File
# Begin Source File

SOURCE=.\bzip\crctable.c
# End Source File
# Begin Source File

SOURCE=.\bzip\decompress.c
# End Source File
# Begin Source File

SOURCE=.\bzip\huffman.c
# End Source File
# Begin Source File

SOURCE=.\bzip\randtable.c
# End Source File
# End Group
# Begin Group "zlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\zlib\adler32.c
# End Source File
# Begin Source File

SOURCE=.\zlib\crc32.c
# End Source File
# Begin Source File

SOURCE=.\zlib\deflate.c
# End Source File
# Begin Source File

SOURCE=.\zlib\deflate.h
# End Source File
# Begin Source File

SOURCE=.\zlib\example.c
# End Source File
# Begin Source File

SOURCE=.\zlib\gzio.c
# End Source File
# Begin Source File

SOURCE=.\zlib\infblock.c
# End Source File
# Begin Source File

SOURCE=.\zlib\infblock.h
# End Source File
# Begin Source File

SOURCE=.\zlib\infcodes.c
# End Source File
# Begin Source File

SOURCE=.\zlib\infcodes.h
# End Source File
# Begin Source File

SOURCE=.\zlib\inffast.c
# End Source File
# Begin Source File

SOURCE=.\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=.\zlib\inffixed.h
# End Source File
# Begin Source File

SOURCE=.\zlib\inflate.c
# End Source File
# Begin Source File

SOURCE=.\zlib\inftrees.c
# End Source File
# Begin Source File

SOURCE=.\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=.\zlib\infutil.c
# End Source File
# Begin Source File

SOURCE=.\zlib\infutil.h
# End Source File
# Begin Source File

SOURCE=.\zlib\trees.c
# End Source File
# Begin Source File

SOURCE=.\zlib\trees.h
# End Source File
# Begin Source File

SOURCE=.\zlib\uncompr.c
# End Source File
# Begin Source File

SOURCE=.\zlib\zcompress.c
# End Source File
# Begin Source File

SOURCE=.\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=.\zlib\zlib.h
# End Source File
# Begin Source File

SOURCE=.\zlib\zutil.c
# End Source File
# Begin Source File

SOURCE=.\zlib\zutil.h
# End Source File
# End Group
# Begin Group "unrar"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\unrar\unrarlib.c
# End Source File
# Begin Source File

SOURCE=.\unrar\unrarlib.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\notes.txt
# End Source File
# End Target
# End Project
