@echo off
chcp 65001 >nul
title CP Language 文件关联注册工具
cd /d "%~dp0"

set "CPLANG_DIR=%CD%"
set "ICON_PATH=%CPLANG_DIR%\icon\cpfile.ico"
set "COMPILER=%CPLANG_DIR%\build\cplang.exe"
set "REG_FILE=%TEMP%\cplang_reg.reg"

echo ============================================
echo  CP Language 文件关联注册
echo ============================================
echo.
echo 编译器路径: %COMPILER%
echo 图标路径:   %ICON_PATH%
echo.

REM Check if running as admin
net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [注意] 注册文件关联需要管理员权限。
    echo        请右键本脚本选择"以管理员身份运行"。
    echo.
    echo [尝试以管理员权限重新运行...]
    powershell -Command "Start-Process '%~f0' -Verb RunAs -Wait"
    exit /b 0
)

if not exist "%ICON_PATH%" (
    echo [错误] 图标文件不存在: %ICON_PATH%
    exit /b 1
)

REM 生成注册表文件
(
echo Windows Registry Editor Version 5.00
echo.
echo [HKEY_CLASSES_ROOT\.cp]
echo @="CPLang.File"
echo "Content Type"="text/plain"
echo.
echo [HKEY_CLASSES_ROOT\CPLang.File]
echo @="CP Language 源文件"
echo.
echo [HKEY_CLASSES_ROOT\CPLang.File\DefaultIcon]
echo @="\"%ICON_PATH:\=\\%\",0"
echo.
echo [HKEY_CLASSES_ROOT\CPLang.File\shell]
echo @="open"
echo.
echo [HKEY_CLASSES_ROOT\CPLang.File\shell\open]
echo @="使用记事本打开(&N)"
echo.
echo [HKEY_CLASSES_ROOT\CPLang.File\shell\open\command]
echo @="notepad.exe \"%%1\""
echo.
echo [HKEY_CLASSES_ROOT\CPLang.File\shell\compile]
echo @="编译运行(&R)"
echo.
echo [HKEY_CLASSES_ROOT\CPLang.File\shell\compile\command]
echo @="\"%COMPILER:\=\\%\" --aot -O2 -o \"%%~n1.exe\" \"%%1\""
echo.
echo [HKEY_CLASSES_ROOT\CPLang.File\shell\compile_aot]
echo @="AOT编译为独立exe(&A)"
echo.
echo [HKEY_CLASSES_ROOT\CPLang.File\shell\compile_aot\command]
echo @="cmd /k \"\"%COMPILER:\=\\%\" --aot -O2 -o \"%%~n1.exe\" \"%%1\" && echo [完成] 按任意键退出... && pause >nul\""
echo.
) > "%REG_FILE%"

echo [导入注册表...]
regedit /s "%REG_FILE%"
if %ERRORLEVEL% EQU 0 (
    echo [成功] .cp 文件关联已注册！
    echo.
    echo 已注册的操作:
    echo   - 双击 .cp 文件 → 用记事本打开
    echo   - 右键 → 编译运行
    echo   - 右键 → AOT编译为独立exe
) else (
    echo [失败] 注册表导入出错
    exit /b 1
)

del "%REG_FILE%" 2>nul
echo.
pause
