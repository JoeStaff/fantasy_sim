# Assets Directory

This directory contains all game assets used by Fantasy Sim.

## Directory Structure

```
assets/
├── fonts/          # Font files (.ttf, .otf, etc.)
├── textures/       # Image files (.png, .jpg, .bmp, etc.)
├── sounds/         # Sound effect files (.wav, .ogg, .mp3, etc.)
├── music/          # Music files (.ogg, .mp3, .wav, etc.)
├── data/           # Game data files (JSON, XML, binary data, etc.)
└── shaders/        # Shader files (.vert, .frag, .glsl, etc.)
```

## Usage

### Fonts
Font files should be placed in the `fonts/` directory. The game will load fonts from this location for menu rendering and UI text.

**Recommended formats:**
- `.ttf` (TrueType Font)
- `.otf` (OpenType Font)

### Textures
Image files for sprites, UI elements, backgrounds, etc. should be placed in the `textures/` directory.

**Supported formats:**
- `.png` (recommended for transparency)
- `.jpg` / `.jpeg`
- `.bmp`
- Other formats supported by SDL2_image

### Sounds
Short sound effects (UI clicks, notifications, etc.) should be placed in the `sounds/` directory.

**Recommended formats:**
- `.wav` (uncompressed, low latency)
- `.ogg` (compressed, good quality)

### Music
Background music and longer audio tracks should be placed in the `music/` directory.

**Recommended formats:**
- `.ogg` (compressed, good quality)
- `.mp3` (widely supported)

### Data
Game data files (save data templates, configuration overrides, etc.) should be placed in the `data/` directory.

**Supported formats:**
- `.json`
- `.xml`
- Binary data files

### Shaders
Shader files for custom rendering effects should be placed in the `shaders/` directory.

**Supported formats:**
- `.vert` (vertex shaders)
- `.frag` (fragment shaders)
- `.glsl` (generic GLSL shaders)

## Asset Loading

Assets are loaded at runtime from this directory. The game will look for assets relative to the executable or using a configured asset path.

## Version Control

Asset files are tracked in git. Large binary files may be excluded via `.gitignore` if they exceed reasonable size limits. Consider using Git LFS for large assets if needed.
