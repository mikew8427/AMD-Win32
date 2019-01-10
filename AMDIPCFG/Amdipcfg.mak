# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=AMDIPCFG - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to AMDIPCFG - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "AMDIPCFG - Win32 Release" && "$(CFG)" !=\
 "AMDIPCFG - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "Amdipcfg.mak" CFG="AMDIPCFG - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "AMDIPCFG - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "AMDIPCFG - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "AMDIPCFG - Win32 Debug"
RSC=rc.exe
CPP=cl.exe

!IF  "$(CFG)" == "AMDIPCFG - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\AMDIPCFG.EXE"

CLEAN : 
	-@erase "..\..\AMD_W95\AMDIPCFG.EXE"
	-@erase ".\Release\W32trace.obj"
	-@erase ".\Release\amdipcfg.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GX /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "AMD_WIN95" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D\
 "AMD_WIN95" /Fp"$(INTDIR)/Amdipcfg.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Amdipcfg.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"C:\AMD_W95\AMDIPCFG.EXE"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/AMDIPCFG.pdb" /machine:I386 /out:"C:\AMD_W95\AMDIPCFG.EXE" 
LINK32_OBJS= \
	"$(INTDIR)/W32trace.obj" \
	"$(INTDIR)/amdipcfg.obj"

"$(OUTDIR)\AMDIPCFG.EXE" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "AMDIPCFG - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\amdipcfg.exe"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\vc40.idb"
	-@erase "..\amdipcfg.exe"
	-@erase ".\Debug\W32trace.obj"
	-@erase ".\Debug\amdipcfg.obj"
	-@erase "..\amdipcfg.ilk"
	-@erase ".\Debug\amdipcfg.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "AMD_WIN95" /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE"\
 /D "AMD_WIN95" /Fp"$(INTDIR)/Amdipcfg.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/"\
 /c 
CPP_OBJS=.\Debug/
CPP_SBRS=
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Amdipcfg.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"C:\test\amdipcfg.exe"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/amdipcfg.pdb" /debug /machine:I386 /out:"C:\test\amdipcfg.exe" 
LINK32_OBJS= \
	"$(INTDIR)/W32trace.obj" \
	"$(INTDIR)/amdipcfg.obj"

"$(OUTDIR)\amdipcfg.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "AMDIPCFG - Win32 Release"
# Name "AMDIPCFG - Win32 Debug"

!IF  "$(CFG)" == "AMDIPCFG - Win32 Release"

!ELSEIF  "$(CFG)" == "AMDIPCFG - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\W32trace.c
DEP_CPP_W32TR=\
	".\schglobal.h"\
	

!IF  "$(CFG)" == "AMDIPCFG - Win32 Release"


"$(INTDIR)\W32trace.obj" : $(SOURCE) $(DEP_CPP_W32TR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "AMDIPCFG - Win32 Debug"


"$(INTDIR)\W32trace.obj" : $(SOURCE) $(DEP_CPP_W32TR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\amdipcfg.c
DEP_CPP_AMDIP=\
	".\w32trace.h"\
	

"$(INTDIR)\amdipcfg.obj" : $(SOURCE) $(DEP_CPP_AMDIP) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
