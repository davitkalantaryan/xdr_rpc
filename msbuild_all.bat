::
:: File:			msbuild_all.bat
:: Created on:		2021 Feb 11
:: Autor:			
::
:: Purpose:	
::
:: Argumets: 
::
@echo off
setlocal EnableDelayedExpansion


set scriptDirectory=%~dp0
rem cd /D %scriptDirectory%..
cd /D %scriptDirectory%
set repositoryRoot=%cd%\

call %repositoryRoot%scripts\.cicd\win_test_pull_to_master_build.bat  %*

endlocal
