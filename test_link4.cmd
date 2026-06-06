@echo off
cd /d D:\CPLANG

REM Set up MSVC
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Override PATH for Hostx86\x64 cl.exe
set "PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx86\x64;%PATH%"

echo [Testing cl: where cl.exe]
where cl.exe

echo int main(){return 0;}>tl4.c
cl.exe /nologo /c tl4.c /Fotl4.obj
if exist tl4.obj (echo [COMPILE OK]) else (echo [COMPILE FAILED])

echo [Testing link: where link.exe]
where link.exe

link.exe /nologo tl4.obj /out:tl4.exe /subsystem:console
if exist tl4.exe (echo [LINK OK]) else (echo [LINK FAILED])

del tl4.c tl4.obj tl4.exe 2>nul
