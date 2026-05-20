@echo off
setlocal EnableExtensions EnableDelayedExpansion
set "MORROW_QNX_SDK_ROOT="
set "MORROW_RAW_ARGS=%*"
set "MORROW_QNX_ARGS="

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

echo(!MORROW_RAW_ARGS!| findstr /I /C:"--sysroot" /C:"-isysroot" >nul
if errorlevel 1 goto run_qcc_raw

:collect_args
if "%~1"=="" goto run_qcc
if /I "%~1"=="--sysroot" (
    shift
    if not "%~1"=="" shift
    goto collect_args
)
if /I "%~1"=="-isysroot" (
    shift
    if not "%~1"=="" shift
    goto collect_args
)
set "MORROW_ARG=%~1"
if /I "!MORROW_ARG:~0,10!"=="--sysroot=" (
    shift
    goto collect_args
)
if defined MORROW_QNX_ARGS (
    set "MORROW_QNX_ARGS=!MORROW_QNX_ARGS! "%~1""
) else (
    set "MORROW_QNX_ARGS="%~1""
)
shift
goto collect_args

:run_qcc
"%MORROW_QNX_SDK_ROOT%\host\win64\x86_64\usr\bin\qcc.exe" !MORROW_QNX_ARGS!
exit /b %errorlevel%

:run_qcc_raw
"%MORROW_QNX_SDK_ROOT%\host\win64\x86_64\usr\bin\qcc.exe" %*
exit /b %errorlevel%

