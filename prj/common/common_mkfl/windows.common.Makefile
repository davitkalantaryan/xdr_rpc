#
# file:			windows.common.Makefile
# created on:	2020 Dec 14
# created by:	
#
# This file can be only as include
#

# if 'nr-' specified before the name of directory, then this directory
# scanned for sources not recursively

#DirectoriesToCompile	= nr-.
#DirectoriesToCompile	= $(DirectoriesToCompile) nr-atn
#DirectoriesToCompile	= $(DirectoriesToCompile) dfa
#DirectoriesToCompile	= $(DirectoriesToCompile) misc
#DirectoriesToCompile	= $(DirectoriesToCompile) support
#DirectoriesToCompile	= $(DirectoriesToCompile) tree
#DirectoriesToCompile	= $(DirectoriesToCompile) tree\pattern
#DirectoriesToCompile	= $(DirectoriesToCompile) tree\xpath

.cpp.obj:
	 @$(CPPC) /c $(CXXFLAGS) /Fo$(ObjectsDir)\$(@D)\ $<

.cxx.obj:
	 @$(CPPC) /c $(CXXFLAGS) /Fo$(ObjectsDir)\$(@D)\ $<

.cc.obj:
	@$(CPPC) /c $(CXXFLAGS) /Fo$(ObjectsDir)\$(@D)\ $*.cc

.c.obj:
	@$(CC) /c   $(CFLAGS)   /Fo$(ObjectsDir)\$(@D)\ $*.c

__setObjects:
	@<<windows_nmake_makefile_setobjects.bat
		@echo off
		setlocal EnableDelayedExpansion enableextensions

		set scriptName=%0
		set scriptDirectory=%~dp0
		set currentDirectory=%cd%

		set ObjectsVar=$(Objects)

		for %%i in ($(DirectoriesToCompile)) do (
			::echo ++++++++++++++++++++++++++++++++++ %%i
			set directoryName=%%i
			set is_recursive_string=!directoryName:~0,3!
			if "!is_recursive_string!" == "nr-" (
				set is_recursive=0
				set directoryName=!directoryName:~3!
			) else (
				set is_recursive=1
			)
			call :iterate_over_files_in_dir !directoryName!
		)

		if not "!VSCMD_ARG_HOST_ARCH!"=="$(Platform)" (
			echo "calling VsDevCmd in the !scriptDirectory!!scriptName!"
			if /i "$(Platform)"=="x64" (
				set PlatformTarget=amd64
			) else (
				set PlatformTarget=$(Platform)
			)
			call VsDevCmd -arch=!PlatformTarget!
		) else (
			echo "VsDevCmd already set to $(Platform)"
		)


		$(MAKE) /f $(MakeFileDir)\%__makeFileName% %__targetToCall% ^
				/e Objects="!ObjectsVar!"  ^
				/e Platform=$(Platform)     ^
				/e MakeFileDir=$(MakeFileDir) 

		exit /b !ERRORLEVEL!

		:trim_string
			set %2=%1
			exit /b 0

		:iterate_over_files_in_dir

			cd $(SrcBaseDir)
			set TARGET_PATH_FOR_SOURCE=%cd%\
			cd $(SrcBaseDir)\%1

			if "!is_recursive!" == "0" (
				echo ++++++++++++++++++++++++++++++++++ not recursive dir "%1"
				if not exist "$(ObjectsDir)\%1" mkdir "$(ObjectsDir)\%1"
				for %%I in ("*.cpp" "*.c" "*.cc" "*.cxx") do (
					set relFilePath=%1\%%~nI.obj
					set shouldExlude=0
					for %%e in ($(excludedObjects)) do (
						if "!relFilePath!" == "%%e" (
							echo %%e should be exluded
							set shouldExlude=1
							rem goto :eofloop
						)
					)
					:eofloop
					if "!shouldExlude!" == "0" (
						set ObjectsVar=!ObjectsVar! !relFilePath!
						echo !relFilePath!
					)
					rem iteration of loop done
				)
			) else (
				echo ++++++++++++++++++++++++++++++++++ recursive     dir "%1"
				for /r %%I in ("*.cpp" "*.c" "*.cc" "*.cxx") do (
					set "dirPath=%%~dpI"
					set relDirPath=!dirPath:%TARGET_PATH_FOR_SOURCE%=!
					if not exist "$(ObjectsDir)\!relDirPath!" mkdir "$(ObjectsDir)\!relDirPath!"
					
					set "filePath=%%~dpnI.obj"
					set relFilePath=!filePath:%TARGET_PATH_FOR_SOURCE%=!
					set shouldExlude=0
					for %%e in ($(excludedObjects)) do (
						if "!relFilePath!" == "%%e" (
							echo %%e should be exluded
							set shouldExlude=1
							rem goto :eofloop
						)
					)
					:eofloop
					if "!shouldExlude!" == "0" (
						set ObjectsVar=!ObjectsVar! !relFilePath!
						echo !relFilePath!
					)
					rem iteration of loop done
				)
			)
			
			cd !currentDirectory!
			exit /b 0
			
		endlocal
<<NOKEEP
