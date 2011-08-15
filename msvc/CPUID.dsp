# Microsoft Developer Studio Project File - Name="CPUID" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=CPUID - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CPUID.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CPUID.mak" CFG="CPUID - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CPUID - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "CPUID - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CPUID - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /Gm /GX /Zi /O2 /I "." /I ".." /I "..\getopt" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FR /Yu"prefix.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 winmm.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "CPUID - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "." /I ".." /I "..\getopt" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /Yu"prefix.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "CPUID - Win32 Release"
# Name "CPUID - Win32 Debug"
# Begin Source File

SOURCE=..\cache.c
# End Source File
# Begin Source File

SOURCE=..\cache.h
# End Source File
# Begin Source File

SOURCE=..\cpuid.c
# End Source File
# Begin Source File

SOURCE=..\cpuid.h
# End Source File
# Begin Source File

SOURCE=..\feature.c
# End Source File
# Begin Source File

SOURCE=..\feature.h
# End Source File
# Begin Source File

SOURCE=..\getopt\getopt.h
# End Source File
# Begin Source File

SOURCE=..\getopt\getopt_long.c
# End Source File
# Begin Source File

SOURCE=..\handlers.c
# End Source File
# Begin Source File

SOURCE=..\handlers.h
# End Source File
# Begin Source File

SOURCE=..\main.c
# End Source File
# Begin Source File

SOURCE=..\platform.h
# End Source File
# Begin Source File

SOURCE=..\msvc\prefix.c
# ADD CPP /Yc"prefix.h"
# End Source File
# Begin Source File

SOURCE=..\prefix.h
# End Source File
# Begin Source File

SOURCE=..\sanity.c
# End Source File
# Begin Source File

SOURCE=..\sanity.h
# End Source File
# Begin Source File

SOURCE=..\state.h
# End Source File
# Begin Source File

SOURCE=..\msvc\stdint.h
# End Source File
# Begin Source File

SOURCE=..\threads.c
# End Source File
# Begin Source File

SOURCE=..\threads.h
# End Source File
# Begin Source File

SOURCE=..\util.c
# End Source File
# Begin Source File

SOURCE=..\util.h
# End Source File
# Begin Source File

SOURCE=..\vendor.h
# End Source File
# Begin Source File

SOURCE=..\version.c
# End Source File
# Begin Source File

SOURCE=..\version.h
# End Source File
# End Target
# End Project
