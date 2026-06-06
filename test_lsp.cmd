@echo off
cd /d D:\CPLANG\tools\vscode-cp
set "NODE=C:\Program Files\nodejs\node.exe"
set "MSG={"jsonrpc":"2.0","id":1,"method":"initialize","params":{"capabilities":{},"processId":null,"rootUri":null}}"
set "LEN=159"
echo Content-Length: %LEN%| "%NODE%" cplsp.js 2>&1
echo.
echo [Exit code: %ERRORLEVEL%]
