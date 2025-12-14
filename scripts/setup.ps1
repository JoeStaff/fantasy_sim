# Fantasy Sim - Environment Setup Script (Windows PowerShell)
# This script sets up vcpkg and installs all required dependencies

param(
    [string]$VcpkgPath = "",
    [switch]$SkipVcpkgInstall = $false
)

$ErrorActionPreference = "Stop"

Write-Host "=== Fantasy Sim Environment Setup ===" -ForegroundColor Cyan
Write-Host ""

# Get script directory
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir

# Determine vcpkg path
if ($VcpkgPath -eq "") {
    # Check common locations
    $CommonPaths = @(
        "$env:USERPROFILE\vcpkg",
        "$env:USERPROFILE\Documents\vcpkg",
        "C:\vcpkg",
        "$ProjectRoot\..\vcpkg",
        "$ProjectRoot\vcpkg"
    )
    
    foreach ($path in $CommonPaths) {
        if (Test-Path "$path\vcpkg.exe") {
            $VcpkgPath = $path
            Write-Host "Found vcpkg at: $VcpkgPath" -ForegroundColor Green
            break
        }
    }
    
    if ($VcpkgPath -eq "") {
        # Default location
        $VcpkgPath = "$env:USERPROFILE\vcpkg"
    }
}

Write-Host "Vcpkg path: $VcpkgPath" -ForegroundColor Yellow

# Install vcpkg if needed
if (-not $SkipVcpkgInstall) {
    if (-not (Test-Path "$VcpkgPath\vcpkg.exe")) {
        Write-Host ""
        Write-Host "vcpkg not found. Installing vcpkg..." -ForegroundColor Yellow
        
        $VcpkgParent = Split-Path -Parent $VcpkgPath
        if (-not (Test-Path $VcpkgParent)) {
            New-Item -ItemType Directory -Path $VcpkgParent -Force | Out-Null
        }
        
        # Clone vcpkg
        Write-Host "Cloning vcpkg repository..." -ForegroundColor Yellow
        Push-Location $VcpkgParent
        git clone https://github.com/Microsoft/vcpkg.git (Split-Path -Leaf $VcpkgPath)
        Pop-Location
        
        if (-not (Test-Path "$VcpkgPath\vcpkg.exe")) {
            Write-Host "Bootstraping vcpkg..." -ForegroundColor Yellow
            Push-Location $VcpkgPath
            .\bootstrap-vcpkg.bat
            Pop-Location
        }
    } else {
        Write-Host "vcpkg already installed." -ForegroundColor Green
    }
}

# Verify vcpkg exists
if (-not (Test-Path "$VcpkgPath\vcpkg.exe")) {
    Write-Host "ERROR: vcpkg not found at $VcpkgPath" -ForegroundColor Red
    Write-Host "Please install vcpkg manually or specify the path with -VcpkgPath" -ForegroundColor Red
    exit 1
}

# Integrate vcpkg with Visual Studio (one-time setup)
Write-Host ""
Write-Host "Integrating vcpkg with Visual Studio..." -ForegroundColor Yellow
Push-Location $VcpkgPath
.\vcpkg integrate install
Pop-Location

# Install dependencies
Write-Host ""
Write-Host "Installing dependencies..." -ForegroundColor Yellow
Write-Host "This may take several minutes..." -ForegroundColor Yellow

$Dependencies = @(
    "sdl2",
    "sdl2-image",
    "sdl2-ttf",
    "nlohmann-json"
)

foreach ($dep in $Dependencies) {
    Write-Host "Installing $dep..." -ForegroundColor Cyan
    Push-Location $VcpkgPath
    .\vcpkg install $dep --triplet x64-windows
    $exitCode = $LASTEXITCODE
    Pop-Location
    
    if ($exitCode -ne 0) {
        Write-Host "ERROR: Failed to install $dep" -ForegroundColor Red
        exit 1
    }
}

# Save vcpkg path to a config file for build script
$ConfigFile = "$ProjectRoot\.vcpkg-path"
$VcpkgPath | Out-File -FilePath $ConfigFile -Encoding ASCII -NoNewline
Write-Host ""
Write-Host "Saved vcpkg path to $ConfigFile" -ForegroundColor Green

# Verify CMake
Write-Host ""
Write-Host "Checking for CMake..." -ForegroundColor Yellow
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmake) {
    $cmakeVersion = cmake --version | Select-Object -First 1
    Write-Host "Found: $cmakeVersion" -ForegroundColor Green
} else {
    Write-Host "WARNING: CMake not found in PATH" -ForegroundColor Yellow
    Write-Host "Please install CMake 3.20 or later from https://cmake.org/download/" -ForegroundColor Yellow
}

# Verify compiler
Write-Host ""
Write-Host "Checking for C++ compiler..." -ForegroundColor Yellow
$cl = Get-Command cl -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "Found: MSVC compiler" -ForegroundColor Green
} else {
    Write-Host "WARNING: MSVC compiler not found in PATH" -ForegroundColor Yellow
    Write-Host "Please install Visual Studio with C++ desktop development workload" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== Setup Complete ===" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Run: .\scripts\build.ps1" -ForegroundColor White
Write-Host "  2. Or manually:" -ForegroundColor White
Write-Host "     mkdir build" -ForegroundColor Gray
Write-Host "     cd build" -ForegroundColor Gray
Write-Host "     cmake .. -DCMAKE_TOOLCHAIN_FILE=`"$VcpkgPath\scripts\buildsystems\vcpkg.cmake`"" -ForegroundColor Gray
Write-Host "     cmake --build . --config Release" -ForegroundColor Gray
Write-Host ""
