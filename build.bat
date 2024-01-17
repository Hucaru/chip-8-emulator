@echo off

set VSTOOLS="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
call %VSTOOLS%

if not exist ".\build" mkdir .\build
if not exist ".\bin" mkdir .\bin

set FLAGS=/Fe: ./bin/emulator.exe /Fo"build\\" /Fd"build\\" /std:c++latest /EHsc /FC /Zi
set INCLUDE_DIR=/I./src
set LIBS=user32.lib gdi32.lib
set CPP=src/emulator.cpp src/win32.cpp

cl.exe %CPP% %LIBS% %FLAGS%