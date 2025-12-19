# Build Scripts

This directory contains scripts to automate the build process for Fantasy Sim.

## Scripts

### Windows (.bat files)

- **setup_env.bat** - Sets up vcpkg and installs dependencies (run once)
- **build.bat** - Configures and builds the project
- **run.bat** - Runs the built executable

### Linux/Mac (.sh files)

- **setup_env.sh** - Sets up vcpkg and installs dependencies (run once)
- **build.sh** - Configures and builds the project
- **run.sh** - Runs the built executable

## Usage

### First Time Setup

1. **Windows**:
   ```batch
   scripts\setup_env.bat
   ```

2. **Linux/Mac**:
   ```bash
   chmod +x scripts/*.sh
   ./scripts/setup_env.sh
   ```

This will:
- Clone vcpkg (if not present)
- Bootstrap vcpkg
- Integrate vcpkg with CMake
- Install all required dependencies (SDL2, nlohmann-json, etc.)

### Building

1. **Windows**:
   ```batch
   scripts\build.bat
   ```

2. **Linux/Mac**:
   ```bash
   ./scripts/build.sh
   ```

### Running

1. **Windows**:
   ```batch
   scripts\run.bat
   ```

2. **Linux/Mac**:
   ```bash
   ./scripts/run.sh
   ```

## Notes

- The `vcpkg` directory will be created in the project root (it's gitignored)
- Dependencies are installed for the appropriate platform (x64-windows, x64-linux, x64-osx)
- The build directory is created automatically
- All scripts use Release configuration by default

## Troubleshooting

### Scripts fail with "command not found"

- **Windows**: Make sure you're running from Command Prompt or PowerShell
- **Linux/Mac**: Make sure scripts are executable: `chmod +x scripts/*.sh`

### vcpkg not found

- Run `setup_env.bat` or `setup_env.sh` first
- Make sure git is installed and in your PATH

### CMake not found

- Install CMake from https://cmake.org/download/
- Add CMake to your system PATH

### Build fails

- Make sure all dependencies were installed successfully
- Check that the vcpkg toolchain path is correct
- Try cleaning the build directory and rebuilding





