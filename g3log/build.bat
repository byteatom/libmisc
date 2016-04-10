call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\VsDevCmd.bat"
cd g2log
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -G "Visual Studio 12" ..
msbuild g2log_by_kjellkod.sln /p:Configuration=Debug
cmake -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 12" ..
msbuild g2log_by_kjellkod.sln /p:Configuration=Release
pause