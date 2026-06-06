@echo off
cd /d D:\CPLANG

REM Set up MSVC environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
set "PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx86\x64;%PATH%"
set "VSROOT=C:\Program Files\Microsoft Visual Studio\2022\Community"

echo [Running AOT with MSVC environment set up...]
build\cplang.exe --aot -O2 -v -o hello_final.exe examples\hello.cp 2>&1

if exist hello_final.exe (
    echo [AOT SUCCESS]
    echo ==========================
    hello_final.exe
    echo ==========================
    dir hello_final.exe
) else (
    echo [AOT FAILED]
)
