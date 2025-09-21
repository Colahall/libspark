# libspark

**libspark** is a lightweight, high-performance C library for fundamental
Digital Signal Processing (DSP) operations. It provides portable, SIMD-aware
kernels intended for **real-time audio and signal processing** applications,
from embedded devices to desktop software.

> **⚠️ Work in progress:** SPARK is still in early alpha. APIs may change.


## Philosophy

* **Speed first**: Kernels are hand-tuned in C with optional SIMD (SSE/AVX/NEON) optimizations.
* **Minimalism**: No external dependencies, no hidden runtime. Just DSP building blocks.
* **Portability**: Written in C99 for broad compiler and platform support.
* **Modularity**: Focused, composable routines you can drop into any project.

## Features

* **Filter primitives**: biquads, EQ sections, and related math.
* **Buffer utilities**: memory-safe operations for interleaved, planar, and pointer-to-pointer layouts.
* **Cross-platform**: Linux, macOS, and Windows (with Meson toolchain).
* **Permissive license**: MIT-licensed for use in both open-source and proprietary projects.

## Build Instructions

### Requirements

* C99-compatible compiler (GCC, Clang, MSVC)
* [Meson](https://mesonbuild.com) and [Ninja](https://ninja-build.org)

### Build & Install

```bash
# 1. Clone the repo
git clone https://github.com/your-org/libspark.git
cd libspark

# 2. Configure
meson setup builddir

# 3. Build
meson compile -C builddir

# 4. Install (optional)
meson install -C builddir
```

## License

Released under the **MIT License**. You are free to use libspark in commercial,
open-source, and academic projects.

## Roadmap

* [x] Biquad filter kernels
* [ ] SIMD multi-channel EQ
* [ ] Buffer utilities for interleaved/planar layouts
* [ ] FFT wrappers and convolution primitives
* [ ] Continuous integration across Linux/macOS/Windows
