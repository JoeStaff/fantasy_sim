@echo off
REM Setup environment script for Windows
REM This script sets up vcpkg and installs dependencies

setlocal enabledelayedexpansion

echo ========================================
echo Fantasy Sim - Environment Setup
echo ========================================
echo.

REM Check if vcpkg directory exists
set VCPKG_DIR=%~dp0..\vcpkg
if exist "%VCPKG_DIR%" (
    echo [INFO] vcpkg directory found at: %VCPKG_DIR%
) else (
    echo [INFO] vcpkg not found. Cloning vcpkg...
    cd /d "%~dp0.."
    git clone https://github.com/Microsoft/vcpkg.git
    if errorlevel 1 (
        echo [ERROR] Failed to clone vcpkg. Make sure git is installed.
        exit /b 1
    )
    echo [SUCCESS] vcpkg cloned successfully
)

REM Check if vcpkg executable exists
set VCPKG_EXE=%VCPKG_DIR%\vcpkg.exe
if not exist "%VCPKG_EXE%" (
    echo [INFO] Bootstrapping vcpkg...
    cd /d "%VCPKG_DIR%"
    call bootstrap-vcpkg.bat
    if errorlevel 1 (
        echo [ERROR] Failed to bootstrap vcpkg.
        exit /b 1
    )
    echo [SUCCESS] vcpkg bootstrapped successfully
) else (
    echo [INFO] vcpkg already bootstrapped
)

REM Integrate vcpkg with CMake (one-time setup)
echo [INFO] Integrating vcpkg with CMake...
cd /d "%VCPKG_DIR%"
call vcpkg integrate install
if errorlevel 1 (
    echo [WARNING] vcpkg integration may have failed, but continuing...
) else (
    echo [SUCCESS] vcpkg integrated with CMake
)

REM Install dependencies
echo.
echo [INFO] Installing dependencies (this may take a while)...
echo Installing packages from vcpkg.json: sdl2 sdl2-image sdl2-ttf nlohmann-json
echo Using manifest mode (packages defined in vcpkg.json)
cd /d "%~dp0.."
call "%VCPKG_DIR%\vcpkg.exe" install --triplet x64-windows
if errorlevel 1 (
    echo [ERROR] Failed to install dependencies.
    exit /b 1
)

echo.
echo [SUCCESS] Environment setup complete!
echo.
echo vcpkg location: %VCPKG_DIR%
echo vcpkg toolchain: %VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake
echo.
echo You can now run build.bat to build the project.
echo.

endlocal



