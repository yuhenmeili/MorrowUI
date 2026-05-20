@echo off
setlocal
set "MORROW_QNX_SDK_ROOT="

if defined QNX_SDK_ROOT if exist "%QNX_SDK_ROOT%\qnxsdp-env.bat" set "MORROW_QNX_SDK_ROOT=%QNX_SDK_ROOT%"
if not defined MORROW_QNX_SDK_ROOT if defined QNX_BASE if exist "%QNX_BASE%\qnxsdp-env.bat" set "MORROW_QNX_SDK_ROOT=%QNX_BASE%"

for %%D in ("D:\WorkTools\QNX\qnx800" "D:\WorkSpace\Client\qnx800" "D:\QNX\qnx800" "C:\QNX\qnx800" "D:\qnx800" "C:\qnx800") do (
    if not defined MORROW_QNX_SDK_ROOT if exist "%%~fD\qnxsdp-env.bat" set "MORROW_QNX_SDK_ROOT=%%~fD"
)

if not exist "%MORROW_QNX_SDK_ROOT%\qnxsdp-env.bat" (
    echo [morrow.gui] QNX 8.0 SDK env script not found. Set QNX_SDK_ROOT or QNX_BASE, or install qnx800 in a standard location. 1^>^&2
    exit /b 1
)

call "%MORROW_QNX_SDK_ROOT%\qnxsdp-env.bat" >nul
set "RC=%ERRORLEVEL%"
@echo off
if not "%RC%"=="0" exit /b %RC%

set "QNX_SDK_ROOT=%MORROW_QNX_SDK_ROOT%"

"%MORROW_QNX_SDK_ROOT%\host\win64\x86_64\usr\bin\make.exe" %*
exit /b %errorlevel%

