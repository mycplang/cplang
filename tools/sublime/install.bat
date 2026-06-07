@echo off
echo ========================================
echo   CP Language - Sublime Text 插件安装
echo ========================================
echo.

set "FOUND=0"
set "SUBLIME_PKG=C:\Sublime Text\Data\Packages\User"
if exist "%SUBLIME_PKG%" set "FOUND=1" & goto :install
set "SUBLIME_PKG=%APPDATA%\Sublime Text\Packages\User"
if exist "%SUBLIME_PKG%" set "FOUND=1" & goto :install
set "SUBLIME_PKG=%APPDATA%\Sublime Text 3\Packages\User"
if exist "%SUBLIME_PKG%" set "FOUND=1" & goto :install

echo 错误: 未找到 Sublime Text 安装目录
echo 请手动复制以下文件到 Packages\User 目录
pause
exit /b 1

:install
echo 安装到: %SUBLIME_PKG%
echo.
copy /Y "%~dp0CP.sublime-syntax"       "%SUBLIME_PKG%\" >nul && echo   CP.sublime-syntax       OK
copy /Y "%~dp0CP.sublime-completions" "%SUBLIME_PKG%\" >nul && echo   CP.sublime-completions OK
copy /Y "%~dp0CP.sublime-build"       "%SUBLIME_PKG%\" >nul && echo   CP.sublime-build       OK
copy /Y "%~dp0CP.sublime-settings"    "%SUBLIME_PKG%\" >nul && echo   CP.sublime-settings    OK

echo.
echo 安装完成！重启 Sublime Text 即可使用。
echo.
echo   Ctrl+B → 编译运行 / 语法检查 / JIT / AOT / 调试
echo   输入代码 → 自动补全关键字+内置函数
echo   错误行号 → 双击跳转
echo.
pause
