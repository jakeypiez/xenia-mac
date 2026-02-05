# Xenia macOS Fork - Copilot Instructions

## Project Overview

Xenia is an open-source Xbox 360 emulator, originally developed by Ben Vanik. This fork targets **macOS** with support for **Apple Silicon (ARM64)** and **Intel x86_64** architectures, using **Metal** as the graphics backend instead of Direct3D12/Vulkan.

### Key Characteristics
- **Language**: C++20 with some Objective-C++ (`.mm` files) for Metal/macOS integration
- **Build System**: Premake5 generating Xcode projects
- **Target Platforms**: macOS 15.0+ (Sequoia)
- **Graphics API**: Metal (Apple's GPU API)
- **CPU Backend**: ARM64 backend using Oaknut JIT library, or x64 backend using Xbyak

## Architecture Overview

### Directory Structure

```
src/xenia/
├── app/           # Main application entry point (xenia_main.cc, emulator_window.cc)
├── apu/           # Audio Processing Unit emulation (audio systems)
├── base/          # Platform abstractions, threading, memory, logging
├── cpu/           # Xbox 360 CPU (PowerPC) emulation
│   ├── backend/   # JIT backends (a64 for ARM64, x64 for Intel)
│   │   ├── a64/   # ARM64 JIT using Oaknut library
│   │   └── x64/   # x64 JIT using Xbyak library
│   ├── compiler/  # HIR (High-level IR) compilation passes
│   ├── hir/       # High-level Intermediate Representation
│   └── ppc/       # PowerPC frontend, instruction decoding
├── gpu/           # Graphics Processing Unit emulation
│   ├── metal/     # Metal graphics backend (macOS-specific)
│   ├── vulkan/    # Vulkan backend (disabled on macOS)
│   ├── d3d12/     # Direct3D 12 backend (Windows)
│   └── null/      # Null graphics backend for testing
├── hid/           # Human Interface Device (input) drivers
├── kernel/        # Xbox 360 kernel emulation
│   ├── xam/       # Xbox Accessory Manager modules
│   ├── xboxkrnl/  # Core Xbox kernel modules
│   └── xbdm/      # Xbox Debug Manager
├── patcher/       # Game patching functionality
├── ui/            # User interface (ImGui-based)
├── vfs/           # Virtual File System
│   └── devices/   # STFS, SVOD, disc image, host path devices
└── tools/         # Various emulator tools

third_party/       # External dependencies
├── oaknut/        # ARM64 JIT assembler library
├── xbyak/         # x86/x64 JIT assembler library
├── metal-cpp/     # C++ bindings for Metal API
├── metal-shader-converter/  # DXIL to Metal shader conversion
├── DirectXShaderCompiler/   # HLSL/DXIL compilation (dxilconv)
├── capstone/      # Disassembler for debugging
├── imgui/         # Immediate mode GUI
├── SDL2/          # Cross-platform windowing/input
└── ...
```

### Core Components

#### CPU Emulation (`src/xenia/cpu/`)
- **PowerPC Frontend**: Decodes Xbox 360 PowerPC instructions
- **HIR Compiler**: Converts PPC to high-level IR with optimization passes
- **JIT Backends**: 
  - `a64/` - ARM64 backend for Apple Silicon using Oaknut
  - `x64/` - x86_64 backend using Xbyak
- **Key Files**: `processor.cc`, `xex_module.cc`, `thread_state.cc`

#### GPU Emulation (`src/xenia/gpu/`)
- **Command Processor**: Parses and executes Xbox 360 GPU command buffers
- **Shader Translation**: Converts Xbox 360 shaders to target API
- **Metal Backend** (`metal/`):
  - `metal_command_processor.cc` - Metal-specific command processing
  - `metal_shader_converter.cc` - DXBC→DXIL→Metal shader pipeline
  - `metal_graphics_system.cc` - Graphics system initialization
- **Key Concepts**: Render target cache, texture cache, primitive processor

#### Kernel Emulation (`src/xenia/kernel/`)
- **xboxkrnl**: Core OS functions (memory, threading, I/O)
- **xam**: Xbox Accessory Manager (user profiles, content, networking)
- **Key Files**: `kernel_state.cc`, `xthread.cc`, `user_module.cc`

#### Virtual File System (`src/xenia/vfs/`)
- **Device Types**:
  - `xcontent_container_device` - STFS/SVOD content packages (XBLA games)
  - `disc_image_device` - ISO/XEX disc images
  - `host_path_device` - Maps host filesystem paths
- Supports LIVE, CON, PIRS container signatures

## Build System

### Prerequisites (macOS)
```bash
xcode-select --install
brew install sdl2 lz4 python@3.12  # Python 3.10+ required
```

**Note**: Python 3.10+ 64-bit is required. If your system Python is older, use Homebrew's Python:
```bash
/usr/local/Cellar/python@3.12/*/bin/python3.12 xenia-build.py build --arch=x86_64
```

### Build Commands
```bash
./xb setup              # Initialize submodules and dependencies
./xb build --arch=arm64 # Build for Apple Silicon
./xb build --arch=x86_64 # Build for Intel Mac
./xb premake            # Regenerate Xcode project
./xb format             # Format code with clang-format
```

### Build Notes
- **First build is slow**: The `dxilconv` component (LLVM-based shader compiler) takes significant time to compile (~10-20 minutes)
- **Submodule issues**: If DirectXShaderCompiler fails with missing SPIRV-Headers:
  ```bash
  git submodule update --init --recursive third_party/DirectXShaderCompiler
  ```

### Build Artifacts
- Debug: `build/bin/Debug/xenia.app`
- Release: `build/bin/Release/xenia.app`

### Key Premake Files
- `premake5.lua` - Root build configuration
- `src/xenia/app/premake5.lua` - Main app target
- `src/xenia/*/premake5.lua` - Per-component targets

## Code Style

- Follow [docs/style_guide.md](docs/style_guide.md)
- Use `xb format` before committing
- Naming: `snake_case` for functions/variables, `PascalCase` for types
- Macros: `XE_` prefix, e.g., `XE_PLATFORM_MAC`, `XE_ARCH_ARM64`
- Use `XELOGI`, `XELOGW`, `XELOGE` for logging

## Common Development Tasks

### Adding a New Kernel Export
1. Add function declaration in `src/xenia/kernel/xboxkrnl/` or `xam/`
2. Register with `SHIM_CALL` macro and `SHIM_SET_MAPPING`
3. Implement using guest memory helpers

### Debugging Game Issues
1. Enable verbose logging: `--log_level=3`
2. Use `--log_file=stdout` for console output
3. Check for unimplemented kernel calls (logged as warnings)
4. GPU issues: Use `--trace_gpu` and trace viewer tools

### Metal Backend Development
- Shader conversion pipeline: DXBC → DXIL → Metal IR
- Uses Apple's Metal Shader Converter library
- Reference: `src/xenia/gpu/metal/`

## Platform-Specific Notes

### macOS Specifics
- Requires macOS 15.0+ for Rosetta AVX support (x86_64 builds)
- Metal backend is the only supported GPU backend
- Discord presence is disabled on macOS
- Debug UI is disabled on ARM64 (TODO: port host context handling)

### Xbox 360 Game Formats
- **XBLA (Xbox Live Arcade)**: STFS containers with title ID folders
  - Path structure: `<container>/0000000000000000/<TitleID>/<ContentID>/`
- **Retail Disc**: ISO images or extracted XEX files
- **Default executable**: `default.xex` (can override with `--launch_module`)

## Useful CVars (Config Variables)
```
--gpu=metal              # Graphics system (metal, null)
--apu=sdl                # Audio system (sdl, nop)
--log_file=stdout        # Log to console
--log_level=3            # Verbose logging
--mount_cache=true       # Enable cache mount
--target=/path/to/game   # Game path
```

## Debugging Tips
- Build with Debug configuration for symbols
- Use `--emit_source_annotations` for JIT disassembly comments
- MMIO and memory access issues often logged at warning level
- GPU command buffer issues visible with trace tools
- For XBLA games, the container path is the full path to the STFS file (e.g., the `30BA92...` file, not the folder)

## Running Games
```bash
# Run an XBLA game (STFS container)
./build/bin/Debug/xenia.app/Contents/MacOS/xenia "/path/to/0000000000000000/TITLEID/CONTENTID/STFSFILE"

# Run with debug logging
./build/bin/Debug/xenia.app/Contents/MacOS/xenia --log_file=stdout --log_level=3 "/path/to/game"

# Run a disc XEX
./build/bin/Debug/xenia.app/Contents/MacOS/xenia "/path/to/default.xex"
```

## Key Entry Points for Investigation
- Game loading: `Emulator::LaunchPath()` in `emulator.cc`
- XEX loading: `XexModule` in `cpu/xex_module.cc`
- STFS container: `XContentContainerDevice` in `vfs/devices/`
- GPU commands: `CommandProcessor::ExecutePacket()` in `gpu/command_processor.cc`
- Kernel calls: `xboxkrnl/` and `xam/` modules
