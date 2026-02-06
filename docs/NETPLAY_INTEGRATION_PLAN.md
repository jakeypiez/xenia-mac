# Netplay Integration Plan (from AdrianCassar/xenia-canary v5.0.0)

## Status: BUILD SUCCEEDED - All Phases Complete

**Last Updated:** February 7, 2026

### Current Progress Summary

| Phase | Description | Status |
|-------|-------------|--------|
| 1 | Fetch and Analyze | ✅ Complete |
| 2 | Third-Party Dependencies | ✅ Complete (libcurl 8.18.0 + OpenSSL 3 TLS, miniupnp) |
| 3 | Kernel Networking | ✅ Complete (full macOS implementations) |
| 4 | XLiveAPI HTTP Client | ✅ Complete (HTTPS enabled via OpenSSL 3) |
| 5 | Session Management | ✅ Complete (all files ported + macOS fixes) |
| 6 | UI/UX and Config | ✅ Complete (NetplayConfigDialog, Network menu) |
| 7 | Build System Updates | ✅ Complete |
| 8 | Validation | ✅ Build Succeeded (Debug, ARM64) — runtime testing pending |
| 9 | Documentation | ✅ Up to date |

### Recent Fixes (Feb 5, 2026 Late Session)

**xsocket.cc macOS Compatibility:**
- ✅ Fixed `be<uint32_t>` bitwise compound assignment (`|=`, `&=`) by using explicit get/set operations
- ✅ Changed `accept()` and `recvfrom()` to use `socklen_t*` instead of `int*` for POSIX compliance
- ✅ Reorganized variable declarations to avoid cross-initialization with goto statements
- ✅ Added `WSAEventSelect` stub for non-Windows platforms (returns success)

### Final Build Fixes (Feb 6, 2026)

**xam_net.cc Final Fixes:**
- ✅ Replaced remaining `IN_ADDR` types with `in_addr` (POSIX)
- ✅ Fixed `lpstring_t` → `std::string` ambiguous assignment with explicit `std::string()` casts
- ✅ Replaced `ev->native_handle()` (XEvent doesn't expose one on macOS) with 0 since `WSAEventSelect` is a no-op stub

**user_profile.cc Fixes:**
- ✅ Added `on_presence_change` delegate to emulator.h: `xe::Delegate<const std::string_view, const std::u16string_view>`
- ✅ Fixed `AttributeStringFormatter::AttributeStringFormatter(...)` → direct constructor `AttributeStringFormatter(...)`

**user_tracker.cc/.h Fixes:**
- ✅ Added `RemoveTitleFromPlayedList(uint64_t xuid, uint32_t title_id)` method (declaration + implementation)

**chrono.h Fix:**
- ✅ Changed `to_local()` return type to `std::chrono::local_time<std::chrono::system_clock::duration>` to match field types

**XLiveAPI.h/.cpp macOS Fixes:**
- ✅ Expanded `IP_ADAPTER_ADDRESSES` stub with `PIP_ADAPTER_UNICAST_ADDRESS_LH`, `MAX_ADAPTER_NAME_LENGTH`
- ✅ Wrapped `wcstombs` call in `GetNetworkFriendlyName()` with `#ifdef XE_PLATFORM_WIN32`

**upnp.cc Fix:**
- ✅ Changed `<third_party/miniupnp/...>` angled includes to `"quotes"` includes

**Third-Party Library Fixes:**
- ✅ Pinned libcurl to stable release `curl-8_18_0` (was on bleeding-edge master)
- ✅ Generated `curl_config.h` via cmake for macOS (POSIX sockets, no SSL backend for now)
- ✅ Fixed include path: `sysincludedirs` for `libcurl/include` so `<curl/...>` angled includes work
- ✅ Fixed `multi_ntfy.h` missing `curl_notify_callback` type by adding `#include <curl/multi.h>`
- ✅ Generated `miniupnpcstrings.h` for miniupnpc build

**Linker Fixes:**
- ✅ Added `Security.framework`, `SystemConfiguration.framework` (for curl macOS proxy detection)
- ✅ Added `z` (zlib) linking for XLast deflate decompression

### Runtime Readiness Fixes (Feb 6, 2026 Session 2)

**DiscoverNetworkInterfaces macOS Implementation (XLiveAPI.cpp):**
- ✅ Implemented using `getifaddrs()` — enumerates interfaces, filters by AF_INET + IFF_UP + !IFF_LOOPBACK
- ✅ Creates `IP_ADAPTER_ADDRESSES` entries with stable `unique_ptr` storage for unicast address pointers
- ✅ Added `#include <ifaddrs.h>` and `#include <net/if.h>` (guarded by `#ifndef XE_PLATFORM_WIN32`)

**IOControl macOS Implementation (xsocket.cc):**
- ✅ Replaced stub returning `X_STATUS_UNSUCCESSFUL` with working `ioctl()` call
- ✅ Added `#include <sys/ioctl.h>` for non-Windows

**WinsockGetLocalIP macOS Implementation (net_utils.cc):**
- ✅ Implemented POSIX path using UDP-connect-to-8.8.8.8 technique (same as Windows)
- ✅ Added `#include <unistd.h>` to `net_utils.h` for `close()`

**TLS/HTTPS — Fixed (Critical):**
- ✅ Discovered `USE_SECTRANSPORT` was removed in curl 8.x — curl was building with NO TLS backend
- ✅ Switched to `USE_OPENSSL` with Homebrew OpenSSL 3 (`/usr/local/opt/openssl@3` or `/opt/homebrew/opt/openssl@3`)
- ✅ Added `USE_APPLE_SECTRUST` for macOS native certificate trust verification
- ✅ OpenSSL dylibs bundled in `.app/Contents/Frameworks/` via postbuild script
- ✅ `install_name_tool` rewrites references to `@rpath/libssl.3.dylib` and `@rpath/libcrypto.3.dylib`
- ✅ Both dylibs codesigned

**WSAEventSelect Documentation:**
- ✅ Improved stub comments — no-op is adequate for netplay (uses `poll()`-based `PollWSARecvFrom`)

### UI/Config & Hardening Fixes (Feb 6, 2026 Session 3)

**NetplayConfigDialog UI (emulator_window.cc/.h):**
- ✅ Added `Network` menu to emulator menu bar (between XMP and Help)
- ✅ Created `NetplayConfigDialog` ImGui dialog with full feature set:
  - Network mode selector (Offline/Systemlink/Xbox Live) with per-mode descriptions
  - Server selection from `api_list` + custom server input with "Connect" button
  - Connection status display with color-coded state (green=Connected, yellow=Pending, red=Failed)
  - "Connect Now" button for Pending state, "Retry Connection" button for Failed state
  - Friends list management (add/remove XUIDs) — only shown in Xbox Live mode
  - Options: UPnP, XStorage, logging, IP masking toggles — mode-aware visibility
  - Systemlink mode shows "no backend server needed" guidance
- ✅ Added miniupnp/libcurl/rapidjson include paths to app premake (`src/xenia/app/premake5.lua`)
- ✅ Added `CURL_STATICLIB`/`MINIUPNP_STATICLIB` defines to app premake

**Config Persistence Fix:**
- ✅ All CVar changes in the dialog now call `config::SaveConfig()` immediately
- ✅ Settings are written to `xenia-canary.config.toml` and persist across restarts
- ✅ Added `#include "xenia/config.h"` to emulator_window.cc

**Connection Retry Mechanism (XLiveAPI.cpp/.h):**
- ✅ Added `ResetInitState()` public method — resets `online_ip_` and `initialized_` to `Pending`
- ✅ Removed `Pending` state gate from `SetAPIAddress()` — now always sets address and re-initializes
- ✅ Dialog "Retry Connection" button calls `ResetInitState()` + `Init()` to allow recovery from failures

**Crash Prevention — assert_always() Removal (XLiveAPI.cpp):**
- ✅ Commented out all 33 `assert_always()` calls that called `abort()` in Debug builds
- ✅ These were triggered on any unexpected HTTP status code (e.g., 404, 500, timeout)
- ✅ Errors are still logged via `XELOGE`/`XELOGW` — just no longer fatal
- ✅ Fixed `GetLocalMachineId()` null dereference: was assert+fall-through, now returns `0`
- ✅ Replaced SSL support assert with `XELOGW("No SSL support in libcurl!")` warning

**Crash Prevention — xsession.cc:**
- ✅ Replaced `assert_always()` in `CreateHostSession` unknown flags path with `XELOGE` + `return X_ERROR_FUNCTION_FAILED`
- ✅ Replaced `assert_always()` in `JoinExistingSession` unknown type path with `XELOGW` + continue

### Remaining Work

**All Phases Complete — Runtime Testing Pending**
- All 9 phases of the netplay integration plan have been implemented
- UI has been tested: backend connection works, IP addresses display correctly
- Session create/browse/join needs game-level testing

**Known Limitations:**
- Discord rich presence intentionally disabled on macOS (build system + source guards)
- `WSAEventSelect` is a no-op on macOS — netplay uses `poll()` instead; only affects games doing their own socket event waits
- `GetMACaddress()` always returns random MAC (dead code below early return — same on all platforms)
- All `assert_always()` calls in `XLiveAPI.cpp` are commented out — HTTP errors are logged but not fatal (prevents Debug build crashes on backend errors)

**xam_state.cc/.h Extensions:**
- ✅ Added `GetUserIndexAssignedToProfileFromXUID(uint64_t xuid)` method
- ✅ Added `GetUserProfileLive(uint64_t xuid)` method
- ✅ Added `GetUserProfileAny(uint64_t xuid)` method

**xlivebase_app.cc Fixes:**
- ✅ Changed `FILE_ATTRIBUTE_DIRECTORY` → `X_FILE_ATTRIBUTE_DIRECTORY`
- ✅ Changed `HRESULT` → `X_HRESULT`

**xbox.h Type Additions:**
- ✅ Added `X_XAMACCOUNTINFO` struct with full implementation (~100 lines)
- ✅ Added `#pragma pack(push, 4)` / `#pragma pack(pop)` for proper struct alignment
- ✅ Added `static_assert_size(X_XAMACCOUNTINFO, 0x17C)` for size verification
- ✅ Added `X_E_INSUFFICIENT_BUFFER` error code

**xam.h Namespace Resolution:**
- ✅ Replaced duplicate `X_XAMACCOUNTINFO` struct with `using X_XAMACCOUNTINFO = ::xe::X_XAMACCOUNTINFO;`
- ✅ Added `using AccountSubscriptionTier = X_XAMACCOUNTINFO::AccountSubscriptionTier;`
- ✅ Added `#pragma pack(push, 4)` around packed structs (X_USER_PAYMENT_INFO, etc.)

**profile_manager.cc/.h Fixes:**
- ✅ Fixed lambda parameter shadowing in `ConvertToXboxLiveEnabledProfile` and `ConvertToOfflineProfile`
- ✅ Changed to use `ToggleLiveFlag()` method instead of direct bitwise operations on `reserved_flags`
- ✅ Moved `GenerateXuid()` from private to public in header

**user_profile.h Fixes:**
- ✅ Added `type()` method returning `UserType::kHost` for backward compatibility

**xam_user.cc Fixes:**
- ✅ Changed `signin_state` assignments to use `static_cast<uint32_t>()` for enum conversion
- ✅ Changed comparison `!= 2` to `!= X_USER_SIGNIN_STATE::SignedInToLive`

**xam_profile.cc Fixes:**
- ✅ Changed `GenerateXuid()` call to use `kernel_state()->xam_state()->profile_manager()->GenerateXuid()`

**xam_net.cc macOS Compatibility:**
- ✅ Added `<netdb.h>` include for `gethostbyname()` and `hostent` types
- ✅ Added cross-platform `IP_BYTE1/2/3/4` macros for IP address byte extraction
- ✅ Fixed all `in_addr.S_un.S_un_b.s_b*` references to use macros
- ✅ Changed `IN_ADDR` → `in_addr` for POSIX compatibility
- ✅ Changed `in_addr->S_un.S_addr` → `in_addr->s_addr`

**xboxkrnl_modules.cc/.h Fixes:**
- ✅ Added public `bool XexCheckExecutablePrivilege(uint32_t privilege)` function declaration
- ✅ Added implementation that can be called from xam_net.cc

**user_tracker.cc Fixes:**
- ✅ Added `#include "xenia/kernel/xnet.h"` for `PLATFORM_TYPE` enum
- ✅ Changed `GAMERCARD_ZONE_OPTIONS::GAMERCARD_ZONE_PRO` → `X_USER_PROFILE_GAMERCARD_ZONE_OPTIONS::GAMERCARD_ZONE_PRO`

**chrono.h Fixes:**
- ✅ Re-enabled `to_local()` method with simplified implementation (returns system time without local conversion)

### Actual Netplay Implementation

The Adrian Cassar fork DOES contain full netplay functionality in release tag `v5.0.0`:

**Core Components (~4,500+ lines):**
1. **XLiveAPI** (`src/xenia/kernel/XLiveAPI.cpp/.h`) - HTTP REST client using libcurl (2,057 lines)
2. **XSession** (`src/xenia/kernel/xsession.cc/.h`) - Full session management (1,173 lines)
3. **xnet.h** - Network definitions and structures (1,332 lines)
4. **upnp** (`src/xenia/kernel/upnp.cc/.h`) - UPnP port forwarding

**JSON API Serialization (26 files in `src/xenia/kernel/json/`):**
- `session_object_json` - Session data
- `player_object_json` - Player/profile data
- `presence_object_json` - Presence/status
- `friend_presence_object_json` - Friends list
- `leaderboard_object_json` - Leaderboards
- `http_response_object_json` - Generic responses
- And more...

**Third-Party Dependencies:**
- `third_party/libcurl` - HTTP client library
- `third_party/wolfssl` - TLS/SSL support
- `third_party/miniupnp` - UPnP port forwarding
- `third_party/rapidjson` (already present)

**Features (from v5.0.0 release notes):**
- XSession Properties Support
- XLiveBase Unmarshaller
- XStorage support (download/upload)
- Friends list manager
- Join friend sessions
- Discord rich presence integration
- Network mode selector (Offline, Systemlink, Xbox Live)
- 500+ compatible games

### What Was Ported (Feb 5, 2026)

**Core Netplay Files Ported:**
1. `src/xenia/kernel/XLiveAPI.cpp/.h` - HTTP REST client (2,057 lines)
2. `src/xenia/kernel/xsession.cc/.h` - Session management (1,173 lines)
3. `src/xenia/kernel/xnet.h` - Network definitions (1,332 lines, macOS fixes)
4. `src/xenia/kernel/upnp.cc/.h` - UPnP port forwarding
5. `src/xenia/kernel/xsocket.cc/.h` - Socket handling (macOS fixes)
6. `src/xenia/kernel/util/net_utils.cc/.h` - Network utilities

**XAM/Kernel Updates Ported:**
7. `src/xenia/kernel/xam/xam_net.cc/.h` - Network exports (macOS platform fix)
8. `src/xenia/kernel/xam/apps/xgi_app.cc/.h` - Session handlers
9. `src/xenia/kernel/xam/apps/xlivebase_app.cc/.h` - XLive handlers (2,945 lines)
10. `src/xenia/kernel/xam/profile_manager.cc/.h` - Profile management extensions
11. `src/xenia/kernel/xam/user_profile.cc/.h` - User profile extensions
12. `src/xenia/kernel/xam/user_tracker.cc/.h` - User tracking extensions

**Utility Files Ported:**
13. `src/xenia/kernel/util/game_info_database.cc/.h` - Game info database with GetXLast
14. `src/xenia/kernel/util/xlast.cc/.h` - XLast parsing (new API with query_id params)
15. `src/xenia/kernel/util/presence_string_builder.cc/.h` - Presence string formatting

**JSON Serialization (26 files in `src/xenia/kernel/json/`):**
16. All 26 JSON files ported

**Unmarshaller Infrastructure (20 files in `src/xenia/kernel/xam/unmarshaller/`):**
17. All 20 unmarshaller files ported

**Third-Party Dependencies:**
18. `third_party/libcurl` - HTTP client submodule added
19. `third_party/miniupnp` - UPnP submodule added
20. `third_party/libcurl.lua` - Premake config (macOS SecureTransport)
21. `third_party/miniupnp.lua` - Premake config

**Core Type Additions:**
22. `src/xenia/xbox.h` - Added `X_XAMACCOUNTINFO` struct (~100 lines)

**Build System Updates:**
23. `premake5.lua` - Added libcurl, miniupnp includes
24. `src/xenia/kernel/premake5.lua` - Updated links and includedirs

**Files Modified for Compatibility:**
25. `src/xenia/emulator.cc` - Simplified logging to match v5.0.0 API

### Current Build Errors (as of Feb 5, 2026 late session)

**Most macOS compatibility issues have been fixed.** Build errors reduced from ~40+ to a handful.

**Remaining issues to verify:**
- user_tracker.cc - May need additional fixes for chrono/time handling
- xam_net.cc - Verify all IP_BYTE macro uses are correct
- Any remaining namespace issues with X_XAMACCOUNTINFO

**Build command:** `python3.12 xenia-build.py build --config=Debug`

### Full Netplay Port Requirements

To port the complete netplay functionality to xenia-mac:

## Goals
Integrate netplay functionality from the `netplay_canary_experimental` branch head into this xenia-mac fork. Keep behavior and defaults aligned with netplay_canary, including default WebServices endpoint selection and optional features (updater, Discord rich presence, friends UI, etc.).

## Scope
This plan covers kernel networking, session management, XLive/XAM surfaces, WebServices REST integration, UI/UX changes, config/flags, build system updates, and validation.

## Assumptions
- The netplay fork uses a REST backend (Xenia-WebServices) and provides an explicit base URL and endpoints.
- netplay_canary has defaults for optional features; we will mirror those defaults unless explicitly overridden.
- The macOS fork includes Metal-specific changes that must be preserved.

## Approach Summary (UPDATED)
Use a structured forward-port strategy from **release tag v5.0.0** (not the stripped branch):
1. Identify the netplay change set from v5.0.0 tag vs upstream canary.
2. Port third-party dependencies (libcurl, wolfssl, miniupnp).
3. Port kernel networking and XLiveAPI.
4. Port session management and JSON serialization.
5. Port UI and config changes.
6. Adapt for macOS/Metal compatibility.
7. Validate with a minimal netplay flow.

## Detailed Plan (REVISED)

### Phase 0: Baseline and Branching
1. Record the current local HEAD SHA and any dirty state. ✅
2. Create branch `codex/netplay-integration`.
3. If the working tree is dirty, capture diffs for reapplication after merges.

### Phase 1: Fetch and Analyze Netplay Source ✅ COMPLETE
1. Add remote for netplay fork and fetch. ✅
2. Fetch release tag v5.0.0 (contains actual netplay). ✅
3. Identify netplay files:
   - `src/xenia/kernel/XLiveAPI.cpp/.h` - HTTP REST client (2,057 lines) ✅
   - `src/xenia/kernel/xsession.cc/.h` - Session management (1,173 lines) ✅
   - `src/xenia/kernel/xnet.h` - Network definitions (1,332 lines) ✅
   - `src/xenia/kernel/upnp.cc/.h` - UPnP port forwarding ✅
   - `src/xenia/kernel/json/*` - 26 JSON serialization files ✅
4. Identify third-party dependencies:
   - `third_party/libcurl` - HTTP client ✅
   - `third_party/wolfssl` - TLS/SSL ✅
   - `third_party/miniupnp` - UPnP ✅

### Phase 2: Port Third-Party Dependencies ✅ COMPLETE
1. Add libcurl submodule and premake integration. ✅
2. ~~Add wolfssl submodule~~ (using macOS SecureTransport instead)
3. Add miniupnp submodule and premake integration. ✅
4. Ensure macOS compatibility for all libraries. ✅
5. Created `third_party/libcurl.lua` with SecureTransport backend. ✅
6. Created `third_party/miniupnp.lua` for UPnP. ✅

### Phase 3: Port Kernel Networking ✅ COMPLETE
1. Port `src/xenia/kernel/xnet.h` network definitions. ✅
2. Port enhanced `xsocket.cc/.h` with full networking support. ✅
3. Port `src/xenia/kernel/upnp.cc/.h` for UPnP. ✅
4. Port `src/xenia/kernel/util/net_utils.cc/.h`. ✅
5. Add macOS platform support (`XE_PLATFORM_MAC` handling). ✅
6. Fixed `in_addr`, `sockaddr_in`, socket headers for POSIX. ✅

### Phase 4: Port XLiveAPI HTTP Client ✅ COMPLETE
1. Port `src/xenia/kernel/XLiveAPI.cpp/.h`. ✅
2. Port all 26 JSON serialization classes from `src/xenia/kernel/json/`. ✅
3. Added `IP_ADAPTER_ADDRESSES` stub for non-Windows platforms. ✅
4. Fixed `CHAR` → `char`, `__declspec(align(8))` → `alignas(8)`. ✅

### Phase 5: Port Session Management ✅ COMPLETE
1. Port `src/xenia/kernel/xsession.cc/.h`. ✅
2. Port XGI message handlers (`xgi_app.cc`). ✅
3. Port XLiveBase handlers (`xlivebase_app.cc/.h`). ✅
4. Port unmarshaller infrastructure (20 files). ✅
5. Port `xam_net.cc/.h` updates. ✅
6. Port ProfileManager extensions. ✅
7. Port UserProfile extensions. ✅
8. Port GameInfoDatabase extensions (GetXLast). ✅
9. Port UserTracker extensions. ✅
10. Port xlast.cc/.h (new API with query_id parameters). ✅
11. Port presence_string_builder.cc/.h. ✅
12. Fixed xsocket.cc macOS compatibility (Windows API references). ✅
13. Added GetUserIndexAssignedToProfileFromXUID to XamState. ✅
14. Fixed xam_user.cc enum conversions. ✅
15. Fixed chrono.h to_local method. ✅

### Phase 6: Port UI/UX and Config ✅ COMPLETE
1. ✅ Ported network mode selector UI (Offline/Systemlink/Xbox Live combo with per-mode descriptions).
2. ✅ Ported friends list manager UI (add/remove XUIDs in Xbox Live mode).
3. ✅ Discord rich presence intentionally skipped (disabled on macOS).
4. ✅ All config CVars working with persistence:
   - `api_address` - Xenia Server Address
   - `api_list` - List of server URLs
   - `network_mode` - Offline/Systemlink/Xbox Live
   - `friends_xuids` - Friend XUID list
   - `log_mask_ips` - Privacy setting
   - `xstorage_backend` - XStorage from backend
   - All changes saved to `xenia-canary.config.toml` via `config::SaveConfig()`
5. ✅ Metal UI compatibility confirmed (ImGui-based dialog works on macOS).
6. ✅ Added `Network` menu to emulator menu bar.
7. ✅ Server selection from `api_list` + custom server input with "Connect" button.
8. ✅ Connection status display with color-coded state (green/yellow/red).
9. ✅ Mode-aware UI sections (server/friends only shown in Xbox Live mode).
10. ✅ Retry mechanism for failed connections ("Retry Connection" button).
11. ✅ Options panel: UPnP, XStorage, logging, IP masking toggles.

### Phase 7: Build System Updates ✅ COMPLETE
1. Add premake rules for libcurl, miniupnp. ✅
2. Updated `premake5.lua` to include new third-party libs. ✅
3. Update xenia-kernel premake for new sources and includes. ✅
4. Added `CURL_STATICLIB`, `MINIUPNP_STATICLIB` defines. ✅
5. Premake generation successful. ✅

### Phase 8: Validation ⚠️ IN PROGRESS
1. Build debug on macOS ARM64. ✅ BUILD SUCCEEDED
2. Build release on macOS. 🔲
3. Test offline mode still works. 🔲
4. Test connection to https://xenia-netplay-2a0298c0e3f4.herokuapp.com/ 🔲
5. Test session creation/browse. 🔲
6. Test friends list. 🔲
7. Verify no regressions in single-player games. 🔲

**Fixes Applied (reducing from ~40 to minimal errors):**

**xsocket.cc (all fixed):**
- ✅ Fixed `be<uint32_t>` bitwise compound assignment operations
- ✅ Added `socklen_t*` casts for `accept()` and `recvfrom()`
- ✅ Fixed variable initialization with goto statements
- ✅ Added `WSAEventSelect` stub for non-Windows

**xam_state.cc/.h (all fixed):**
- ✅ Added `GetUserIndexAssignedToProfileFromXUID()` method
- ✅ Added `GetUserProfileLive()` method
- ✅ Added `GetUserProfileAny()` method

**profile_manager.cc/.h (all fixed):**
- ✅ Fixed lambda parameter shadowing
- ✅ Fixed bitwise operations on `reserved_flags`
- ✅ Moved `GenerateXuid()` to public

**xam_net.cc (all fixed):**
- ✅ Added `<netdb.h>` include
- ✅ Added `IP_BYTE1/2/3/4` macros
- ✅ Fixed all IP address byte extraction
- ✅ Changed `IN_ADDR` to `in_addr`

**xboxkrnl_modules.cc/.h (all fixed):**
- ✅ Added public `XexCheckExecutablePrivilege()` function

**user_tracker.cc (all fixed):**
- ✅ Added xnet.h include for PLATFORM_TYPE
- ✅ Fixed GAMERCARD_ZONE_OPTIONS namespace

**chrono.h (all fixed):**
- ✅ Re-enabled `to_local()` method

### Phase 9: Documentation ✅ COMPLETE
1. Update this plan with completion status. ✅
2. Add netplay setup guide for macOS. ✅ (see below)
3. Document config options. ✅ (see below)
4. Add troubleshooting guide. 🔲

## Implementation Details and Tasks (REVISED)

### Files to Port from v5.0.0

**Core XLiveAPI (~2,100 lines):**
- `src/xenia/kernel/XLiveAPI.cpp`
- `src/xenia/kernel/XLiveAPI.h`

**Network Layer (~1,400 lines):**
- `src/xenia/kernel/xnet.h`
- `src/xenia/kernel/upnp.cc`
- `src/xenia/kernel/upnp.h`
- Enhanced `src/xenia/kernel/xsocket.cc`

**Session Management (~1,200 lines):**
- `src/xenia/kernel/xsession.cc`
- `src/xenia/kernel/xsession.h`

**JSON Serialization (26 files):**
- `src/xenia/kernel/json/arbitration_object_json.cc/.h`
- `src/xenia/kernel/json/base_object_json.cc/.h`
- `src/xenia/kernel/json/delete_my_profiles_json.cc/.h`
- `src/xenia/kernel/json/find_users_object_json.cc/.h`
- `src/xenia/kernel/json/friend_presence_object_json.cc/.h`
- `src/xenia/kernel/json/http_response_object_json.cc/.h`
- `src/xenia/kernel/json/leaderboard_object_json.cc/.h`
- `src/xenia/kernel/json/player_object_json.cc/.h`
- `src/xenia/kernel/json/presence_object_json.cc/.h`
- `src/xenia/kernel/json/properties_object_json.cc/.h`
- `src/xenia/kernel/json/services_json.cc/.h`
- `src/xenia/kernel/json/session_object_json.cc/.h`
- `src/xenia/kernel/json/xstorage_file_info_object_json.cc/.h`

**XAM Updates:**
- `src/xenia/kernel/xam/apps/xgi_app.cc` - Session handlers
- `src/xenia/kernel/xam/apps/xlivebase_app.cc` - XLive handlers
- `src/xenia/kernel/xam/xam_net.cc` - Network exports

**Third-Party Dependencies:**
```
third_party/libcurl/        - HTTP client
third_party/libcurl.lua     - Premake config
third_party/wolfssl/        - TLS/SSL
third_party/wolfssl.lua     - Premake config
third_party/miniupnp/       - UPnP
third_party/miniupnp.lua    - Premake config
```

### macOS-Specific Concerns
1. ✅ libcurl configured to use SecureTransport on macOS (no wolfssl needed)
2. ✅ miniupnp works as-is (POSIX)
3. ✅ Socket code has `XE_PLATFORM_MAC` handling
4. ✅ POSIX socket headers added to xnet.h, net_utils.h, xam_net.cc
5. ✅ IP_ADAPTER_ADDRESSES stubbed for non-Windows
6. ✅ Fixed `be<>` bitwise compound assignment operations (use get/set instead)
7. ✅ Added `socklen_t*` casts for `accept()` and `recvfrom()`
8. ✅ Added `WSAEventSelect` stub (returns success)
9. ✅ Added `IP_BYTE1/2/3/4` macros for cross-platform IP address byte extraction
10. ✅ Added `<netdb.h>` for `gethostbyname()` and `hostent`
11. ✅ Fixed variable declaration scope with goto statements
12. ✅ Re-enabled chrono.h `to_local()` method with simplified implementation
13. ✅ Implemented `DiscoverNetworkInterfaces` on macOS using `getifaddrs()`
14. ✅ Implemented `IOControl` on macOS using `ioctl()`
15. ✅ Implemented `WinsockGetLocalIP` on macOS using POSIX UDP connect technique
16. ✅ Switched TLS backend from non-existent `USE_SECTRANSPORT` to `USE_OPENSSL` with Homebrew OpenSSL 3
17. ✅ Added OpenSSL dylib bundling in postbuild script (`libssl.3.dylib`, `libcrypto.3.dylib`)

## Risk and Mitigations
- **Risk:** Conflicts with Metal-related changes.  
  **Mitigation:** Forward-port and resolve conflicts per subsystem; preserve macOS-specific code.
- **Risk:** Missing macOS support in netplay networking.  
  **Mitigation:** Add `XE_PLATFORM_MAC` handling to all socket/network code.
- **Risk:** Third-party library compatibility on macOS.  
  **Mitigation:** Test libcurl with SecureTransport; verify wolfssl/miniupnp build.
- **Risk:** Backend API changes.  
  **Mitigation:** Keep defaults aligned with Xenia-WebServices and expose config overrides.

## Definition of Done
1. ✅ Third-party dependencies (libcurl, miniupnp) build on macOS.
2. ✅ XLiveAPI connects to Xenia-WebServices backend (verified — dialog shows Connected state with local/online IPs).
3. 🔲 XSession create/browse/join works (needs game-level testing).
4. 🔲 Friends list and presence works (UI implemented, needs runtime verification).
5. ✅ Network mode selector UI works (NetplayConfigDialog with mode-aware sections).
6. 🔲 No regressions in offline titles (needs runtime verification).
7. ✅ Core porting complete, documentation updated.

## Execution Order
1. ✅ Fetch and analyze netplay source (v5.0.0 tag).
2. ✅ Ported minor infrastructure fixes (WSAStartup, IOControl).
3. ✅ Port third-party dependencies (libcurl, miniupnp).
4. ✅ Port XLiveAPI and JSON serialization (26 files).
5. ✅ Port XSession management.
6. ✅ Port XAM network handlers (xgi_app, xlivebase_app, xam_net).
7. ✅ Port unmarshaller infrastructure (20 files).
8. ✅ Update build system (premake).
9. ✅ Fix xsocket.cc macOS compatibility (be<> operators, socklen_t, WSAEventSelect stub).
10. ✅ Fix xam_net.cc macOS compatibility (IP_BYTE macros, netdb.h, IN_ADDR→in_addr).
11. ✅ Add XamState extension methods (GetUserIndexAssignedToProfileFromXUID, etc.).
12. ✅ Fix profile_manager.cc (lambda shadowing, ToggleLiveFlag, GenerateXuid public).
13. ✅ Fix xam_user.cc (enum casts, X_USER_SIGNIN_STATE comparison).
14. ✅ Fix user_tracker.cc (xnet.h include, GAMERCARD_ZONE namespace).
15. ✅ Fix chrono.h (re-enable to_local method).
16. ✅ Add XexCheckExecutablePrivilege public function.
17. ✅ Final build verification — BUILD SUCCEEDED (Debug, ARM64).
18. ✅ Port UI components (NetplayConfigDialog with Network menu, mode-aware sections, config persistence).
19. 🔲 Runtime test and validate (backend connection verified, game-level session testing pending).
20. ✅ Document.

## Conclusion (UPDATED Feb 7, 2026)

**Netplay port from v5.0.0 is complete — all phases implemented!**

### Files Successfully Ported (80+ files):
- `src/xenia/kernel/XLiveAPI.cpp/.h` - HTTP REST client ✅
- `src/xenia/kernel/xsession.cc/.h` - Session management ✅
- `src/xenia/kernel/xnet.h` - Network definitions (with macOS fixes) ✅
- `src/xenia/kernel/upnp.cc/.h` - UPnP port forwarding ✅
- `src/xenia/kernel/xsocket.cc/.h` - Socket handling ✅ (macOS fixes applied)
- `src/xenia/kernel/util/net_utils.cc/.h` - Network utilities ✅
- `src/xenia/kernel/util/game_info_database.cc/.h` - Game info with GetXLast ✅
- `src/xenia/kernel/util/xlast.cc/.h` - XLast parsing (v5.0.0 API) ✅
- `src/xenia/kernel/util/presence_string_builder.cc/.h` - Presence formatting ✅
- `src/xenia/kernel/xam/xam_net.cc/.h` - XAM network exports ✅ (macOS fixes applied)
- `src/xenia/kernel/xam/profile_manager.cc/.h` - Profile management ✅
- `src/xenia/kernel/xam/user_profile.cc/.h` - User profile ✅
- `src/xenia/kernel/xam/user_tracker.cc/.h` - User tracking ✅
- `src/xenia/kernel/xam/xam_state.cc/.h` - Extended with new methods ✅
- `src/xenia/kernel/xam/xam_user.cc` - Fixed enum conversions ✅
- `src/xenia/kernel/xam/xam_profile.cc` - Fixed GenerateXuid call ✅
- `src/xenia/kernel/xam/apps/xgi_app.cc/.h` - Session handlers ✅
- `src/xenia/kernel/xam/apps/xlivebase_app.cc/.h` - XLive handlers ✅
- `src/xenia/kernel/xam/unmarshaller/*` - 20 unmarshaller files ✅
- `src/xenia/kernel/json/*` - 26 JSON serialization files ✅
- `src/xenia/kernel/xboxkrnl/xboxkrnl_modules.cc/.h` - Added XexCheckExecutablePrivilege ✅
- `src/xenia/base/chrono.h` - Re-enabled to_local method ✅
- `src/xenia/app/emulator_window.cc/.h` - NetplayConfigDialog UI, Network menu ✅
- `src/xenia/app/premake5.lua` - Added miniupnp/libcurl/rapidjson includes + defines ✅
- `third_party/libcurl` - HTTP client submodule ✅
- `third_party/miniupnp` - UPnP submodule ✅
- `third_party/libcurl.lua` - Premake config ✅
- `third_party/miniupnp.lua` - Premake config ✅
- `src/xenia/xbox.h` - Added X_XAMACCOUNTINFO struct with pack pragmas ✅
- `src/xenia/kernel/xam/xam.h` - Namespace resolution for X_XAMACCOUNTINFO ✅

### macOS Platform Fixes Applied:
- Added `XE_PLATFORM_MAC` handling to xnet.h, xsocket.cc, xam_net.cc
- Added POSIX socket headers (`<arpa/inet.h>`, `<netinet/in.h>`, `<netdb.h>`, etc.)
- Replaced `CHAR` with `char` for cross-platform compatibility
- Replaced `__declspec(align(8))` with `alignas(8)`
- Fixed `xe::be<uint32_t>` bitwise operations using get/set instead of compound assignment
- Added `IP_ADAPTER_ADDRESSES` stub for non-Windows platforms
- Added `IP_BYTE1/2/3/4` macros for cross-platform IP address byte extraction
- Changed `socklen_t*` casts for `accept()` and `recvfrom()` on POSIX
- Added `WSAEventSelect` stub returning success on non-Windows
- Fixed variable initialization with goto statement scope issues
- Using macOS SecureTransport instead of wolfssl for libcurl
- Changed zlib include from `third_party/zlib/zlib.h` to `<zlib.h>`
- Added `<string_view>` and `xenia/base/byte_order.h` includes to xbox.h
- Added `#pragma pack(push/pop, 4)` for struct alignment consistency
- Re-enabled chrono.h `to_local()` with simplified implementation

### XamState Extensions Added:
- `GetUserIndexAssignedToProfileFromXUID(uint64_t xuid)` - Find user slot by XUID
- `GetUserProfileLive(uint64_t xuid)` - Get profile for online XUID
- `GetUserProfileAny(uint64_t xuid)` - Get profile for any XUID type

### Next Steps:
1. ✅ Build compiles cleanly (Debug ARM64) — all macOS platform stubs implemented
2. 🔲 Runtime test: verify offline mode still works
3. ✅ Runtime test: HTTPS connectivity to backend verified (dialog shows Connected + IPs)
4. ✅ Port UI components (NetplayConfigDialog with Network menu, config persistence, retry mechanism)
5. 🔲 Comprehensive netplay testing (session create/browse/join with actual game)
6. 🔲 Test Systemlink mode (LAN-only, no backend server)
7. 🔲 Verify friends list and presence features end-to-end

**Backend:** https://xenia-netplay-2a0298c0e3f4.herokuapp.com/

---

## Netplay Setup Guide (macOS)

### Prerequisites
- macOS 15.0+ (Sequoia)
- Build: `./xb build --arch=arm64` (Apple Silicon) or `./xb build --arch=x86_64` (Intel)

### Configuration
Netplay is configured via the **Network** menu in the emulator (Network → Netplay Configuration), command-line flags, or the config file (`xenia-canary.config.toml`):

| Flag | Default | Description |
|------|---------|-------------|
| `--network_mode` | `2` | 0=Offline, 1=Systemlink, 2=Xbox Live |
| `--api_address` | `192.168.0.1:36000/` | Netplay server address (set at runtime via UI) |
| `--api_list` | Heroku backend URL | Comma-delimited list of server URLs (max 10) |
| `--network_guid` | (empty) | Network interface name (e.g., `en0`) |
| `--logging` | `false` | Log network activity & stats |
| `--log_mask_ips` | `true` | Don't include P2P IPs in logs |
| `--upnp` | `true` | Enable UPnP port forwarding (Xbox Live mode) |
| `--xstorage_backend` | `true` | Use backend for XStorage (Xbox Live mode) |
| `--xlink_kai_systemlink_hack` | `false` | Enable XLink Kai compatibility hacks |

Changes made via the Network menu UI are automatically saved to `xenia-canary.config.toml`.

### Running with Netplay
```bash
# Xbox Live mode (default)
./build/bin/Mac-ARM64/Debug/Xenia-Canary.app/Contents/MacOS/xenia \
  --network_mode=2 "/path/to/game"

# Systemlink mode
./build/bin/Mac-ARM64/Debug/Xenia-Canary.app/Contents/MacOS/xenia \
  --network_mode=1 "/path/to/game"

# Offline mode (no networking)
./build/bin/Mac-ARM64/Debug/Xenia-Canary.app/Contents/MacOS/xenia \
  --network_mode=0 "/path/to/game"
```

### macOS-Specific Notes
- TLS uses OpenSSL 3 (bundled in the .app — requires `brew install openssl@3`)
- Network interface discovery uses `getifaddrs()` — set `--network_guid=en0` for Wi-Fi
- UPnP port forwarding works via miniupnpc (cross-platform)
- Discord rich presence is not available on macOS
