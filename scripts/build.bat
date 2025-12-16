@echo off
REM Build script for Windows
REM This script configures and builds the project using CMake

setlocal enabledelayedexpansion

echo ========================================
echo Fantasy Sim - Build Script
echo ========================================
echo.

REM Set vcpkg directory (relative to script location)
set VCPKG_DIR=%~dp0..\vcpkg
set VCPKG_TOOLCHAIN=%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake

REM Check if vcpkg exists
if not exist "%VCPKG_DIR%" (
    echo [ERROR] vcpkg not found at: %VCPKG_DIR%
    echo [INFO] Please run setup_env.bat first to set up the environment.
    exit /b 1
)

REM Check if vcpkg toolchain file exists
if not exist "%VCPKG_TOOLCHAIN%" (
    echo [ERROR] vcpkg toolchain file not found at: %VCPKG_TOOLCHAIN%
    echo [INFO] Please run setup_env.bat first to set up the environment.
    exit /b 1
)

REM Check if vcpkg packages are installed (manifest mode creates vcpkg_installed directory)
set VCPKG_INSTALLED=%~dp0..\vcpkg_installed
if not exist "%VCPKG_INSTALLED%" (
    echo [WARNING] vcpkg_installed directory not found.
    echo [INFO] Packages may not be installed. Run setup_env.bat to install dependencies.
    echo [INFO] Continuing anyway...
)

REM Check if CMake is available
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found in PATH.
    echo [INFO] Please install CMake and add it to your PATH.
    exit /b 1
)

REM Get project root directory
set PROJECT_ROOT=%~dp0..
set BUILD_DIR=%PROJECT_ROOT%\build

REM Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    echo [INFO] Creating build directory...
    mkdir "%BUILD_DIR%"
)

REM Configure CMake
echo [INFO] Configuring CMake...
echo [INFO] Using vcpkg toolchain: %VCPKG_TOOLCHAIN%
cd /d "%BUILD_DIR%"
cmake .. -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%" -DCMAKE_BUILD_TYPE=Release -DVCPKG_TARGET_TRIPLET=x64-windows
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    echo [INFO] Make sure vcpkg packages are installed by running setup_env.bat
    exit /b 1
)

REM Build project
echo.
echo [INFO] Building project (Release configuration)...
cmake --build . --config Release
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo.
echo [SUCCESS] Build complete!
echo.
echo Executable location:
if exist "%BUILD_DIR%\bin\Release\FantasySim.exe" (
    echo   %BUILD_DIR%\bin\Release\FantasySim.exe
) else if exist "%BUILD_DIR%\Release\FantasySim.exe" (
    echo   %BUILD_DIR%\Release\FantasySim.exe
) else (
    echo   (Check build directory for executable)
)
echo.

endlocal



