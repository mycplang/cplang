@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul

set "SCRIPT_DIR=%~dp0"

:: 查找 cplang.exe
set "CPLANG="
for %%c in (
    "%SCRIPT_DIR%build_llvm\bin\Release\cplang.exe"
    "%SCRIPT_DIR%build_cmake_new\bin\Release\cplang.exe"
    "%SCRIPT_DIR%build\cplang.exe"

) do (
    if exist "%%c" (
        set "CPLANG=%%c"
        goto :found
    )
)
:found

if "%CPLANG%"=="" (
    echo [错误] 找不到 cplang.exe，请先运行 build_msvc.bat
    endlocal
    exit /b 1
)

set "TEST_DIR=%SCRIPT_DIR%tests\cp"
set "PASSED=0"
set "FAILED=0"

echo ══════════════════════════════════════════════════
echo   数据库集成测试 (MySQL + PostgreSQL + Redis)
echo ══════════════════════════════════════════════════
echo   cplang.exe: %CPLANG%
echo   目标服务器: 120.48.128.250
echo.

:: ─── MySQL 测试 ─────────────────────────────────
echo ┌──────────────────────────────────────────┐
echo │  [1/3] MySQL  (120.48.128.250:3306)      │
echo └──────────────────────────────────────────┘
echo   claw_mall_test / 操作: root
echo.

"%CPLANG%" -c "%TEST_DIR%\test_mysql.cp"
if !ERRORLEVEL! EQU 0 (
    echo.
    echo   [结果] MySQL 测试完成 (exit=0)
    set /a PASSED+=1
) else (
    echo.
    echo   [结果] MySQL 测试失败 (exit=!ERRORLEVEL!)
    set /a FAILED+=1
)
echo.

:: ─── PostgreSQL 测试 ────────────────────────────
echo ┌──────────────────────────────────────────┐
echo │  [2/3] PostgreSQL (120.48.128.250:5432)  │
echo └──────────────────────────────────────────┘
echo   claw_mall / 操作: claw_mall
echo.

"%CPLANG%" -c "%TEST_DIR%\test_pg.cp"
if !ERRORLEVEL! EQU 0 (
    echo.
    echo   [结果] PostgreSQL 测试完成 (exit=0)
    set /a PASSED+=1
) else (
    echo.
    echo   [结果] PostgreSQL 测试失败 (exit=!ERRORLEVEL!)
    set /a FAILED+=1
)
echo.

:: ─── Redis 测试 ──────────────────────────────
echo ┌──────────────────────────────────────────┐
echo │  [3/3] Redis   (120.48.128.250:6379)     │
echo └──────────────────────────────────────────┘
echo   认证: AUTH ClawMall_2026!
echo.

"%CPLANG%" -c "%TEST_DIR%\test_redis.cp"
if !ERRORLEVEL! EQU 0 (
    echo.
    echo   [结果] Redis 测试完成 (exit=0)
    set /a PASSED+=1
) else (
    echo.
    echo   [结果] Redis 测试失败 (exit=!ERRORLEVEL!)
    set /a FAILED+=1
)
echo.

:: ─── 汇总 ──────────────────────────────────────
echo ══════════════════════════════════════════════════
set /a TOTAL=%PASSED%+%FAILED%
echo   数据库测试: %PASSED%/%TOTAL% 通过
if %FAILED% EQU 0 (
    echo   [全部通过] 所有数据库集成测试通过！
) else (
    echo   [有失败] %FAILED% 个数据库测试失败
)
echo ══════════════════════════════════════════════════

endlocal
exit /b %FAILED%