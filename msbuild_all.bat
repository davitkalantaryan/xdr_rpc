::
:: File:			msbuild_all.bat
:: Created on:		2020 Jul 24
:: Autor:			Davit Kalantaryan (davit.kalantaryan@desy.de)
::
:: Purpose:	
::
:: Argumets: 
::

@echo off
setlocal EnableDelayedExpansion

:: here we define default platformtarget to build, platforms are following
:: other platforms should be provided as first argument
:: ARM, ARM64, x86, x64
set PlatformTarget=ARM,ARM64,x86,x64
set Configuration=Debug,Release
set ActionConfirm=Build

set scriptName=%0
set driveName=%~d0
set scriptDirectory=%~dp0
set currentDirectory=%cd%
rem cd %scriptDirectory%..
cd %scriptDirectory%
%driveName%
set repositoryRoot=%cd%\


:: handling arguments
::set argC=0
::for %%x in (%*) do Set /A argC+=1
set nextArg=%scriptName%
for %%x in (%*) do (
	if /i "%%x"=="help" (
		call :call_help
		exit /b 0
	)
	set nextArg=%%x
	set possibleQuote=!nextArg:~0,1!
	set possibleQuote|find """" >nul && set quote_found=yes||set quote_found=no
	if "!quote_found:~0,3!"=="yes" ( 
		set nextArg=!nextArg:~1,-1! 
		echo fixed arg: !nextArg!
	)
	call :parse_argument
	if not "!ERRORLEVEL!"=="0" (exit /b %ERRORLEVEL%)
)

echo action=%ActionConfirm%,PlatformTarget=!PlatformTarget!,configuration=%Configuration%


for %%p in (%PlatformTarget%) do (
	echo "!!!!!!!!!!!! platform %%p"
	for %%c in (%Configuration%) do (
		echo "!!!!!!!!!!!! !!!!!!!!!!!! compiling for configuration %%c"
		call msbuild "%repositoryRoot%workspaces\xdr_rpc-all_vs\xdr_rpc-all.sln" /t:!ActionConfirm! /p:Configuration=%%c /p:Platform=%%p
		if not "!ERRORLEVEL!"=="0" (exit /b !ERRORLEVEL!)
	)
)


exit /b %ERRORLEVEL%


:parse_argument
	set isNextArgPlatform=true
	if /i not "!nextArg:~0,3!"=="ARM" if /i not "!nextArg:0,5!"=="ARM64" if /i not "!nextArg:~0,3!"=="x86" if /i not "!nextArg:~0,3!"=="x64" (set isNextArgPlatform=false)
	if "!isNextArgPlatform!"=="true" (set PlatformTarget=!nextArg!) else (
		set isNextArgAction=true
		if /i not "!nextArg!"=="Build" if /i not "!nextArg!"=="Rebuild" if /i not "!nextArg!"=="Clean" (set isNextArgAction=false)
		if "!isNextArgAction!"=="true" (set ActionConfirm=!nextArg!) else (
			set isNextArgConfiguration=true
			rem if /i not "!nextArg!"=="Debug" if /i not "!nextArg!"=="Release" (set isNextArgConfiguration=false)
			if /i not "!nextArg:~0,5!"=="Debug" if /i not "!nextArg:~0,7!"=="Release" (set isNextArgConfiguration=false)
			if "!isNextArgConfiguration!"=="true" (set Configuration=!nextArg!) else (
				echo Unknown argument !nextArg!.
				call :call_help
				exit /b 1
			)
		)		
	)

	exit /b 0

:call_help
	echo Valid platforms are ARM, ARM64, x86 and x64. Using defalt x64.
	echo Valid configurations are Debug and Release. Using default Debug.
	echo Valid actions are Build, Rebuild and Clen. Using default Build.
	echo Call !scriptName! help to displaye help message
	exit /b 0

endlocal
