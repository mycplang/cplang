@echo off
cd /d D:\CPLANG
set "MSVCROOT=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207"
set "CL_EXE=%MSVCROOT%\bin\Hostx86\x64\cl.exe"
set "PATH=%MSVCROOT%\bin\Hostx86\x64;%MSVCROOT%\bin\Hostx64\x64;%PATH%"
echo [Testing: %CL_EXE%]
echo int main(){return 0;}>_test_msvc.c
"%CL_EXE%" /nologo /c _test_msvc.c /Fo_test_msvc.obj
if %ERRORLEVEL% EQU 0 (echo [OK]) else (echo [FAILED])
del _test_msvc.c _test_msvc.obj 2>nul
