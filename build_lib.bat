::
:: File:			build_lib.bat
:: Created on:		2020 Feb 27
:: Autor:			Davit Kalantaryan (davit.kalantaryan@desy.de)
::
:: Purpose:	
::
:: Argumets: 
::

@ECHO off
SETLOCAL enableextensions

:: here we define platformtarhet to build, platforms are following
:: ARM, ARM64, x86, x64
set PlatformTarget=x64
set Configuration=Debug

SET  scriptDirectory=%~dp0
set  currentDirectory=%cd%

msbuild %scriptDirectory%prj\core\doocs_win_xdr_rpc_vs\doocs_win_xdr_rpc.sln /t:Build /p:Configuration=%Configuration% /p:Platform=%PlatformTarget%

ENDLOCAL
