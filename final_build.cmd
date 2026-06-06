@echo off
cd /d D:\CPLANG
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 || exit /b 1
echo [VCvars done. Running build_msvc.bat...]
call build_msvc.bat
set EXIT_CODE=%ERRORLEVEL%
echo [Final exit code: %EXIT_CODE%]
exit /b %EXIT_CODE%
