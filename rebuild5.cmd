@echo off
cd /d D:\CPLANG

REM Use the VS dev prompt properly - this is the correct way
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

echo [Checking cl.exe works...]
echo int main(){return 0;}>_test_msvc.c
where cl.exe
cl.exe /nologo /c _test_msvc.c /Fo_test_msvc.obj
if %ERRORLEVEL% EQU 0 (
    echo [OK] cl.exe works
    del _test_msvc.c _test_msvc.obj 2>nul
) else (
    echo [FAILED] Direct cl.exe usage fails
)

echo.
echo [PATH check for mspdbcore...]
dir /s /b "%MSVC_BIN%\mspdbcore.dll" 2>nul

echo.
echo [Now running build_msvc.bat with VSROOT set...]
set "VSROOT=C:\Program Files\Microsoft Visual Studio\2022\Community"
set "VSCMD_ARG_TGT_ARCH=x64"
set "VSCMD_VER=17.0"

REM Disable the sub-call to vcvarsall by setting SKIP_VCVARS
REM Actually, let's patch the approach: pass VSROOT as env var
call build_msvc.bat
echo [Exit: %ERRORLEVEL%]
