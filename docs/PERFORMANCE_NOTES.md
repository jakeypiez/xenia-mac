# Xenia macOS Performance Analysis Notes

**Date:** February 4, 2026  
**Game Tested:** GoldenEye 007 XBLA (Title ID: 584108A9)  
**Platform:** Apple Silicon Mac (M5)  
**Build:** Release (ARM64)

---

## Baseline Performance Metrics

**Test:** 30-second run with vsync disabled, Release build

| Metric | Value |
|--------|-------|
| Total Frames | 26,580 |
| Average FPS | **860 fps** |
| Average Frame Time | 1.16 ms |
| Min Frame Time | 0.0004 ms |
| Max Frame Time | 148.34 ms (startup spike) |
| Frames below 60fps | 2 (0.0075%) |

**Note:** This baseline was captured with the game running at unlimited framerate (no vsync). The ~860 FPS indicates the emulator is running well above real-time on Apple Silicon.

---

## How to Run Performance Tests

### Enable FPS Logging

```bash
/path/to/xenia \
  --log_fps=true \
  --fps_log_file=/path/to/xenia_perf.csv \
  --vsync=false \
  "/path/to/game"
```

### Analyze Results

```bash
# Quick summary
awk -F',' 'NR>1 && $3 > 0 {count++; sum+=$3; if($3<min || min==0) min=$3; if($3>max) max=$3} END {print "Avg FPS:", 1000/(sum/count), "Min:", min, "Max:", max}' xenia_perf.csv

# Compare two runs
echo "Before:" && head -1 baseline.csv && tail -5 baseline.csv
echo "After:" && head -1 optimized.csv && tail -5 optimized.csv
```

### CSV Columns

| Column | Description |
|--------|-------------|
| timestamp_ms | Time since start (milliseconds) |
| frame_number | Frame counter |
| frame_time_ms | Time to render this frame (ms) |
| fps_instant | 1000 / frame_time_ms |
| fps_avg | Running average FPS |
| fps_1sec | 1-second rolling FPS window |
| min_frame_ms | Minimum frame time so far |
| max_frame_ms | Maximum frame time so far |

---

## Summary

The emulator runs GoldenEye successfully but with suboptimal performance. Analysis of debug logs reveals several areas for potential improvement.

---

## Key Findings

### 1. Metal Index Buffer Allocation Overhead (HIGH PRIORITY)

**Issue:** During a 30-second run, **232 new Metal index buffers** were created for primitive conversion, most at 4KB each.

**Location:** [metal_primitive_processor.cc](src/xenia/gpu/metal/metal_primitive_processor.cc#L200)

**Root Cause:** The buffer pooling algorithm creates a new buffer whenever existing buffers are in use. In GoldenEye's case, this results in excessive allocations.

**Potential Fix:**
```cpp
// Current: Creates new 4KB buffer when all existing buffers are in-use
// Improvement: Pre-allocate a larger pool or use a ring buffer approach

// Consider increasing minimum allocation or using a frame-delayed reuse strategy
size_t allocation_size = std::max(required_size, size_t(16384));  // 16KB minimum instead of 4KB
```

**Impact:** GPU memory allocation is expensive; reducing allocations could significantly improve frame times.

---

### 2. Metal Binary Archive Initialization Failed

**Warning:** `Metal binary archive init failed: Invalid URL`

**Location:** [metal_command_processor.cc](src/xenia/gpu/metal/metal_command_processor.cc#L1431)

**Impact:** Pipeline caching not working optimally. This means shaders may be recompiled more often than necessary.

**Potential Fix:** Check the archive path construction - likely an issue with the cache directory path or permissions.

---

### 3. Excessive File Resolution Attempts

**Issue:** The game makes hundreds of failed file resolution attempts for optional content:
- Developer head textures (headstevee, headjoel, etc. - Rare employee Easter eggs)
- Optional texture variations (`\files\new\` vs `\files\original\`)
- Blood animation frames (titleblood/frame01-40)

**Impact:** Low - these are expected failures for missing optional content. However, the logging overhead could be reduced.

**Recommendation:** Add a file existence cache to avoid repeated lookups for the same missing files.

---

### 4. Logging Overhead

**Current Config:** `log_level=2` (info level) produces 3341 lines in 30 seconds (~111 lines/sec)

**Major Log Spam Sources:**
- `xeRtlNtStatusToDosError` - Status code translations (informational)
- `Added handle` / `Removed handle` - Object lifecycle tracking
- `Metal index buffer` creation messages

**Recommendation for Production:**
```bash
--log_level=1          # Warning and above only
--log_to_stdout=false  # Don't write to terminal
--flush_log=false      # Batch flushes for better I/O
```

---

## Compilation/Build Optimizations

### Current Release Build Settings (premake5.lua)

```lua
filter("configurations:Release")
  runtime("Release")
  optimize("Speed")           -- Uses -O3
  flags("NoBufferSecurityCheck")
  inlining("Auto")
  -- Note: LTO only enabled for Windows
```

### Recommendations for macOS ARM64

1. **Enable LTO (Link-Time Optimization):**
   ```lua
   filter({"configurations:Release", "platforms:Mac"})
     linktimeoptimization("On")
   ```

2. **ARM64-specific optimizations:**
   ```lua
   filter({"configurations:Release", "architecture:ARM64"})
     buildoptions({
       "-mcpu=apple-m1",     -- Or -mcpu=native for best performance
       "-mtune=native",
     })
   ```

3. **Consider Profile-Guided Optimization (PGO):**
   - Run instrumented build
   - Collect profile data from typical gameplay
   - Rebuild with profile data

---

## Performance Flags to Try

### GPU Flags

| Flag | Default | Recommendation | Notes |
|------|---------|----------------|-------|
| `--vsync` | true | `false` | Removes 60fps cap |
| `--framerate_limit` | 0 | `0` | Keep unlimited |
| `--metal_pipeline_disk_cache` | true | `true` | Keep enabled |
| `--metal_shader_disk_cache` | true | `true` | Reduces recompilation |
| `--metal_use_heaps` | true | `true` | Better memory management |
| `--metal_shared_memory_zero_copy` | true | `true` | Unified memory optimization |

### CPU Flags

| Flag | Default | Recommendation | Notes |
|------|---------|----------------|-------|
| `--inline_mmio_access` | true | `true` | Keep for performance |
| `--disable_context_promotion` | false | `false` | Keep disabled |

---

## Kernel Implementation Status

From the log:
- **xboxkrnl:** 92% implemented (66/71 functions)
- **xam:** 97% implemented (126/129 functions)

Missing implementations are unlikely to affect GoldenEye performance.

---

## Recommended Next Steps

1. **HIGH:** Investigate Metal index buffer allocation strategy - consider larger pre-allocation or better pooling
2. **MEDIUM:** Fix Metal binary archive URL construction for proper pipeline caching
3. **LOW:** Add file existence cache to reduce repeated failed lookups
4. **LOW:** Consider reducing default log level for Release builds

---

## Quick Performance Test Command

```bash
# Run with optimized settings for performance testing
/path/to/xenia \
  --log_file=/dev/null \
  --log_to_stdout=false \
  --log_level=0 \
  --vsync=false \
  "/path/to/game"
```

---

## Hardware Info from Log

- **Metal Device:** Apple M5
- **GPU Thread Stack:** 70010000-70030000 (128KB)
- **Built-in Index Buffer:** 589,794 bytes
