@echo off
cd /d D:\CPLANG
set "CL=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx86\x64\cl.exe"
echo int main(){return 0;}>_test_link2.c
"%CL%" /nologo _test_link2.c /Fe_test_link2.exe
if exist _test_link2.exe (
    _test_link2.exe
    echo [LINK OK]
    del _test_link2.c _test_link2.exe 2>nul
) else (
    echo [LINK FAILED]
)
