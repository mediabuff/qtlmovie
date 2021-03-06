;-----------------------------------------------------------------------------
; 
;  Copyright (c) 2013-2017, Thierry Lelegard
;  All rights reserved.
; 
;  Redistribution and use in source and binary forms, with or without
;  modification, are permitted provided that the following conditions are met:
; 
;  1. Redistributions of source code must retain the above copyright notice,
;     this list of conditions and the following disclaimer. 
;  2. Redistributions in binary form must reproduce the above copyright
;     notice, this list of conditions and the following disclaimer in the
;     documentation and/or other materials provided with the distribution. 
; 
;  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
;  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
;  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
;  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
;  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
;  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
;  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
;  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
;  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
;  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
;  THE POSSIBILITY OF SUCH DAMAGE.
; 
;-----------------------------------------------------------------------------
; 
;  NSIS script to build the QtlMovie binary installer for Windows.
;
;  Do not invoke NSIS directly, use build-installer.ps1 to build the installer.
;  If Win64 is defined, generate a 64-bit installer (default: 32-bit installer).
;
;  This script uses the following required symbols:
;  - ProductVersion: QtlMovie version.
;  - RootDir: Project top-level directory.
;  - BuildDir: Build top-level directory.
;  - InstallerDir: Directory for binary installers.
;  - Win64: If defined, generate an installer for 64-bit systems.
;
;-----------------------------------------------------------------------------

Name "QtlMovie"

!verbose push
!verbose 0
!include "MUI2.nsh"
!include "WinMessages.nsh"
!include "x64.nsh"
!verbose pop

; Installer file name.
!ifdef Win64
    OutFile "${InstallerDir}\QtlMovie-Win64-${ProductVersion}.exe"
!else
    OutFile "${InstallerDir}\QtlMovie-Win32-${ProductVersion}.exe"
!endif

; Registry entry for product info and uninstallation info.
!define ProductKey "Software\QtlMovie"
!define UninstallKey "Software\Microsoft\Windows\CurrentVersion\Uninstall\QtlMovie"

; Use XP manifest.
XPStyle on

; Request administrator privileges for Windows Vista and higher.
RequestExecutionLevel admin

; "Modern User Interface" (MUI) settings
!define MUI_ABORTWARNING
!define MUI_ICON "${RootDir}\src\QtlMovie\images\qtlmovie.ico"
!define MUI_UNICON "${RootDir}\src\QtlMovie\images\qtlmovie.ico"

; Language selection dialog
!insertmacro MUI_RESERVEFILE_LANGDLL

; Remember the installer language for the current user in the registry.
!define MUI_LANGDLL_REGISTRY_ROOT "HKCU"
!define MUI_LANGDLL_REGISTRY_KEY "${ProductKey}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

; Default installation folder
!ifdef Win64
    InstallDir "$PROGRAMFILES64\QtlMovie"
!else
    InstallDir "$PROGRAMFILES\QtlMovie"
!endif

; Get installation folder from registry if available from a previous installation.
InstallDirRegKey HKLM "${ProductKey}" "InstallDir"

; Installer pages
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Languages
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "French"

;-----------------------------------------------------------------------------
; Initialization functions
;-----------------------------------------------------------------------------

; Installation initialization.
function .onInit

    ; Language selection.
    !insertmacro MUI_LANGDLL_DISPLAY

    ; In 64-bit installers, don't use registry redirection. Also prevent execution
    ; of 64-bit installers on 32-bit systems (the installer itself is 32-bit and
    ; can run on 32-bit systems but it contains some 64-bit executables).
    !ifdef Win64
        ${If} ${RunningX64}
            SetRegView 64
        ${Else}
            MessageBox MB_OK|MB_ICONSTOP \
                "This is a 64-bit version of QtlMovie.$\r$\n\
                You have a 32-bit version of Windows.$\r$\n\
                Please use a 32-bit version of QtlMovie on this system."
            Quit
        ${EndIf}
    !endif
functionEnd

; Uninstallation initialization.
function un.onInit

    ; Retrieve the installer language.
    !insertmacro MUI_UNGETLANGUAGE

    ; In 64-bit installers, don't use registry redirection.
    !ifdef Win64
        ${If} ${RunningX64}
            SetRegView 64
        ${EndIf}
    !endif
functionEnd

;-----------------------------------------------------------------------------
; Installation section
;-----------------------------------------------------------------------------

Section "Install"

    ; Work on "all users" context, not current user.
    SetShellVarContext all

    ; Delete obsolete files from previous versions.
    Delete "$INSTDIR\qt.conf"
    Delete "$INSTDIR\icuin51.dll"
    Delete "$INSTDIR\icuuc51.dll"
    Delete "$INSTDIR\icudt51.dll"
    Delete "$INSTDIR\fonts\fonts.win.conf"
    Delete "$INSTDIR\plugins\platforms\qwindows.dll"
    RMDir  "$INSTDIR\plugins\platforms"
    RMDir  "$INSTDIR\plugins"
    Delete "$INSTDIR\help\qtlmovie.html"
    Delete "$INSTDIR\help\qtlmovie_fr.html"
    Delete "$INSTDIR\wintools\telxcc.exe"
    Delete "$INSTDIR\wintools\telxcc.txt"
    Delete "$INSTDIR\wintools\dvddecrypter.exe"
    Delete "$INSTDIR\wintools\dvddecrypter.txt"

    ; Install product files.
    SetOutPath "$INSTDIR"
    File /r "${BuildDir}\*"

    ; Create a shortcut in start menu.
    CreateShortCut "$SMPROGRAMS\QtlMovie.lnk" "$INSTDIR\QtlMovie.exe"

    ; Store installation folder in registry.
    WriteRegStr HKLM "${ProductKey}" "InstallDir" $INSTDIR

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\QtlMovieUninstall.exe"

    ; Declare uninstaller in "Add/Remove Software" control panel
    WriteRegStr HKLM "${UninstallKey}" "DisplayName" "QtlMovie"
    WriteRegStr HKLM "${UninstallKey}" "DisplayVersion" "${ProductVersion}"
    WriteRegStr HKLM "${UninstallKey}" "DisplayIcon" "$INSTDIR\QtlMovieUninstall.exe"
    WriteRegStr HKLM "${UninstallKey}" "UninstallString" "$INSTDIR\QtlMovieUninstall.exe"

SectionEnd

;-----------------------------------------------------------------------------
; Uninstallation section
;-----------------------------------------------------------------------------

Section "Uninstall"

    ; Work on "all users" context, not current user.
    SetShellVarContext all

    ; Delete product files.
    RMDir /r "$INSTDIR"

    ; Delete start menu entries  
    Delete "$SMPROGRAMS\QtlMovie.lnk"

    ; Delete registry entries
    DeleteRegKey HKCU "${ProductKey}"
    DeleteRegKey HKLM "${ProductKey}"
    DeleteRegKey HKLM "${UninstallKey}"

SectionEnd
