@echo off
REM ===== 游戏启动器 =====
REM 这个 bat 和 cplang.exe + game.cp 一起打包成自解压 exe
setlocal
cd /d "%~dp0"
start /wait "" "cplang.exe" -c "game.cp"
