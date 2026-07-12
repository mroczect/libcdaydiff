---
title: "Installation Guide"
desc: "How to build and install libcdaydiff on various platforms"
author: "mroczect"
license: "GPL-3.0"
---

# Installation Guide

This guide provides detailed instructions for building and installing
`libcdaydiff` on various platforms and build systems.

## Table of Contents

- [System Requirements](#system-requirements)
- [Method 1: Using CMake (Recommended)](#method-1-using-cmake-recommended)
  - [Linux / macOS](#linux--macos)
  - [Windows (Visual Studio)](#windows-visual-studio)
  - [Windows (MinGW-w64)](#windows-mingw-w64)
  - [CMake Options](#cmake-options)
- [Method 2: Using GNU Make](#method-2-using-gnu-make)
- [Method 3: Manual Compilation](#method-3-manual-compilation)
- [Verifying the Installation](#verifying-the-installation)
- [Uninstalling](#uninstalling)
- [Troubleshooting](#troubleshooting)

---

## System Requirements

| Requirement        | Minimum Version                                       |
| ------------------ | ----------------------------------------------------- |
| C Compiler         | C99 compliant (GCC 4.9+, Clang 3.3+, MSVC 2013+)      |
| CMake              | 3.10+ (only if using CMake)                           |
| GNU Make           | 3.81+ (only if using Make)                            |
| Operating System   | Linux, macOS, Windows, or any POSIX-compatible system |
| Standard C Library | Any (glibc, musl, msvcrt, etc.)                       |

No external libraries or dependencies are required.

---

## Method 1: Using CMake (Recommended)

CMake provides the most flexible build experience, supporting both static and
shared library builds, unit testing, and system-wide installation with
package configuration files for `find_package()`.

### Linux / macOS

**Step 1: Clone the repository**

```bash
git clone https://github.com/mroczect/libcdaydiff.git
cd libcdaydiff
```

**Step 2: Configure the build**

```bash
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
```

| Option                   | Description                                                | Default             |
| ------------------------ | ---------------------------------------------------------- | ------------------- |
| `-DCMAKE_INSTALL_PREFIX` | Where to install the library and headers                   | `/usr/local`        |
| `-DBUILD_SHARED_LIBS=ON` | Build shared library instead of static                     | `OFF` (builds both) |
| `-DCMAKE_BUILD_TYPE`     | Build configuration (`Debug`, `Release`, `RelWithDebInfo`) | `Release`           |

**Step 3: Build**

```bash
cmake --build .
```

**Step 4: Run tests (optional but recommended)**

```bash
ctest
```

All tests should pass. If any fail, please open an issue on GitHub.

**Step 5: Install**

```bash
sudo cmake --install .
```

This installs:

- Header: `/usr/local/include/libcdaydiff.h`
- Static library: `/usr/local/lib/libcdaydiff.a`
- Shared library: `/usr/local/lib/libcdaydiff.so` (or `.dylib` on macOS)
- CMake package: `/usr/local/lib/cmake/libcdaydiff/`

**Using the installed library in your CMake project**

```cmake
find_package(libcdaydiff REQUIRED)
target_link_libraries(your_app PRIVATE libcdaydiff::static)
# or
target_link_libraries(your_app PRIVATE libcdaydiff::shared)
```

### Windows (Visual Studio)

**Step 1: Open Developer Command Prompt**

Open "Developer Command Prompt for VS 2022" (or your Visual Studio version).

**Step 2: Clone and configure**

```cmd
git clone https://github.com/mroczect/libcdaydiff.git
cd libcdaydiff
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_INSTALL_PREFIX=C:\libs\libcdaydiff
```

For 32-bit builds, use `-A Win32` instead of `-A x64`.

**Step 3: Build**

```cmd
cmake --build . --config Release
```

**Step 4: Run tests**

```cmd
ctest -C Release
```

**Step 5: Install**

```cmd
cmake --install . --config Release
```

This installs:

- Header: `C:\libs\libcdaydiff\include\libcdaydiff.h`
- Static library: `C:\libs\libcdaydiff\lib\cdaydiff.lib`
- Shared library: `C:\libs\libcdaydiff\bin\cdaydiff.dll`

### Windows (MinGW-w64)

**Step 1: Install MinGW-w64**

Download from [winlibs.com](https://winlibs.com/) or use MSYS2:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake
```

**Step 2: Clone and configure**

```bash
git clone https://github.com/mroczect/libcdaydiff.git
cd libcdaydiff
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=/c/libs/libcdaydiff
```

**Step 3: Build**

```bash
mingw32-make
```

**Step 4: Run tests**

```bash
ctest
```

**Step 5: Install**

```bash
mingw32-make install
```

### CMake Options

| Option                 | Type   | Default            | Description                                         |
| ---------------------- | ------ | ------------------ | --------------------------------------------------- |
| `CMAKE_INSTALL_PREFIX` | PATH   | (system-dependent) | Installation root directory                         |
| `CMAKE_BUILD_TYPE`     | STRING | Release            | `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`  |
| `BUILD_SHARED_LIBS`    | BOOL   | OFF                | Build shared library (CMake builds both by default) |
| `CMAKE_C_COMPILER`     | PATH   | (auto)             | Path to the C compiler (e.g., `gcc`, `clang`)       |

---

## Method 2: Using GNU Make

A basic `Makefile` is included for quick builds on Unix-like systems. It only
builds the static library.

**Step 1: Clone the repository**

```bash
git clone https://github.com/mroczect/libcdaydiff.git
cd libcdaydiff
```

**Step 2: Build**

```bash
make
```

This produces `libcdaydiff.a` in the project root. Object files are placed
in the `obj/` directory.

**Available targets:**

| Target    | Description                                |
| --------- | ------------------------------------------ |
| `all`     | Build the static library (default)         |
| `clean`   | Remove all build artifacts                 |
| `install` | Install library and header to `/usr/local` |

**Override compiler or flags:**

```bash
make CC=clang CFLAGS="-O3 -march=native"
```

**Install (optional):**

```bash
sudo make install
```

---

## Method 3: Manual Compilation

If you prefer not to use any build system, you can compile the source files
directly.

**Step 1: Clone the repository**

```bash
git clone https://github.com/mroczect/libcdaydiff.git
cd libcdaydiff
```

**Step 2: Compile all source files**

```bash
gcc -std=c99 -Wall -Wextra -c \
    src/core.c src/parser.c src/diff.c src/manip.c \
    src/fmt.c src/tz.c src/epoch.c src/error.c \
    src/info.c src/comparison.c \
    -Isrc/include -Isrc/internal
```

**Step 3: Create the static library**

```bash
ar rcs libcdaydiff.a *.o
rm *.o
```

The library `libcdaydiff.a` is now ready to be linked.

**Using the library:**

```bash
gcc -std=c99 your_program.c -Isrc/include libcdaydiff.a -o your_program
```

---

## Verifying the Installation

To confirm the library is correctly installed and functional:

**Step 1: Create a test file** `check.c`:

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    Date d;
    if (date_parse("01.01.2020", NULL, &d) == DATE_OK) {
        printf("OK: %04d-%02d-%02d\n", d.year, d.month, d.day);
        return 0;
    }
    return 1;
}
```

**Step 2: Compile and run:**

```bash
gcc -std=c99 check.c -lcdaydiff -o check
./check
```

Expected output:

```
OK: 2020-01-01
```

---

## Uninstalling

### CMake (Linux/macOS)

```bash
# From the build directory
sudo cmake --build . --target uninstall
# Or manually remove files:
sudo rm /usr/local/include/libcdaydiff.h
sudo rm /usr/local/lib/libcdaydiff.a
sudo rm /usr/local/lib/libcdaydiff.so   # or .dylib
sudo rm -rf /usr/local/lib/cmake/libcdaydiff
```

### CMake (Windows)

Delete the installation directory (e.g., `C:\libs\libcdaydiff`).

### Make (Linux/macOS)

Manually remove:

```bash
sudo rm /usr/local/include/libcdaydiff.h
sudo rm /usr/local/lib/libcdaydiff.a
```

---

## Troubleshooting

### Problem: `cmake` command not found

**Solution:** Install CMake from your package manager:

- **Ubuntu/Debian:** `sudo apt install cmake`
- **Fedora:** `sudo dnf install cmake`
- **macOS (Homebrew):** `brew install cmake`
- **Windows:** Download from [cmake.org](https://cmake.org/download/)

### Problem: `error: unknown type name 'size_t'`

**Solution:** Ensure you are using a C99 compiler and that the standard
library headers are available. Try adding `-std=c99` to your compiler flags.

### Problem: `error: 'struct tm' has no member named 'tm_gmtoff'`

**Solution:** This is a known portability issue with some platforms (older
MSVC, MinGW). The library now uses a portable offset calculation. Ensure you
are using the latest version from the repository.

### Problem: Tests fail on `datetime_now_tz`

**Solution:** This test requires a working `time()` function. On some
embedded or sandboxed environments, `time()` may not be available or may
return `-1`. This does not affect the core date logic.

### Problem: `make: command not found` on Windows

**Solution:** Use CMake with the MinGW generator, or install GNU Make via
MSYS2, Cygwin, or Chocolatey (`choco install make`).

---

## Next Steps

- Follow the [Quick Start Guide](quick-start.md) to write your first program.
- Browse the [API Reference](api-reference.md) for complete function documentation.
- See [Examples](example.md) for more advanced usage scenarios.

```

```
