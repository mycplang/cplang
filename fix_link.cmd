@echo off
cd /d D:\CPLANG
set MSVC_BIN=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin

echo [Copying link.exe from Hostx86\x64 to Hostx64\x64...]
copy "%MSVC_BIN%\Hostx86\x64\link.exe" "%MSVC_BIN%\Hostx64\x64\link.exe" /y
if exist "%MSVC_BIN%\Hostx64\x64\link.exe" (
    echo [OK] link.exe restored
) else (
    echo [FAILED] Could not restore link.exe
    exit /b 1
)

REM Also copy mspdb files that link.exe needs
copy "%MSVC_BIN%\Hostx86\x64\mspdb*.dll" "%MSVC_BIN%\Hostx64\x64\" /y >nul 2>&1
echo [Done]
