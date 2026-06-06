@echo off
cd /d D:\CPLANG
set "VSROOT=C:\Program Files\Microsoft Visual Studio\2022\Community"

REM Source vcvarsall
call "%VSROOT%\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Set VSROOT explicitly so build_msvc.bat uses it
set "VSROOT=%VSROOT%"

echo [VSROOT=%VSROOT%]
echo [Building cplang.exe with MSVC...]
call build_msvc.bat
echo [Exit code: %ERRORLEVEL%]
