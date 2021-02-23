::
:: File:			win_test_pull_to_master_build.bat
:: Created on:		2021 Feb 09
:: Autor:			
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

set "PlatformTargetDefault=ARM,ARM64,x86,x64"
set "ConfigurationDefault=Debug,Release"
set "ActionConfirmDefault=Build"


set scriptDirectory=%~dp0
cd /D %scriptDirectory%..\..
set repositoryRoot=%cd%\

set "PlatformTarget="
set "Configuration="
set "ActionConfirm="

:: handling arguments
set nextArg=%scriptName%
for %%x in (%*) do (
	if /i "%%x"=="help" (
		call :call_help
		exit /b 0
	)
	set nextArg=%%x
	if !nextArg:~-1!==^" ( set "quote_found=yes" ) else ( set "quote_found=no" )
	if "!quote_found:~0,3!"=="yes" (
		set nextArg=!nextArg:~1,-1! 
		echo fixed arg: !nextArg!
	)
	call :parse_argument
	if not "!ERRORLEVEL!"=="0" (exit /b %ERRORLEVEL%)
)

if "%PlatformTarget%" == "" ( set "PlatformTarget=!PlatformTargetDefault!" )
if "%Configuration%" == "" ( set "Configuration=!ConfigurationDefault!" )
if "%ActionConfirm%" == "" ( set "ActionConfirm=!ActionConfirmDefault!" )

echo action=%ActionConfirm%; PlatformTarget=!PlatformTarget!; configuration=%Configuration%
rem exit /b 0

for %%p in (%PlatformTarget%) do (
	echo "+++++++++++ platform %%p"
	for %%c in (%Configuration%) do (
		echo "+++++++++++ +++++++++++ configuration %%c"
		for %%a in (%ActionConfirm%) do (
			echo "+++++++++++ +++++++++++ +++++++++++ action %%a"
			call msbuild "%repositoryRoot%workspaces\xdr_rpc-all_vs\xdr_rpc-all.sln" /t:%%a /p:Configuration=%%c /p:Platform=%%p -m:2
			if not "!ERRORLEVEL!"=="0" (exit /b !ERRORLEVEL!)
		)
	)
)


exit /b %ERRORLEVEL%


:parse_argument
	set isNextArgPlatform=true
	if /i not "!nextArg:~0,3!"=="ARM" if /i not "!nextArg:0,5!"=="ARM64" if /i not "!nextArg:~0,3!"=="x86" if /i not "!nextArg:~0,3!"=="x64" (set isNextArgPlatform=false)
	if "!isNextArgPlatform!"=="true" (set "PlatformTarget=!PlatformTarget!,!nextArg!") else (
		set isNextArgAction=true
		if /i not "!nextArg!"=="Build" if /i not "!nextArg!"=="Rebuild" if /i not "!nextArg!"=="Clean" (set isNextArgAction=false)
		if "!isNextArgAction!"=="true" (set "ActionConfirm=!ActionConfirm!,!nextArg!") else (
			set isNextArgConfiguration=true
			if /i not "!nextArg:~0,5!"=="Debug" if /i not "!nextArg:~0,7!"=="Release" (set isNextArgConfiguration=false)
			if "!isNextArgConfiguration!"=="true" (set "Configuration=!Configuration!,!nextArg!") else (
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
