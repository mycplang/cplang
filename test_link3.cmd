@echo off
cd /d D:\CPLANG

REM Set up MSVC environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

REM Create a simple .obj
echo int main(){return 0;}>tl3.c
cl.exe /nologo /c tl3.c /Fotl3.obj

REM Try linking with link.exe
echo ---
echo [Linking with link.exe...]
link.exe /nologo tl3.obj /out:tl3.exe /subsystem:console
if exist tl3.exe (
    tl3.exe
    echo [LINK OK]
) else (
    echo [LINK FAILED]
)
del tl3.c tl3.obj tl3.exe 2>nul
