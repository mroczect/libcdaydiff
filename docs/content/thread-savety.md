---
title: "Thread Safety Notes"
desc: "Thread safety guarantees and limitations of libcdaydiff"
author: "mroczect"
license: "GPL-3.0"
---

# Thread Safety Notes

This page describes the thread safety guarantees (and limitations) of the
`libcdaydiff` library.

## Summary

`libcdaydiff` is designed to be safe for use in multithreaded applications,
with the exception of a few platform‑specific edge cases noted below. In
general, you can call any function concurrently from multiple threads without
internal data races, provided each call operates on its own set of arguments.

## Design Principles

The library achieves thread safety by avoiding all mutable global state and
by using only re‑entrant standard library functions.

- **No internal static buffers** – All output is written to buffers supplied
  by the caller. Functions such as `date_format()` and `datetime_format()`
  do not share any hidden storage between calls.

- **No global error state** – Every function returns an error code directly.
  There is no `errno`‑style global variable that could be overwritten by
  another thread.

- **Re‑entrant time functions** – On POSIX systems (Linux, macOS), the library
  uses `gmtime_r()` and `localtime_r()` instead of the thread‑unsafe
  `gmtime()` and `localtime()`. On Windows, the `_s` variants (`gmtime_s`,
  `localtime_s`) are used through compatibility macros defined in the
  internal header.

## Functions That Are Always Thread‑Safe

The following functions operate solely on their arguments and have no side
effects on any hidden state. They can be called from any number of threads
simultaneously.

| Category         | Functions                                                                                  |
| ---------------- | ------------------------------------------------------------------------------------------ |
| Validation       | `date_is_leap_year`, `date_days_in_month`, `date_validate`                                 |
| Parsing          | `date_parse`                                                                               |
| Difference       | `date_diff_days`, `date_diff_days_excl`, `date_diff_full`                                  |
| Manipulation     | `date_add_days`, `date_add_months`, `date_add_years`                                       |
| Information      | `date_day_of_week`, `date_day_of_year`, `date_week_number_iso`                             |
| Comparison       | `date_compare`, `date_equals`                                                              |
| Formatting       | `date_format`, `datetime_format`                                                           |
| Epoch conversion | `date_to_epoch_utc`, `date_from_epoch_utc`, `date_to_epoch_local`, `date_from_epoch_local` |
| Error string     | `date_error_string`                                                                        |

## Functions Involving the System Clock

The functions that read the current time (`date_now`, `date_now_utc`,
`datetime_now`, `datetime_now_utc`, `datetime_now_tz`) are thread‑safe in
the sense that they do not interfere with each other. However, they depend on
the underlying system call `time()`, which typically returns a value that may
change between calls. This is not a thread‑safety issue; it simply means that
two threads calling `date_now()` at the same moment may get slightly different
results due to scheduling.

On systems that do not provide `gmtime_r` / `localtime_r` (very old or
non‑POSIX platforms), the library may fall back to the unsafe versions. In
practice, all modern desktop and server operating systems support the
re‑entrant functions.

## Platform‑Specific Considerations

### Linux / macOS / BSD

Fully thread‑safe. `gmtime_r` and `localtime_r` are part of POSIX.1‑2001.

### Windows (MSVC)

The library detects `_WIN32` and uses `gmtime_s` / `localtime_s` internally.
These are thread‑safe and recommended by Microsoft.

### Windows (MinGW‑w64)

MinGW‑w64 provides both `gmtime_r` and `gmtime_s`. The library uses the
`_WIN32` path and selects the appropriate version automatically.

### Embedded or Bare‑Metal Systems

If the standard library does not provide re‑entrant time functions, you
should serialise access to the time‑related functions from your application
code, or implement your own locking around them.

## Recommendations for Multithreaded Applications

- Do not share the same output buffer between threads unless you use external
  synchronisation (e.g., a mutex). Each thread should supply its own buffer
  for functions like `date_format`.

- The `Date`, `DateTime`, and `DateDiff` structures are plain C structs.
  They are safe to read from multiple threads as long as no thread is
  writing to them concurrently.

- If you use `datetime_now_tz()` in multiple threads, be aware that it
  internally calls `time()`. This is harmless but may give slightly different
  results if the system clock advances between calls.

## Guarantees

`libcdaydiff` will never crash due to a data race caused by its own internal
implementation when used correctly. If you encounter a threading issue,
please report it on the [GitHub issue tracker](https://github.com/mroczect/libcdaydiff/issues).

## Summary Table

| Scenario                                            | Safe? | Notes                             |
| --------------------------------------------------- | ----- | --------------------------------- |
| Multiple threads formatting different dates         | Yes   | Each call uses its own buffer     |
| Multiple threads parsing different strings          | Yes   | No shared state                   |
| Multiple threads calling `date_now()`               | Yes   | Uses `time()` + `localtime_r`     |
| Same buffer passed to multiple threads              | No    | External synchronisation required |
| Same `Date*` written by one thread, read by another | No    | Use atomic operations or mutex    |
