# Jen Project Build Guide

## 🚀 Single Command Build

```bash
# First, activate Emscripten (one-time setup per terminal session)
source ~/emsdk/emsdk_env.sh  # or your Emscripten path

# Then build everything with one command:
make

# Output will be in lux_react/src/lux.js
```

The `make` command automatically:
1. ✅ Verifies Emscripten is active
2. 📁 Creates directories if needed  
3. 🔧 Builds dependencies if needed (first time: 10-20 min)
4. ⚡ Compiles project (incremental builds)
5. 🎉 Outputs `lux_react/src/lux.js`

## Build Targets

- `make` - **Does everything automatically!** (recommended)
- `make emscripten` - Check Emscripten environment
- `make setup` - Create project directory structure manually
- `make deps` - Build external dependencies manually
- `make no-deps` - Build without video recording support (faster, smaller)
- `make clean` - Remove build files
- `make deep-clean` - Remove build files and external libraries
- `make help` - Show all available targets

## Requirements

- **Emscripten SDK** - Must be activated in your terminal session
- Git (for downloading dependencies)
- Standard build tools (make, etc.)

## Emscripten Setup

### Install Emscripten (first time only)
```bash
# Clone and install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
```

### Activate Emscripten (each terminal session)
```bash
# Activate in your current terminal (choose your path)
source ~/emsdk/emsdk_env.sh
# or
source ~/Documents/emsdk/emsdk_env.sh
# or  
source /opt/emsdk/emsdk_env.sh
```

### Verify Emscripten
```bash
make emscripten  # Check if properly activated
```

## What Happens During Build

### First Time (Complete Setup)
1. **Emscripten Check** - Verifies Emscripten is activated
2. **Directory Setup** - Creates `external/`, `web_build/`, `lux_react/src/`
3. **Dependency Download** - Clones x264 and FFmpeg repositories
4. **Dependency Build** - Compiles x264 and FFmpeg (10-20 minutes)
5. **Project Build** - Compiles all C++ source files and links final output

### Subsequent Builds (Incremental)
1. **Quick Checks** - Verifies setup and dependencies
2. **Incremental Compilation** - Only recompiles changed files
3. **Fast Linking** - Updates final output (usually under 30 seconds)

## Features

- **Incremental builds** - Only recompiles changed files
- **Automatic dependency detection** - Knows when FFmpeg is available
- **Optimized compilation** - Different optimization levels for different files
- **Clean error messages** - Clear feedback on what's happening
- **Single command** - No complex scripts to remember

## Troubleshooting

**"Emscripten not found"**
```bash
# Activate Emscripten first
source /path/to/emsdk/emsdk_env.sh
```

**"FFmpeg dependencies not found"**
```bash
# Build dependencies first
make deps
```

**Build fails**
```bash
# Clean and try again
make clean
make
```

## File Structure

```
├── Makefile              # Complete build system
├── src/                  # C++ source files
├── web_build/           # Compiled object files
├── external/build/      # External libraries (FFmpeg, x264)
└── lux_react/src/       # Output directory
    └── lux.js          # Final WebAssembly module
``` 