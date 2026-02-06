# Xenia-Mac-Netplay - Copilot Instructions

## Project Overview

Xenia-Mac-Netplay is a macOS fork of [Xenia Canary](https://github.com/xenia-canary/xenia-canary) with full **netplay/online multiplayer** support, ported from [AdrianCassar's netplay branch](https://github.com/AdrianCassar/xenia-canary) (v5.0.0). It targets **macOS** with support for **Apple Silicon (ARM64)** and **Intel x86_64** architectures, using **Metal** as the graphics backend.

### Key Characteristics
- **Language**: C++20 with some Objective-C++ (`.mm` files) for Metal/macOS integration
- **Build System**: Premake5 generating Xcode projects
- **Target Platforms**: macOS 15.0+ (Sequoia)
- **Graphics API**: Metal (Apple's GPU API)
- **CPU Backend**: ARM64 backend using Oaknut JIT library, or x64 backend using Xbyak
- **Netplay**: Xbox Live online multiplayer via Xenia-WebServices backend, Systemlink LAN
- **Repo**: https://github.com/jakeypiez/xenia-mac-netplay
- **Bundle ID**: `com.jakeypiez.xenia-mac-netplay`

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
│   ├── xam/       # Xbox Accessory Manager modules (profiles, networking, UI)
│   │   ├── apps/  # XGI and XLiveBase app handlers (session RPCs)
│   │   ├── ui/    # ImGui dialogs (FriendsUI, SigninUI, GamercardUI, etc.)
│   │   └── unmarshaller/ # 20 unmarshaller files for network data
│   ├── xboxkrnl/  # Core Xbox kernel modules
│   ├── xbdm/      # Xbox Debug Manager
│   ├── json/      # 26 JSON serialization files (RapidJSON-based)
│   ├── util/      # Network utilities, game info database, XLast, presence
│   ├── XLiveAPI.cpp/.h   # HTTP REST client for netplay backend
│   ├── xsession.cc/.h    # Xbox session management
│   ├── xsocket.cc/.h     # Xbox socket handling (macOS POSIX)
│   ├── xnet.h             # Network definitions (macOS platform stubs)
│   └── upnp.cc/.h        # UPnP port forwarding via miniupnpc
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
├── libcurl/       # HTTP client (OpenSSL 3 TLS on macOS)
├── miniupnp/      # UPnP port forwarding library
├── rapidjson/     # JSON parsing for netplay API
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

#### Netplay / Networking (`src/xenia/kernel/`)
- **XLiveAPI** (`XLiveAPI.cpp/.h`): HTTP REST client for Xenia-WebServices backend
  - Manages player sessions, matchmaking, presence, XStorage
  - Uses libcurl with OpenSSL 3 for HTTPS
  - Network interface discovery via `getifaddrs()` on macOS
  - `Init()` → discovers interfaces → selects interface → initializes UPnP → connects to backend
- **UPnP** (`upnp.cc/.h`): Port forwarding via miniupnpc
  - `Initialize(multicast_if)` — binds to specific interface, 3-attempt retry loop
  - `SearchUPnP()` — discovers IGD and maps ports (3074 UDP/TCP)
  - `Deactivate()` — removes port mappings and resets state
  - macOS requires `NSLocalNetworkUsageDescription` + `NSBonjourServices` in Info.plist
- **XSession** (`xsession.cc/.h`): Xbox session create/browse/join
- **XSocket** (`xsocket.cc/.h`): Xbox socket abstraction (POSIX on macOS)
- **XAM Apps** (`xam/apps/xgi_app.cc`, `xlivebase_app.cc`): Session RPC handlers
- **JSON** (`json/`): 26 RapidJSON serialization files for API payloads
- **Unmarshaller** (`xam/unmarshaller/`): 20 files for network data marshalling
- **Backend**: https://xenia-netplay-2a0298c0e3f4.herokuapp.com/

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
brew install sdl2 lz4 openssl@3  # Build dependencies (bundled in .app for end users)
```

Python 3.10+ 64-bit is required for the build script (`xenia-build.py`).

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
- Debug: `build/bin/Mac-ARM64/Debug/Xenia-Mac-Netplay.app`
- Release: `build/bin/Mac-ARM64/Release/Xenia-Mac-Netplay.app`
- Checked: `build/bin/Mac-ARM64/Checked/Xenia-Mac-Netplay.app`

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
- UPnP multicast requires Info.plist `NSLocalNetworkUsageDescription` + `NSBonjourServices` (`_ssdp._udp.`)
- XAM dialog creation deferred to UI thread via `CallInUIThread()` factory pattern (prevents ImGui threading crashes)
- Release builds default to `log_level=1` (Warning) and `log_to_stdout=false`
- OpenSSL 3 dylibs bundled in `.app/Contents/Frameworks/` via postbuild script

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
--log_level=3            # Verbose logging (Release defaults to 1=Warning)
--mount_cache=true       # Enable cache mount
--target=/path/to/game   # Game path

# Netplay CVars
--network_mode=2         # 0=Offline, 1=Systemlink, 2=Xbox Live (default)
--api_address=host:port/ # Netplay server address
--upnp=true              # Enable UPnP port forwarding
--xstorage_backend=true  # Use backend for XStorage
--logging=false          # Log network activity & stats
--log_mask_ips=true      # Don't include P2P IPs in logs
--network_guid=en0       # Network interface name
```

Config file: `~/.local/share/Xenia/xenia-canary.config.toml`
Settings changed via Network menu UI are persisted via `OverrideConfigVar<T>()` + `SaveConfig()`.

## Debugging Tips
- Build with Debug configuration for symbols
- Use `--emit_source_annotations` for JIT disassembly comments
- MMIO and memory access issues often logged at warning level
- GPU command buffer issues visible with trace tools
- For XBLA games, the container path is the full path to the STFS file (e.g., the `30BA92...` file, not the folder)

## Running Games
```bash
# Run an XBLA game (STFS container)
./build/bin/Mac-ARM64/Debug/Xenia-Mac-Netplay.app/Contents/MacOS/xenia "/path/to/0000000000000000/TITLEID/CONTENTID/STFSFILE"

# Run with debug logging
./build/bin/Mac-ARM64/Debug/Xenia-Mac-Netplay.app/Contents/MacOS/xenia --log_file=stdout --log_level=3 "/path/to/game"

# Run a disc XEX
./build/bin/Mac-ARM64/Debug/Xenia-Mac-Netplay.app/Contents/MacOS/xenia "/path/to/default.xex"

# Run with netplay (Xbox Live mode, default)
./build/bin/Mac-ARM64/Release/Xenia-Mac-Netplay.app/Contents/MacOS/xenia --network_mode=2 "/path/to/game"

# Run in Systemlink (LAN) mode
./build/bin/Mac-ARM64/Release/Xenia-Mac-Netplay.app/Contents/MacOS/xenia --network_mode=1 "/path/to/game"
```

## Key Entry Points for Investigation
- Game loading: `Emulator::LaunchPath()` in `emulator.cc`
- XEX loading: `XexModule` in `cpu/xex_module.cc`
- STFS container: `XContentContainerDevice` in `vfs/devices/`
- GPU commands: `CommandProcessor::ExecutePacket()` in `gpu/command_processor.cc`
- Kernel calls: `xboxkrnl/` and `xam/` modules
- Netplay init: `XLiveAPI::Init()` in `kernel/XLiveAPI.cpp`
- UPnP discovery: `UPnP::Initialize()` in `kernel/upnp.cc`
- Session management: `XSession` in `kernel/xsession.cc`
- Network UI: `NetplayConfigDialog` in `app/emulator_window.cc`
- Friends UI: `FriendsUI` dialog in `kernel/xam/ui/friends_ui.cc`
- Dialog dispatch: `xeXamDispatchDialogAsync<T>` in `kernel/xam/xam_ui.cc`
- Config persistence: `OverrideConfigVar<T>()` helper in `app/emulator_window.cc`

## Netplay Architecture

### Network Flow
1. `XLiveAPI::Init()` is called when network mode is Xbox Live (2)
2. `DiscoverNetworkInterfaces()` enumerates via `getifaddrs()` on macOS
3. `SelectNetworkInterface()` picks the active interface
4. `UPnP::Initialize(LocalIP_str())` discovers IGD router, maps ports 3074 UDP/TCP
5. HTTP POST to backend registers player, gets online IP
6. Game calls XSession APIs → routed through `xgi_app.cc` / `xlivebase_app.cc`

### Config Persistence Pattern
CVars are defined with `DEFINE_bool/int32/string` in their respective `.cc` files.
To override from a different translation unit (e.g., `emulator_window.cc`):
```cpp
template <typename T>
void OverrideConfigVar(const std::string& name, T value) {
  // Uses cvar::ConfigVars global map to find and update config_value_
}
// Then call config::SaveConfig() to write to xenia-canary.config.toml
```

### Dialog Threading Pattern
XAM dialogs (SigninUI, FriendsUI, MessageBoxDialog, etc.) must be created on the UI thread:
```cpp
xeXamDispatchDialogAsync<T>(kernel_state, thread, factory_fn, args...)
// factory_fn is a std::function<T*()> invoked via CallInUIThread()
// Prevents ImGui threading crashes (SIGABRT in ImGui::Begin)
```

### Friends UI Architecture
The friends list UI is triggered when games call `XamShowFriendsUI` (ordinal 0x2BF):
- `xam_ui.cc` dispatches `FriendsUI` dialog via `xeXamDispatchDialogAsync<ui::FriendsUI>`
- `FriendsUI` constructor kicks off `std::async` fetch of `XLiveAPI::GetAllFriendsPresence()`
- `OnDraw()` polls the future; when ready, stores results in `friends_presence_result_`
- `DrawFriendsContent()` renders the main popup with search, filters, and friend entries
- `DrawFriendContent()` renders each friend: gamertag, XUID, title info, Join/Remove buttons
- `DrawAddFriend()` renders the "Add Friend" modal with XUID input and validation
- Shared argument structs in `netplay_manager_util.h`: `FriendsContentArgs`, `AddFriendArgs`

### UPnP macOS Specifics
- `upnpDiscover()` requires `multicast_if` parameter (local IP) on macOS
- Info.plist must have `NSLocalNetworkUsageDescription` and `NSBonjourServices`
- First discovery fails while macOS shows Local Network permission dialog → 3-attempt retry loop with 3s delays
- `Deactivate()` removes port mappings; re-check via `RefreshPorts()`
