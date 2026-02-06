<p align="center">
    <a href="https://github.com/jakeypiez/xenia-mac-netplay">
        <img height="256px" src="https://raw.githubusercontent.com/xenia-canary/xenia/master/assets/icon/256.png" />
    </a>
</p>

<h1 align="center">Xenia — Xbox 360 Emulator for macOS with Netplay</h1>

A macOS fork of [Xenia Canary](https://github.com/xenia-canary/xenia-canary) with full **netplay/online multiplayer** support, ported from [AdrianCassar's netplay branch](https://github.com/AdrianCassar/xenia-canary) (v5.0.0).

## Features

- **macOS native** — Metal GPU backend, Apple Silicon (ARM64) and Intel (x86_64)
- **Xbox Live netplay** — Online multiplayer via [Xenia-WebServices](https://github.com/AdrianCassar/Xenia-WebServices) backend
- **Systemlink** — LAN multiplayer without a backend server
- **UPnP port forwarding** — Automatic port mapping with macOS Local Network permission handling
- **Network config UI** — In-app Network menu for server selection, connection status, and settings

## Requirements

- **macOS 15.0+** (Sequoia) — required for Rosetta AVX support on Intel builds
- **Apple Silicon** (M1/M2/M3/M4/M5) or Intel Mac
- **Homebrew**: `brew install sdl2 lz4 openssl@3`

## Download

See [Releases](https://github.com/jakeypiez/xenia-mac-netplay/releases) for pre-built `.dmg` downloads.

## Building from Source

```bash
# Clone
git clone --recursive https://github.com/jakeypiez/xenia-mac-netplay.git
cd xenia-mac-netplay

# Setup (submodules + premake)
./xb setup

# Build
./xb build --arch=arm64          # Apple Silicon
./xb build --arch=x86_64         # Intel Mac
./xb build --config=Release      # Release build (default is Debug)
```

See [docs/building.md](docs/building.md) for detailed build instructions.

## Running

```bash
# Run a game (Xbox Live mode is default)
./build/bin/Mac-ARM64/Release/Xenia-Mac-Netplay.app/Contents/MacOS/xenia "/path/to/game"

# Run with specific network mode
./build/bin/Mac-ARM64/Release/Xenia-Mac-Netplay.app/Contents/MacOS/xenia \
  --network_mode=1 "/path/to/game"   # Systemlink (LAN)
```

Or open the `.app` bundle directly and configure via **Network → Netplay Configuration**.

### Network Modes

| Mode | Flag | Description |
|------|------|-------------|
| Offline | `--network_mode=0` | No networking |
| Systemlink | `--network_mode=1` | LAN only, no backend server |
| Xbox Live | `--network_mode=2` | Online via Xenia-WebServices backend (default) |

## Credits

- [Ben Vanik](https://github.com/benvanik) — Original Xenia emulator
- [Xenia Canary](https://github.com/xenia-canary/xenia-canary) — Canary/experimental branch
- [wmarti](https://github.com/wmarti/xenia-mac) — macOS/Metal port
- [AdrianCassar](https://github.com/AdrianCassar/xenia-canary) — Netplay implementation (v5.0.0)

## License

BSD license — see [LICENSE](LICENSE) for details.

## Disclaimer

The goal of this project is to experiment, research, and educate on the topic
of emulation of modern devices and operating systems. **It is not for enabling
illegal activity**. All information is obtained via reverse engineering of
legally purchased devices and games and information made public on the internet.
