@echo off
echo ========================================
echo   CP Language - Sublime Text 插件安装
echo ========================================
echo.

set "SUBLIME_PKG=%APPDATA%\Sublime Text\Packages\User"
if exist "%SUBLIME_PKG%" (
    echo 找到 Sublime Text: %SUBLIME_PKG%
) else (
    set "SUBLIME_PKG=%APPDATA%\Sublime Text 3\Packages\User"
    if exist "%SUBLIME_PKG%" (
        echo 找到 Sublime Text 3: %SUBLIME_PKG%
    ) else (
        echo 错误: 未找到 Sublime Text 安装目录
        echo 请手动复制以下文件到 Sublime Text 的 Packages/User 目录:
        echo   CP.sublime-syntax
        echo   CP.sublime-completions
        echo   CP.sublime-build
        echo   CP.sublime-settings
        pause
        exit /b 1
    )
)

echo 正在安装...
copy /Y "CP.sublime-syntax"   "%SUBLIME_PKG%\CP.sublime-syntax"   >nul
copy /Y "CP.sublime-completions" "%SUBLIME_PKG%\CP.sublime-completions" >nul
copy /Y "CP.sublime-build"       "%SUBLIME_PKG%\CP.sublime-build"       >nul
copy /Y "CP.sublime-settings"    "%SUBLIME_PKG%\CP.sublime-settings"    >nul

echo.
echo 安装完成！重启 Sublime Text 即可使用。
echo.
echo 功能:
echo   - 语法高亮 (.cp 文件自动识别)
echo   - 代码补全 (输入时自动触发)
echo   - 编译运行 (Ctrl+B, 选 "编译运行")
echo   - 语法检查 (Ctrl+B, 选 "语法检查")
echo   - JIT/AOT 编译 (Ctrl+B 更多选项)
echo.
pause
