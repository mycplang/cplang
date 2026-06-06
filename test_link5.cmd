@echo off
cd /d D:\CPLANG

REM Set up MSVC
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
set "PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx86\x64;%PATH%"

REM Create a simple .obj using cl.exe
echo int main(){return 0;}>tl5.c
cl.exe /nologo /c tl5.c /Fotl5.obj >nul 2>&1

REM Test: direct full path to Hostx64\x64\link.exe
echo [Testing Hostx64\x64\link.exe directly...]
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\link.exe" /nologo tl5.obj /out:tl5_a.exe /subsystem:console 2>&1
if exist tl5_a.exe (echo [Hostx64\x64\link.exe WORKS]) else (echo [Hostx64\x64\link.exe FAILED])

REM Test: direct full path to Hostx86\x64\link.exe
echo [Testing Hostx86\x64\link.exe directly...]
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx86\x64\link.exe" /nologo tl5.obj /out:tl5_b.exe /subsystem:console 2>&1
if exist tl5_b.exe (echo [Hostx86\x64\link.exe WORKS]) else (echo [Hostx86\x64\link.exe FAILED])

del tl5.c tl5.obj tl5_a.exe tl5_b.exe 2>nul
