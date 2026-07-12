---
title: "Format Specifiers"
desc: "Complete format specifier reference for date and datetime formatting"
author: "mroczect"
license: "GPL-3.0"
---

# Format Specifiers

This page describes all format specifiers supported by `date_format()` and
`datetime_format()` in `libcdaydiff`.

## Overview

Both formatting functions use a `printf`-style format string where `%`
introduces a specifier character. Ordinary characters (letters, digits,
symbols) are copied directly to the output buffer.

## Supported Specifiers

### Date Specifiers (available in both functions)

| Specifier | Description                    | Example Output               |
| --------- | ------------------------------ | ---------------------------- |
| `%Y`      | Four‑digit year                | `2026`                       |
| `%m`      | Two‑digit month (01–12)        | `05`                         |
| `%d`      | Two‑digit day of month (01–31) | `06`                         |
| `%B`      | Full month name                | `May`                        |
| `%b`      | Abbreviated month name         | `May` (same as `%B` for May) |
| `%A`      | Full weekday name              | `Wednesday`                  |
| `%a`      | Abbreviated weekday name       | `Wed`                        |
| `%%`      | A literal `%` character        | `%`                          |

### Time Specifiers (only available in `datetime_format()`)

| Specifier | Description                           | Example Output |
| --------- | ------------------------------------- | -------------- |
| `%H`      | Two‑digit hour (24‑hour clock, 00–23) | `14`           |
| `%M`      | Two‑digit minute (00–59)              | `30`           |
| `%S`      | Two‑digit second (00–59)              | `45`           |
| `%z`      | Timezone offset as `±HHMM`            | `+0700`        |

## Common Format Strings

| Format String                | Example Output                    |
| ---------------------------- | --------------------------------- |
| `"%Y-%m-%d"`                 | `2026-05-06`                      |
| `"%d/%m/%Y"`                 | `06/05/2026`                      |
| `"%A, %B %d, %Y"`            | `Wednesday, May 06, 2026`         |
| `"%d-%b-%Y"`                 | `06-May-2026`                     |
| `"%Y-%m-%dT%H:%M:%S%z"`      | `2026-05-06T14:30:45+0700`        |
| `"%H:%M:%S"`                 | `14:30:45`                        |
| `"%a, %d %b %Y %H:%M:%S %z"` | `Wed, 06 May 2026 14:30:45 +0700` |

## Error Handling

- If the format string contains an unrecognised specifier (e.g., `%X`), the
  function returns `DATE_ERR_FORMAT`.
- If the output buffer is too small to hold the formatted string, the function
  returns `DATE_ERR_FORMAT`. Always make sure the buffer has enough space for
  the expected output plus the null terminator.

## Notes

- Weekday names (`%A`, `%a`) are calculated automatically from the date
  using Zeller's congruence.
- Month names (`%B`, `%b`) are in English.
- The timezone offset `%z` is taken directly from the `tz_offset` field of
  the `DateTime` struct; it does not query the system timezone.
- The library does not support locale‑specific or translated strings.
