---
title: "Contributing Guide"
desc: "How to contribute to libcdaydiff"
author: "mroczect"
license: "GPL-3.0"
---

# Contributing Guide

Thank you for your interest in contributing to `libcdaydiff`. This document
explains how to set up your environment, propose changes, and submit them for
review.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Setting Up the Development Environment](#setting-up-the-development-environment)
- [Coding Standards](#coding-standards)
- [Testing](#testing)
- [Submitting Changes](#submitting-changes)
- [Pull Request Checklist](#pull-request-checklist)
- [Reporting Bugs](#reporting-bugs)
- [Requesting Features](#requesting-features)
- [License](#license)

## Code of Conduct

All contributors are expected to treat others with respect. Harassment,
offensive language, and disruptive behaviour are not tolerated. Use common
sense and be professional.

## Getting Started

1. Fork the repository on GitHub:
   [https://github.com/mroczect/libcdaydiff](https://github.com/mroczect/libcdaydiff)

2. Clone your fork locally:

   ```bash
   git clone https://github.com/YOUR_USERNAME/libcdaydiff.git
   cd libcdaydiff
   ```

3. Add the upstream repository as a remote:

   ```bash
   git remote add upstream https://github.com/mroczect/libcdaydiff.git
   ```

4. Create a branch for your work:

   ```bash
   git checkout -b feature/my-feature
   ```

   Use a descriptive branch name. Prefixes like `feature/`, `fix/`, or
   `docs/` are helpful.

## Setting Up the Development Environment

You can use either CMake or GNU Make to build the library during development.

### CMake (recommended)

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
ctest
```

### GNU Make

```bash
make
gcc -std=c99 -Isrc/include test.c libcdaydiff.a -o test_date
./test_date
```

Make sure all tests pass before making any changes.

## Coding Standards

- **C standard**: C99, with `-Wall -Wextra -Wpedantic` and no warnings.
- **Formatting**: Use the style that is consistent with the existing code
  (K&R‑like with 2‑space indentation).
- **Naming**: Public functions use `date_` or `datetime_` prefixes. Internal
  helpers use a leading underscore (e.g., `_days_since_epoch`).
- **Comments**: Use `/* */` for multiline comments and `//` for short inline
  notes where it aids readability. Doxygen‑style comments are used in the
  public header.
- **Portability**: Avoid non‑standard extensions unless guarded by `#ifdef`.
  The library must compile on Linux, macOS, Windows (MSVC and MinGW), and
  POSIX‑compliant systems.
- **Error handling**: Every function that can fail must return a `DateError`
  and must check all pointer arguments for `NULL`.

## Testing

The project has a comprehensive test suite in `test.c`. It covers:

- Parsing (valid and invalid inputs)
- Validation (leap years, month lengths)
- Date differences (inclusive, exclusive, full)
- Date manipulation (days, months, years)
- Day‑of‑week, day‑of‑year, ISO week
- Formatting (date and datetime)
- Comparison
- Real‑time functions (where system clock is available)
- Epoch conversions (UTC and local)
- Error strings

### Running the Tests

```bash
# Using the Makefile
make
gcc -std=c99 -Isrc/include test.c libcdaydiff.a -o test_date
./test_date

# Using CMake + CTest
cd build && cmake .. && cmake --build . && ctest
```

All tests must pass before you submit a pull request. If you add new
functionality, please add corresponding tests.

### Adding a New Test

Edit `test.c` and follow the existing pattern:

```c
printf("\n=== My New Tests ===\n");
Date d = {1, 1, 2020};
TEST("my new test");
{
    int rc = my_new_function(&d);
    CHECK(rc == DATE_OK);
}
```

`TEST()` prints the test name. `CHECK()` evaluates a condition and records
a pass or fail.

## Submitting Changes

1. Commit your changes with a clear, descriptive message:

   ```bash
   git add .
   git commit -m "feat: add function to calculate end of quarter"
   ```

   Follow the [Conventional Commits](https://www.conventionalcommits.org/)
   format if possible.

2. Keep your branch up to date with upstream:

   ```bash
   git fetch upstream
   git rebase upstream/master
   ```

3. Push to your fork:

   ```bash
   git push origin feature/my-feature
   ```

4. Open a pull request on GitHub. Include:
   - A brief description of the change.
   - Why the change is needed.
   - Any potential side effects or breaking changes.
   - Links to related issues, if applicable.

## Pull Request Checklist

Before submitting, verify that:

- [ ] Code compiles with `-Wall -Wextra -Wpedantic` without warnings on GCC
      and Clang.
- [ ] All existing tests pass (`ctest` or `./test_date`).
- [ ] New functions are tested in `test.c`.
- [ ] New functions are declared in `src/include/libcdaydiff.h` if they are
      part of the public API.
- [ ] New internal helpers are declared in
      `src/internal/libcdaydiff_internal.h`.
- [ ] Documentation pages (if any) are updated in `docs/`.
- [ ] No trailing whitespace or unrelated formatting changes.

## Reporting Bugs

If you find a bug, please open an issue on GitHub with the following
information:

- Library version (or commit hash).
- Operating system and compiler version.
- Minimal code example that reproduces the bug.
- Expected behaviour and actual behaviour.

## Requesting Features

Feature requests are welcome. Open an issue and describe:

- What you would like the library to do.
- Why the feature would be useful.
- Any ideas for how it might be implemented.

## License

By contributing, you agree that your code will be distributed under the same
GNU General Public License v3.0 that covers the project. See the
[LICENSE](license.md) file for the full text.
