@echo off
cd /d D:\CPLANG
set "RC=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe"
set "KITINC=C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0"

echo [Compiling resources...]
"%RC%" /nologo /fo build\cplang.res /I "%KITINC%\um" /I "%KITINC%\shared" /I "%KITINC%\winrt" cplang.rc
if %ERRORLEVEL% EQU 0 (
    echo [RES OK]
    dir build\cplang.res
) else (
    echo [RES FAILED]
)
