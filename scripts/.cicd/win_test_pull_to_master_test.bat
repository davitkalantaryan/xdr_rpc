::
:: File:			win_test_pull_to_master_test.bat
:: Created on:		2021 Feb 09
:: Autor:			
::
:: Purpose:	
::
:: Argumets: 
::
@echo off
setlocal EnableDelayedExpansion

set scriptDirectory=%~dp0
cd /D %scriptDirectory%..\..
set repositoryRoot=%cd%\

cd %repositoryRoot%prj\tests\googletest_mult
call nmake /f windows.Makefile
if not "!ERRORLEVEL!"=="0" (exit /b !ERRORLEVEL!)

cd %repositoryRoot%sys\win_x64\Debug\test
.\googletest.exe
if not "!ERRORLEVEL!"=="0" (exit /b !ERRORLEVEL!)

exit /b !ERRORLEVEL!

endlocal
