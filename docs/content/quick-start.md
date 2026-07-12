---
title: "Quick Start Guide"
desc: "Get started with libcdaydiff in minutes"
author: "mroczect"
license: "GPL-3.0"
---

# Quick Start Guide

This guide will help you get `libcdaydiff` up and running in your C project
within minutes.

## 1. Obtain the Library

Clone the repository from GitHub:

```bash
git clone https://github.com/mroczect/libcdaydiff.git
cd libcdaydiff
```

## 2. Build the Library

Choose one of the following methods.

### Using CMake (recommended)

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

This produces both a static library (`libcdaydiff.a`) and a shared library
(`libcdaydiff.so` or `.dylib`).

### Using GNU Make (Unix-like systems)

```bash
make
```

This builds the static library `libcdaydiff.a` in the project root.

## 3. Write Your First Program

Create a file named `example.c` with the following content:

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    Date d1, d2;
    int64_t days;

    // Parse two dates (auto-detect delimiter)
    date_parse("06.05.2026", NULL, &d1);
    date_parse("08.12.2026", NULL, &d2);

    // Calculate inclusive day difference
    date_diff_days(&d1, &d2, &days);
    printf("Difference: %lld days\n", (long long)days);

    // Format a date as a human-readable string
    char buf[100];
    date_format(&d1, "%A, %d %B %Y", buf, sizeof(buf));
    printf("Date: %s\n", buf);

    return 0;
}
```

## 4. Compile and Run

### If you installed the library system-wide

```bash
gcc -std=c99 example.c -lcdaydiff -o example
./example
```

### If you built locally without installing

```bash
gcc -std=c99 -Isrc/include example.c libcdaydiff.a -o example
./example
```

**Expected output:**

```
Difference: 217 days
Date: Wednesday, 06 May 2026
```

## What Just Happened?

1. **Parsing** – `date_parse` converted the strings `"06.05.2026"` and
   `"08.12.2026"` into `Date` structures. The `NULL` argument tells the
   function to auto-detect the delimiter (`.` in this case).

2. **Difference** – `date_diff_days` calculated the inclusive number of days
   between the two dates. The result is 217 because both the start and end
   dates are counted.

3. **Formatting** – `date_format` produced a human-readable string using
   specifiers:
   - `%A` – full weekday name (Wednesday)
   - `%d` – day of the month (06)
   - `%B` – full month name (May)
   - `%Y` – four-digit year (2026)

## Next Steps

- Read the [API Reference](api-reference.md) for a complete list of
  functions.
- See more [Examples](example.md) covering timezones, epoch conversion,
  and date arithmetic.
- Review [Installation](installation.md) for detailed platform-specific
  instructions.
