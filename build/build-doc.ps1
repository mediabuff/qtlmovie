﻿#-----------------------------------------------------------------------------
# 
#  Copyright (c) 2013, Thierry Lelegard
#  All rights reserved.
# 
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
# 
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer. 
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution. 
# 
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
# 
#-----------------------------------------------------------------------------
# 
#  Windows PowerShell script to build the documentation using Doxygen.
# 
#  See http://www.doxygen.org/download.html for Doxygen binary installers
#  for Windows. The installer updates %Path% to include the Doxygen binary
#  directory. Thus, doxygen is available as a simple command.
# 
#-----------------------------------------------------------------------------

<#
 .SYNOPSIS

  Build the project documentation using Doxygen.

 .DESCRIPTION

  Build the project documentation using Doxygen.

 .PARAMETER NoOpen

  Do not open the generated documentation. By default, run the default HTML
  browser on the generated documentation hom page.

 .PARAMETER NoPause

  Do not wait for the user to press <enter> at end of execution. By default,
  execute a "pause" instruction at the end of execution, which is useful
  when the script was run from Windows Explorer.
#>
param(
    [switch]$NoOpen = $false,
    [switch]$NoPause = $false
)

Push-Location $PSScriptRoot
Write-Host "Running Doxygen..."
doxygen
Pop-Location

if (-not $NoOpen) {
    $RootDir = (Split-Path -Parent $PSScriptRoot)
    $HtmlIndex = (Join-Path (Join-Path (Join-Path $RootDir "build-doxygen") "html") "index.html")
    if (Test-Path $HtmlIndex) {
        Invoke-Item $HtmlIndex
    }
}

if (-not $NoPause) {
    pause
}
