@echo off
cd /d D:\CPLANG
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
echo [PATH entries for link.exe]
for %%i in (link.exe) do where %%i 2>nul
echo.
echo [Direct link.exe check]
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\link.exe" (
    echo FOUND: MSVC link.exe
) else (
    echo NOT FOUND: MSVC link.exe
    dir "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\" 2>nul
)
echo.
echo [VC\\bin directory listing]
dir /b "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\" 2>nul
