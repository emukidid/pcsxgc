# Microsoft Developer Studio Project File - Name="gpuPeopsSoft" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=gpuPeopsSoft - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gpuPeopsSoft.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gpuPeopsSoft.mak" CFG="gpuPeopsSoft - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gpuPeopsSoft - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "gpuPeopsSoft - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "gpuPeopsSoft - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "__i386__" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 user32.lib gdi32.lib ddraw.lib winmm.lib advapi32.lib vfw32.lib d3dx.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy release\gpuPeopsSoft.dll  d:\emus\epsxe\plugins	rem copy release\gpuPeopsSoft.dll d:\emus\zinc\renderer.znc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "gpuPeopsSoft - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib gdi32.lib ddraw.lib dxguid.lib d3dim.lib winmm.lib advapi32.lib vfw32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "gpuPeopsSoft - Win32 Release"
# Name "gpuPeopsSoft - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\cfg.c
# End Source File
# Begin Source File

SOURCE=.\draw.c
# End Source File
# Begin Source File

SOURCE=.\fps.c
# End Source File
# Begin Source File

SOURCE=.\fpsewp.c
# End Source File
# Begin Source File

SOURCE=.\gpu.c
# End Source File
# Begin Source File

SOURCE=.\gpuPeopsSoft.c
# End Source File
# Begin Source File

SOURCE=.\gpuPeopsSoft.def
# End Source File
# Begin Source File

SOURCE=.\hq2x16.asm

!IF  "$(CFG)" == "gpuPeopsSoft - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\hq2x16.asm
InputName=hq2x16

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	$(MSVCDIR)\bin\nasmw.exe -O9999 -f win32 -D__WIN32__ -D__i386__ hq2x16.asm -o "$(OutDir)"\hq2x16.obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gpuPeopsSoft - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hq2x32.asm

!IF  "$(CFG)" == "gpuPeopsSoft - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\hq2x32.asm
InputName=hq2x32

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	$(MSVCDIR)\bin\nasmw.exe -O9999 -f win32 -D__WIN32__ -D__i386__ hq2x32.asm -o "$(OutDir)"\hq2x32.obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gpuPeopsSoft - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hq3x16.asm

!IF  "$(CFG)" == "gpuPeopsSoft - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\hq3x16.asm
InputName=hq3x16

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	$(MSVCDIR)\bin\nasmw.exe -O9999 -f win32 -D__WIN32__ -D__i386__ hq3x16.asm -o "$(OutDir)"\hq3x16.obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gpuPeopsSoft - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hq3x32.asm

!IF  "$(CFG)" == "gpuPeopsSoft - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\hq3x32.asm
InputName=hq3x32

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	$(MSVCDIR)\bin\nasmw.exe -O9999 -f win32 -D__WIN32__ -D__i386__ hq3x32.asm -o "$(OutDir)"\hq3x32.obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gpuPeopsSoft - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\i386.asm

!IF  "$(CFG)" == "gpuPeopsSoft - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=.\i386.asm
InputName=i386

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	$(MSVCDIR)\bin\nasmw.exe -f win32 -D__WIN32__ -D__i386__ $(InputPath) -o $(OutDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gpuPeopsSoft - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\key.c
# End Source File
# Begin Source File

SOURCE=.\menu.c
# End Source File
# Begin Source File

SOURCE=.\prim.c
# End Source File
# Begin Source File

SOURCE=.\record.c
# End Source File
# Begin Source File

SOURCE=.\soft.c
# End Source File
# Begin Source File

SOURCE=.\zn.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\cfg.h
# End Source File
# Begin Source File

SOURCE=.\draw.h
# End Source File
# Begin Source File

SOURCE=.\externals.h
# End Source File
# Begin Source File

SOURCE=.\fps.h
# End Source File
# Begin Source File

SOURCE=.\fpsewp.h
# End Source File
# Begin Source File

SOURCE=.\gpu.h
# End Source File
# Begin Source File

SOURCE=.\key.h
# End Source File
# Begin Source File

SOURCE=.\macros.inc
# End Source File
# Begin Source File

SOURCE=.\menu.h
# End Source File
# Begin Source File

SOURCE=.\prim.h
# End Source File
# Begin Source File

SOURCE=.\psemu.h
# End Source File
# Begin Source File

SOURCE=.\record.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\soft.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\gpu.bmp
# End Source File
# Begin Source File

SOURCE=.\gpuPeopsSoft.rc
# End Source File
# Begin Source File

SOURCE=.\res\gpuPeopsSoft.rc2
# End Source File
# End Group
# Begin Group "Documentation Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\changelog.txt
# End Source File
# Begin Source File

SOURCE=.\filemap.txt
# End Source File
# Begin Source File

SOURCE=.\license.txt
# End Source File
# End Group
# End Target
# End Project
