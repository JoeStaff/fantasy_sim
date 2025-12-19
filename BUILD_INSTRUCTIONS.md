# Build Instructions

## Prerequisites

1. **CMake 3.20+** - Download from https://cmake.org/download/
2. **C++20 Compiler**:
   - Windows: Visual Studio 2019 or later (with C++ desktop development)
   - Linux: GCC 10+ or Clang 12+
   - macOS: Xcode 12+ or Clang 12+
3. **Git** - For cloning vcpkg
4. **vcpkg** - Will be automatically set up by the setup script

## Quick Start (Recommended)

### Windows

1. **Setup environment** (first time only):
   ```batch
   scripts\setup_env.bat
   ```

2. **Build project**:
   ```batch
   scripts\build.bat
   ```

3. **Run executable**:
   ```batch
   scripts\run.bat
   ```

### Linux/Mac

1. **Make scripts executable** (first time only):
   ```bash
   chmod +x scripts/*.sh
   ```

2. **Setup environment** (first time only):
   ```bash
   ./scripts/setup_env.sh
   ```

3. **Build project**:
   ```bash
   ./scripts/build.sh
   ```

4. **Run executable**:
   ```bash
   ./scripts/run.sh
   ```

## Manual Setup (Alternative)

If you prefer to set up manually:

### Setup vcpkg

1. Clone vcpkg:
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
```

2. Bootstrap vcpkg:
   - Windows: `.\bootstrap-vcpkg.bat`
   - Linux/Mac: `./bootstrap-vcpkg.sh`

3. Install dependencies:
```bash
vcpkg install sdl2 sdl2-image sdl2-ttf nlohmann-json
```

4. Integrate vcpkg with CMake (one-time setup):
```bash
vcpkg integrate install
```

### Building Manually

#### Windows (Visual Studio)

```powershell
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

The executable will be in `build/bin/Release/FantasySim.exe`

#### Linux

```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

The executable will be in `build/bin/FantasySim`

#### macOS

```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

The executable will be in `build/bin/FantasySim`

## Running

After building, run the executable:

- **Windows**: `build\bin\Release\FantasySim.exe` or use `scripts\run.bat`
- **Linux/Mac**: `./build/bin/FantasySim` or use `./scripts/run.sh`

The window should open and close when you:
- Click the window's close button
- Press Escape

## Troubleshooting

### CMake can't find SDL2

Make sure you've:
1. Installed SDL2 via vcpkg: `vcpkg install sdl2 sdl2-image sdl2-ttf`
2. Set the vcpkg toolchain: `-DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake`

### Linker errors

Make sure all vcpkg packages are installed and the toolchain file is set correctly.

### Runtime errors (DLL not found on Windows)

Copy SDL2 DLLs to the executable directory, or add vcpkg's bin directory to PATH.





