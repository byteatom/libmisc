cd boost
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\VsDevCmd.bat"
call bootstrap.bat
.\b2 --stagedir=stage\x86
.\b2 --stagedir=stage\x86_64 address-model=64
pause