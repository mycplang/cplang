@echo off
setlocal enabledelayedexpansion
set "SCRIPT_DIR=%~dp0"
set "LLVM_DIR=%SCRIPT_DIR%llvm-dev"
set "LLVM_VER=18.1.8"
echo ============================================
echo  CP Language LLVM Auto-Downloader
echo ============================================
echo.
echo Target: !LLVM_DIR!
echo Version: LLVM !LLVM_VER!
echo.
if exist "!LLVM_DIR!\bin\llvm-config.exe" (
    echo [OK] LLVM already installed at !LLVM_DIR!
    echo      Delete that folder first if you want to re-download.
    endlocal
    exit /b 0
)
echo [1/3] Downloading LLVM !LLVM_VER! for Windows...
set "URL=https://github.com/llvm/llvm-project/releases/download/llvmorg-!LLVM_VER!/LLVM-!LLVM_VER!-win64.exe"
set "OUTFILE=%TEMP%\LLVM-!LLVM_VER!-win64.exe"
echo Download URL: !URL!
echo.
curl -L -o "!OUTFILE!" "!URL!"
if !ERRORLEVEL! NEQ 0 (
    echo [ERROR] Download failed. Try manually:
    echo         !URL!
    echo         Then extract to: !LLVM_DIR!
    endlocal
    exit /b 1
)
echo [2/3] Extracting...
md "!LLVM_DIR!" 2>nul
rem Try 7z first (common tool)
where 7z >nul 2>nul
if !ERRORLEVEL! EQU 0 (
    7z x "!OUTFILE!" -o"!LLVM_DIR!" -y >nul
) else (
    rem Fallback: run installer silently
    "!OUTFILE!" /S /D=!LLVM_DIR!
)
if !ERRORLEVEL! NEQ 0 (
    echo [ERROR] Extraction failed. Please manually run:
    echo         "!OUTFILE!" /S /D=!LLVM_DIR!
    endlocal
    exit /b 1
)
echo [3/3] Verifying...
if exist "!LLVM_DIR!\bin\llvm-config.exe" (
    echo [OK] LLVM !LLVM_VER! installed successfully
    echo      Location: !LLVM_DIR!
) else (
    echo [WARN] Extraction may have failed - llvm-config.exe not found
    echo        Try running: "!OUTFILE!" /S /D=!LLVM_DIR!
)
del "!OUTFILE!" 2>nul
endlocal