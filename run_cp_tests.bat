@echo off
setlocal enabledelayedexpansion
set "SCRIPT_DIR=%~dp0"
set "CPLANG=%SCRIPT_DIR%build\cplang.exe"
set "TEST_DIR=%SCRIPT_DIR%tests\cp"
if not exist "%CPLANG%" (
    echo CPLANG ERROR: cplang.exe not found. Run build_msvc.bat first.
    endlocal
    exit /b 1
)
echo === CP Language Tests ===
echo.
set PASSED=0
set FAILED=0
set TOTAL=0
for %%f in ("%TEST_DIR%\*.cp") do (
    set /a TOTAL+=1
    set "NAME=%%~nf"
    echo TEST: %%f ...
    "%CPLANG%" -c "%%f" >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        echo   PASSED
        set /a PASSED+=1
    ) else (
        echo   FAILED (exit code: !ERRORLEVEL!)
        set /a FAILED+=1
    )
)
echo.
echo Results: !PASSED!/!TOTAL! passed, !FAILED! failed
endlocal
exit /b !FAILED!