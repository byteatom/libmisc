@echo off

goto Win%PROCESSOR_ARCHITEW6432%

:Win
goto %PROCESSOR_ARCHITECTURE%

:Winx86
goto x86

:Winamd64
goto x64

:x86
echo 32-bit
"%~dp0Inst\x86\MirrInst.exe" -u dfmirage
goto Result

:amd64
echo 64-bit
"%~dp0Inst\x64\MirrInst.exe" -u dfmirage
goto Result

:Result
echo Result code is %ERRORLEVEL%

:end
