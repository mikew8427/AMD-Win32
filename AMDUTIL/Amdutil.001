# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=AMDUTIL - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to AMDUTIL - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "AMDUTIL - Win32 Release" && "$(CFG)" !=\
 "AMDUTIL - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "Amdutil.mak" CFG="AMDUTIL - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "AMDUTIL - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "AMDUTIL - Win32 Debug" (based on "Win32 (x86) Application")
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
# PROP Target_Last_Scanned "AMDUTIL - Win32 Debug"
RSC=rc.exe
MTL=mktyplib.exe
CPP=cl.exe

!IF  "$(CFG)" == "AMDUTIL - Win32 Release"

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

ALL : "$(OUTDIR)\amdutil.exe"

CLEAN : 
	-@erase "..\amdutil.exe"
	-@erase ".\Release\Admtrie.obj"
	-@erase ".\Release\Routine.obj"
	-@erase ".\Release\Jdate.obj"
	-@erase ".\Release\WINSETUP.OBJ"
	-@erase ".\Release\AMDNET.OBJ"
	-@erase ".\Release\W32trace.obj"
	-@erase ".\Release\CRC.OBJ"
	-@erase ".\Release\Nettcp.obj"
	-@erase ".\Release\SLST.OBJ"
	-@erase ".\Release\Winutil.obj"
	-@erase ".\Release\W32reg.obj"
	-@erase "..\amdutil.ilk"
	-@erase ".\Release\amdutil.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /GX /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/Amdutil.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Amdutil.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib mpr.lib wsock32.lib amdio.lib lz32.lib /nologo /subsystem:windows /incremental:yes /debug /machine:I386 /out:"c:\test\amdutil.exe"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib mpr.lib wsock32.lib amdio.lib lz32.lib /nologo /subsystem:windows\
 /incremental:yes /pdb:"$(OUTDIR)/amdutil.pdb" /debug /machine:I386\
 /out:"c:\test\amdutil.exe" 
LINK32_OBJS= \
	"$(INTDIR)/Admtrie.obj" \
	"$(INTDIR)/Routine.obj" \
	"$(INTDIR)/Jdate.obj" \
	"$(INTDIR)/WINSETUP.OBJ" \
	"$(INTDIR)/AMDNET.OBJ" \
	"$(INTDIR)/W32trace.obj" \
	"$(INTDIR)/CRC.OBJ" \
	"$(INTDIR)/Nettcp.obj" \
	"$(INTDIR)/SLST.OBJ" \
	"$(INTDIR)/Winutil.obj" \
	"$(INTDIR)/W32reg.obj"

"$(OUTDIR)\amdutil.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "AMDUTIL - Win32 Debug"

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

ALL : "$(OUTDIR)\amdutil.exe"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\vc40.idb"
	-@erase "..\amdutil.exe"
	-@erase ".\Debug\W32reg.obj"
	-@erase ".\Debug\W32trace.obj"
	-@erase ".\Debug\SLST.OBJ"
	-@erase ".\Debug\AMDNET.OBJ"
	-@erase ".\Debug\CRC.OBJ"
	-@erase ".\Debug\Nettcp.obj"
	-@erase ".\Debug\Winutil.obj"
	-@erase ".\Debug\Admtrie.obj"
	-@erase ".\Debug\Routine.obj"
	-@erase ".\Debug\WINSETUP.OBJ"
	-@erase ".\Debug\Jdate.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/Amdutil.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Amdutil.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 LZ32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib mpr.lib wsock32.lib amdio.lib lz32.lib /nologo /subsystem:windows /pdb:none /debug /machine:I386 /force /out:"c:\test\amdutil.exe"
# SUBTRACT LINK32 /nodefaultlib
LINK32_FLAGS=LZ32.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 odbc32.lib odbccp32.lib mpr.lib wsock32.lib amdio.lib lz32.lib /nologo\
 /subsystem:windows /pdb:none /debug /machine:I386 /force\
 /out:"c:\test\amdutil.exe" 
LINK32_OBJS= \
	"$(INTDIR)/W32reg.obj" \
	"$(INTDIR)/W32trace.obj" \
	"$(INTDIR)/SLST.OBJ" \
	"$(INTDIR)/AMDNET.OBJ" \
	"$(INTDIR)/CRC.OBJ" \
	"$(INTDIR)/Nettcp.obj" \
	"$(INTDIR)/Winutil.obj" \
	"$(INTDIR)/Admtrie.obj" \
	"$(INTDIR)/Routine.obj" \
	"$(INTDIR)/WINSETUP.OBJ" \
	"$(INTDIR)/Jdate.obj"

"$(OUTDIR)\amdutil.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "AMDUTIL - Win32 Release"
# Name "AMDUTIL - Win32 Debug"

!IF  "$(CFG)" == "AMDUTIL - Win32 Release"

!ELSEIF  "$(CFG)" == "AMDUTIL - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\Winutil.c
DEP_CPP_WINUT=\
	".\winsetup.h"\
	".\W32TRACE.h"\
	".\admtrie.h"\
	".\crc.h"\
	".\slst.h"\
	".\parmblk.h"\
	".\routine.h"\
	".\winutil.h"\
	

!IF  "$(CFG)" == "AMDUTIL - Win32 Release"


"$(INTDIR)\Winutil.obj" : $(SOURCE) $(DEP_CPP_WINUT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "AMDUTIL - Win32 Debug"


"$(INTDIR)\Winutil.obj" : $(SOURCE) $(DEP_CPP_WINUT) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WINSETUP.C

"$(INTDIR)\WINSETUP.OBJ" : $(SOURCE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\W32trace.c
DEP_CPP_W32TR=\
	".\schglobal.h"\
	

"$(INTDIR)\W32trace.obj" : $(SOURCE) $(DEP_CPP_W32TR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\W32reg.c
DEP_CPP_W32RE=\
	".\W32TRACE.h"\
	".\parmblk.h"\
	

"$(INTDIR)\W32reg.obj" : $(SOURCE) $(DEP_CPP_W32RE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\SLST.C
DEP_CPP_SLST_=\
	".\slst.h"\
	

"$(INTDIR)\SLST.OBJ" : $(SOURCE) $(DEP_CPP_SLST_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Routine.c
DEP_CPP_ROUTI=\
	".\winsetup.h"\
	".\schglobal.h"\
	".\W32TRACE.h"\
	".\parmblk.h"\
	".\routine.h"\
	".\Amdnet.h"\
	".\Amdio.h"\
	".\amdioutl.h"\
	".\Jdate.h"\
	".\Nettcp.h"\
	

"$(INTDIR)\Routine.obj" : $(SOURCE) $(DEP_CPP_ROUTI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\CRC.C
DEP_CPP_CRC_C=\
	".\crc.h"\
	

"$(INTDIR)\CRC.OBJ" : $(SOURCE) $(DEP_CPP_CRC_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Admtrie.c
DEP_CPP_ADMTR=\
	".\admtrie.h"\
	".\W32TRACE.h"\
	

"$(INTDIR)\Admtrie.obj" : $(SOURCE) $(DEP_CPP_ADMTR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\AMDNET.C

!IF  "$(CFG)" == "AMDUTIL - Win32 Release"

DEP_CPP_AMDNE=\
	".\Nettcp.h"\
	".\W32TRACE.h"\
	".\crc.h"\
	".\schglobal.h"\
	
NODEP_CPP_AMDNE=\
	".\amdrmtex.h"\
	

"$(INTDIR)\AMDNET.OBJ" : $(SOURCE) $(DEP_CPP_AMDNE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "AMDUTIL - Win32 Debug"

DEP_CPP_AMDNE=\
	".\Nettcp.h"\
	".\W32TRACE.h"\
	".\crc.h"\
	".\schglobal.h"\
	

"$(INTDIR)\AMDNET.OBJ" : $(SOURCE) $(DEP_CPP_AMDNE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Nettcp.c
DEP_CPP_NETTC=\
	".\W32TRACE.h"\
	".\crc.h"\
	".\Nettcp.h"\
	

"$(INTDIR)\Nettcp.obj" : $(SOURCE) $(DEP_CPP_NETTC) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Jdate.c

!IF  "$(CFG)" == "AMDUTIL - Win32 Release"

DEP_CPP_JDATE=\
	".\Jdate.h"\
	
NODEP_CPP_JDATE=\
	".\%f\%f\%f"\
	

"$(INTDIR)\Jdate.obj" : $(SOURCE) $(DEP_CPP_JDATE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "AMDUTIL - Win32 Debug"

DEP_CPP_JDATE=\
	".\Jdate.h"\
	

"$(INTDIR)\Jdate.obj" : $(SOURCE) $(DEP_CPP_JDATE) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
