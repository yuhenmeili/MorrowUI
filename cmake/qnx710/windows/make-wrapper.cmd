@echo off
setlocal
set "MORROW_QNX_SDK_ROOT="

if defined QNX_SDK_ROOT if exist "%QNX_SDK_ROOT%\qnxsdp-env.bat" set "MORROW_QNX_SDK_ROOT=%QNX_SDK_ROOT%"
if not defined MORROW_QNX_SDK_ROOT if defined QNX_BASE if exist "%QNX_BASE%\qnxsdp-env.bat" set "MORROW_QNX_SDK_ROOT=%QNX_BASE%"

for %%D in ("D:\WorkTools\QNX\qnx710" "D:\WorkTools\QNX\sdp710" "D:\WorkSpace\Client\qnx710" "D:\WorkSpace\Client\sdp710" "D:\QNX\qnx710" "D:\QNX\sdp710" "C:\QNX\qnx710" "C:\QNX\sdp710" "D:\qnx710" "D:\sdp710" "C:\qnx710" "C:\sdp710") do (
    if not defined MORROW_QNX_SDK_ROOT if exist "%%~fD\qnxsdp-env.bat" set "MORROW_QNX_SDK_ROOT=%%~fD"
)

if not exist "%MORROW_QNX_SDK_ROOT%\qnxsdp-env.bat" (
    echo [morrow.gui] QNX 7.1 SDK env script not found. Set QNX_SDK_ROOT or QNX_BASE, or install qnx710 or sdp710 in a standard location. 1^>^&2
    exit /b 1
)

call "%MORROW_QNX_SDK_ROOT%\qnxsdp-env.bat" >nul
set "RC=%ERRORLEVEL%"
@echo off
if not "%RC%"=="0" exit /b %RC%

set "QNX_SDK_ROOT=%MORROW_QNX_SDK_ROOT%"

"%MORROW_QNX_SDK_ROOT%\host\win64\x86_64\usr\bin\make.exe" %*
exit /b %errorlevel%

