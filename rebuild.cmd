@echo off
cd /d D:\CPLANG
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
echo [Env] LIB=%LIB:~0,80%...
echo [Env] INCLUDE=%INCLUDE:~0,80%...
echo [Building cplang.exe with MSVC...]
call build_msvc.bat
echo [Build completed with errorlevel=%ERRORLEVEL%]
