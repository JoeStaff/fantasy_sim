@echo off
REM Run script for Windows
REM This script runs the built executable

setlocal

set PROJECT_ROOT=%~dp0..
set BUILD_DIR=%PROJECT_ROOT%\build
set EXE_PATH=%BUILD_DIR%\bin\Release\FantasySim.exe

REM Check if executable exists in standard location
if not exist "%EXE_PATH%" (
    set EXE_PATH=%BUILD_DIR%\Release\FantasySim.exe
)

REM Check if executable exists
if not exist "%EXE_PATH%" (
    echo [ERROR] Executable not found.
    echo [INFO] Expected locations:
    echo   %BUILD_DIR%\bin\Release\FantasySim.exe
    echo   %BUILD_DIR%\Release\FantasySim.exe
    echo.
    echo [INFO] Please build the project first using build.bat
    exit /b 1
)

echo [INFO] Running Fantasy Sim...
echo Executable: %EXE_PATH%
echo.

cd /d "%PROJECT_ROOT%"
"%EXE_PATH%"

endlocal




