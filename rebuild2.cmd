@echo off
cd /d D:\CPLANG

REM Remove the fake link.exe we placed earlier
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\link.exe" (
    echo [Removing fake link.exe wrapper...]
    del /f /q "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\link.exe"
)

REM Clean build directory
if exist build\*.obj del /q build\*.obj 2>nul

REM Set up MSVC environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

echo [Building cplang.exe with MSVC native tools...]
call build_msvc.bat
echo [Exit code: %ERRORLEVEL%]
