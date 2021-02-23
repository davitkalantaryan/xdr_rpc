#
# file:			Makefile (to create sss library)
# created on:	2020 Dec 02
# created by:	
#
# purpose:		To build DOOCS client library without EPICS
# Examples of calling:
#		>nmake /f windows.Makefile /e Platform=x64
#		>nmake /f windows.Makefile /e Platform=x86
#		>nmake /f windows.Makefile 
#
# Please specify any platform target from following list
# arm
# arm64
# x64 
# x86
# Platform				= x64  # default
# PlatformTarget	# will be specified automatically
#
# below is the variables, that should be specified by parent Makefile, before including this
# TargetName				= libsss # this s defined by final makefile
# CallerMakeFilePath		= XXX  # this one you can provide $(MAKEDIR)\makeFileName
#
# here are default values - will be set if parent Makefile does not specify them'
# Platform				= x64  # default
# ObjectsDirBase		= $(CallerMakeFileDir)\.objects
# SourcesRootDir		= $(CallerMakeFileDir)
#
# @$(LINKER) $(LFLAGS) $(Objects) $(LIBS) /DLL $(LIBPATHS) /SUBSYSTEM:CONSOLE /OUT:$(TargetName).$(TargetExtension)
#  {$(SrcBaseDir)\}.cpp{$(ObjectsDir)}.obj:
#  @$(CPPC) /c $(CXXFLAGS) /Fo$(ObjectsDir)\$(@D)\ $(SrcBaseDir)\$*.cpp
#  @$(CPPC) /c $(CXXFLAGS) /Fo$(ObjectsDir)\$(@D)\ $*.cpp
#
# to clean google test run below commands

# one can redefine this by this nmake /e Platform=x86
Platform				= x64
Configuration			= Debug

# this is set automatically when needed
!IFNDEF MakeFileDir
#!IF "$(MakeFileDir)" == ""
MakeFileDir				= $(MAKEDIR)
!ENDIF

RepoRootDir				= $(MakeFileDir)\..\..\..
SrcBaseDir				= $(MakeFileDir)\..\..\..\src
GoogleTestDir			= $(MakeFileDir)\packages\Microsoft.googletest.v140.windesktop.msvcstl.static.rt-dyn.1.8.1.3

TargetName				= googletest
TargetExtension			= exe
TargetFileName			= $(TargetName).$(TargetExtension)
TargetDirectory			= $(RepoRootDir)\sys\win_$(Platform)\$(Configuration)\test

#ObjectsDirBase			= $(MakeFileDir)\.objects
#ObjectsDir				= $(ObjectsDirBase)\$(TargetName)
ObjectsDir				= $(SrcBaseDir)

CC						= cl 
CPPC           			= cl -Zc:__cplusplus
LINKER        			= link
PDB_FILE_PATH			= $(TargetDirectory)\$(TargetName).pdb
DEFINES       			= $(DEFINES) /D "_WINDLL" /D "_MBCS"
#INCLUDE_PATHS			= $(INCLUDE_PATHS) /I"$(RepoRootDir)\contrib\googletest\googletest\include"
INCLUDE_PATHS			= $(INCLUDE_PATHS) /I"$(GoogleTestDir)\build\native\include"
#CFLAGS					= $(CFLAGS) $(INCLUDE_PATHS) $(DEFINES) /bigobj /MTd /nologo
CFLAGS					= $(CFLAGS) $(INCLUDE_PATHS) $(DEFINES) /MDd /nologo
CXXFLAGS				= $(CXXFLAGS) $(CFLAGS)
CXXFLAGS				= $(CXXFLAGS) /JMC /permissive- /GS /W3 /Zc:wchar_t  /ZI /Gm- /Od /sdl- 
#CXXFLAGS				= $(CXXFLAGS) /Fd"$(PDB_FILE_PATH)" /FI"libsss_api.h" 
CXXFLAGS				= $(CXXFLAGS) /Zc:inline /fp:precise /errorReport:prompt /WX- /Zc:forScope /RTC1 /Gd 
CXXFLAGS				= $(CXXFLAGS) /FC /EHsc /diagnostics:column
#LIBPATHS				= $(LIBPATHS) /LIBPATH:"$(RepoRootDir)\contrib\googletest\lib\$(Configuration)"
LIBPATHS				= $(LIBPATHS) /LIBPATH:"$(GoogleTestDir)\lib\native\v140\windesktop\msvcstl\static\rt-dyn\$(Platform)\$(Configuration)"
LIBS					=
LIBS					= $(LIBS) "gtest_maind.lib"
LIBS					= $(LIBS) "gtestd.lib"

LFLAGS					= $(LFLAGS) /OUT:"$(TargetDirectory)\$(TargetFileName)" 
LFLAGS					= $(LFLAGS) /MANIFEST /NXCOMPAT /PDB:"$(TargetDirectory)\$(TargetName).pdb" 
LFLAGS					= $(LFLAGS) /DYNAMICBASE $(LIBS) 
LFLAGS					= $(LFLAGS) /DEBUG /MACHINE:$(Platform) /INCREMENTAL  
LFLAGS					= $(LFLAGS) /SUBSYSTEM:CONSOLE /MANIFESTUAC:"level='asInvoker' uiAccess='false'" 
LFLAGS					= $(LFLAGS) /ERRORREPORT:PROMPT /NOLOGO $(LIBPATHS) /TLBID:1

#Objects				= 

DirectoriesToCompile	=
#DirectoriesToCompile	= $(DirectoriesToCompile) nr-build\gen\cpp\sss\ssslang\antlr
DirectoriesToCompile	= tests\googletest

default: googletest


googletest: __preparationForSetObjectsForBuild __setObjects


clean: __preparationForSetObjectsForClean __setObjects
	@if exist "$(TargetDirectory)\$(TargetName).*" del /s /q "$(TargetDirectory)\$(TargetName).*"
	@echo "clean done!"


__preparationForSetObjectsForBuild:
	@echo -=-=-=-=-=-=-=-==-=-=-=-=-=-==-=-=-=-=-=-=-= __preparationForSetObjectsForBuild
	@set __targetToCall=__buildRaw
	@set __makeFileName=windows.Makefile


__preparationForBuildRaw:
	@cd $(SrcBaseDir)
	@if not exist $(TargetDirectory) mkdir $(TargetDirectory)


__buildRaw: __buildGoogleTestLib __preparationForBuildRaw $(Objects)
	@cd $(ObjectsDir)
	@$(LINKER) $(LFLAGS) $(Objects)


#__buildGoogleTestLib:
#	@cd $(RepoRootDir)\contrib\googletest
#	@cmake -A $(Platform) -Dgtest_force_shared_crt=1 .
#	@cmake --build .
__buildGoogleTestLib:
	@cd $(MakeFileDir)
	@msbuild $(MakeFileDir)\googletest_getter.sln -t:restore -p:RestorePackagesConfig=true

__preparationForSetObjectsForClean:
	@echo -=-=-=-=-=-=-=-==-=-=-=-=-=-==-=-=-=-=-=-=-= __preparationForSetObjectsForBuild
	@set __targetToCall=__cleanRaw
	@set __makeFileName=windows.Makefile


__cleanRaw:
	@<<windows_nmake_makefile_clean_raw.bat
		@echo off
		setlocal EnableDelayedExpansion enableextensions
		for %%i in ($(Objects)) do ( if exist "$(ObjectsDir)\%%i" ( del /Q /F "$(ObjectsDir)\%%i" ) )
		endlocal
<<NOKEEP


!include <$(RepoRootDir)\prj\common\common_mkfl\windows.common.Makefile>
