; CPLang v0.9.3 Windows x64 Installer

!include "MUI2.nsh"

Name "CPLang v0.9.3"
OutFile "C:\cplang\_release\CPLang-v0.9.3-Windows-x64-Setup.exe"
InstallDir "$PROGRAMFILES64\CPLang"
InstallDirRegKey HKLM "Software\CPLang" "InstallDir"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
SetCompressorDictSize 64

!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "SimpChinese"

Section "CPLang v0.9.3" SecCore
    SetOutPath "$INSTDIR"
    File "C:\cplang\_release\CPLang-v0.9.3\VERSION"

    SetOutPath "$INSTDIR\bin"
    File "C:\cplang\_release\CPLang-v0.9.3\bin\cplang.exe"
    File "C:\cplang\_release\CPLang-v0.9.3\bin\cpkg.exe"
    File "C:\cplang\_release\CPLang-v0.9.3\bin\cplang_repl.exe"
    File "C:\cplang\_release\CPLang-v0.9.3\bin\cplsp.exe"

    SetOutPath "$INSTDIR\stdlib"
    File /nonfatal "C:\cplang\_release\CPLang-v0.9.3\stdlib\*.cpp"

    SetOutPath "$INSTDIR\include\stdlib"
    File /nonfatal "C:\cplang\_release\CPLang-v0.9.3\include\stdlib\*.hpp"
    File /nonfatal "C:\cplang\_release\CPLang-v0.9.3\include\stdlib\*.h"

    SetOutPath "$INSTDIR\include\vm"
    File /nonfatal "C:\cplang\_release\CPLang-v0.9.3\include\vm\*.hpp"

    SetOutPath "$INSTDIR\docs"
    File /nonfatal "C:\cplang\_release\CPLang-v0.9.3\docs\README.md"

    SetOutPath "$INSTDIR\tools"
    File /nonfatal "C:\cplang\_release\CPLang-v0.9.3\tools\*.vsix"

    WriteRegStr HKLM "Software\CPLang" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\CPLang" "Version" "0.9.2"
    WriteUninstaller "$INSTDIR\uninstall.exe"

    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CPLang" "DisplayName" "CPLang v0.9.3"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CPLang" "DisplayVersion" "0.9.2"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CPLang" "Publisher" "CPLang Community"
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CPLang" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CPLang" "DisplayIcon" "$INSTDIR\bin\cplang.exe"
    WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CPLang" "NoModify" 1
    WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CPLang" "NoRepair" 1
SectionEnd

SectionGroup /e "Examples" SecExamples
    Section "800+ Example Programs" SecEx
        SetOutPath "$INSTDIR\examples"
        File /r "C:\cplang\_release\CPLang-v0.9.3\examples\800示例\*"
    SectionEnd
SectionGroupEnd

Section "Uninstall"
    RMDir /r "$INSTDIR\bin"
    RMDir /r "$INSTDIR\stdlib"
    RMDir /r "$INSTDIR\include"
    RMDir /r "$INSTDIR\docs"
    RMDir /r "$INSTDIR\tools"
    RMDir /r "$INSTDIR\examples"
    Delete "$INSTDIR\VERSION"
    Delete "$INSTDIR\uninstall.exe"
    RMDir "$INSTDIR"
    DeleteRegKey HKLM "Software\CPLang"
    DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\CPLang"
SectionEnd
