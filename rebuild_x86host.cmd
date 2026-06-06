@echo off
cd /d D:\CPLANG

REM Step 1: Set up MSVC environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Step 2: Override cl.exe to use Hostx86\x64 (works around mspdbcore issue)
set "MSVCROOT=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207"
set "PATH=%MSVCROOT%\bin\Hostx86\x64;%PATH%"

echo [Using: %MSVCROOT%\bin\Hostx86\x64\cl.exe]

REM Step 3: Set VSROOT so build_msvc.bat finds it
set "VSROOT=C:\Program Files\Microsoft Visual Studio\2022\Community"

REM Step 4: Build (VSCMD_ARG_TGT_ARCH keeps it from re-detecting)
call build_msvc.bat
echo [Exit: %ERRORLEVEL%]
