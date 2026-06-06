@echo off
cd /d D:\CPLANG
set "VSROOT=C:\Program Files\Microsoft Visual Studio\2022\Community"
set "MSVC_BIN=%VSROOT%\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"
set "VCIDE=%VSROOT%\Common7\IDE"

REM Set up PATH for MSVC tools
set "PATH=%MSVC_BIN%;%VCIDE%;%VCIDE%\Remote Debugger\x64;%PATH%"

REM Set LIB and INCLUDE as vcvarsall would
set "LIB=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\lib\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64"
set "INCLUDE=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared"

echo [Testing cl.exe with mspdbcore...]
echo int main(){return 0;}>_test_msvc.c
cl.exe /nologo /c _test_msvc.c /Fo_test_msvc.obj
if %ERRORLEVEL% EQU 0 (
    echo [OK] cl.exe works
    del _test_msvc.c _test_msvc.obj 2>nul
) else (
    echo [FAILED] cl.exe still fails
    type _test_msvc.c
)

echo.
echo [Now building cplang.exe...]

REM Call build_msvc.bat, skip its vcvarsall detection by setting VSCMD_ARG_TGT_ARCH
set "VSCMD_ARG_TGT_ARCH=x64"
set "VSCMD_VER=17.0"

REM Override VSROOT detection
call build_msvc.bat
echo [Build exit code: %ERRORLEVEL%]
