# API Reference

Complete documentation for the `libcdaydiff` C library.

## Table of Contents

- [Data Types](#data-types)
- [Error Codes](#error-codes)
- [Core Validation](#core-validation)
- [Parsing](#parsing)
- [Date Difference](#date-difference)
- [Date Manipulation](#date-manipulation)
- [Date Information](#date-information)
- [Comparison](#comparison)
- [Current Time / Timezone](#current-time--timezone)
- [Formatting](#formatting)
- [Epoch Conversion](#epoch-conversion)
- [Error String](#error-string)

---

## Data Types

### `Date`

```c
typedef struct {
    int day;    /**< Day of the month (1–31) */
    int month;  /**< Month of the year (1–12) */
    int year;   /**< Year (1900–9999) */
} Date;
```

Represents a calendar date. All library functions that accept a `Date`
expect it to be valid according to `date_validate()`.

### `DateTime`

```c
typedef struct {
    Date date;      /**< The calendar date */
    int hour;       /**< Hour (0–23) */
    int minute;     /**< Minute (0–59) */
    int second;     /**< Second (0–59) */
    int tz_offset;  /**< UTC offset in minutes (e.g., +420 for WIB) */
} DateTime;
```

Extends `Date` with time and timezone information. The `tz_offset` field
represents the difference from UTC in minutes. Positive values are east of
UTC (e.g., +420 = UTC+7), negative values are west (e.g., -300 = UTC-5).

### `DateDiff`

```c
typedef struct {
    int years;   /**< Number of full years */
    int months;  /**< Number of full months (0–11) */
    int days;    /**< Remaining days (0–30) */
} DateDiff;
```

Represents a difference between two dates broken down into years, months,
and remaining days. The `days` field is always less than a full month.

---

## Error Codes

```c
typedef enum {
    DATE_OK = 0,
    DATE_ERR_NULL_POINTER,
    DATE_ERR_PARSE,
    DATE_ERR_INVALID_DATE,
    DATE_ERR_OVERFLOW,
    DATE_ERR_OUT_OF_RANGE,
    DATE_ERR_TIMEZONE,
    DATE_ERR_FORMAT
} DateError;
```

| Code                    | Value | Description                                          |
| ----------------------- | ----- | ---------------------------------------------------- |
| `DATE_OK`               | 0     | Operation completed successfully                     |
| `DATE_ERR_NULL_POINTER` | 1     | A required pointer argument was `NULL`               |
| `DATE_ERR_PARSE`        | 2     | Failed to parse the input string as a date           |
| `DATE_ERR_INVALID_DATE` | 3     | The date (day/month/year) is not valid               |
| `DATE_ERR_OVERFLOW`     | 4     | Arithmetic overflow or result out of supported range |
| `DATE_ERR_OUT_OF_RANGE` | 5     | Value is outside the allowed year range (1900–9999)  |
| `DATE_ERR_TIMEZONE`     | 6     | Timezone error (reserved for future use)             |
| `DATE_ERR_FORMAT`       | 7     | Format string invalid or output buffer too small     |

Every public function returns a `DateError` (or `int`) indicating success or
the specific failure reason. Always check the return value.

---

## Core Validation

### `date_is_leap_year`

```c
int date_is_leap_year(int year);
```

**Description:** Determines whether the given year is a leap year according
to Gregorian calendar rules.

**Parameters:**

- `year` – The year to test (should be ≥ 1).

**Returns:** `1` if the year is a leap year, `0` otherwise.

**Example:**

```c
date_is_leap_year(2024); // returns 1
date_is_leap_year(1900); // returns 0
date_is_leap_year(2000); // returns 1
```

---

### `date_days_in_month`

```c
int date_days_in_month(int month, int year);
```

**Description:** Returns the number of days in the specified month of the
given year, accounting for leap years.

**Parameters:**

- `month` – Month number (1–12).
- `year` – The year (used for February leap year check).

**Returns:** Number of days (28–31), or `0` if the month is invalid.

**Example:**

```c
date_days_in_month(2, 2024); // returns 29
date_days_in_month(2, 2023); // returns 28
date_days_in_month(4, 2020); // returns 30
```

---

### `date_validate`

```c
int date_validate(int day, int month, int year);
```

**Description:** Checks whether a given day/month/year combination
represents a valid date within the supported range (1900–9999).

**Parameters:**

- `day` – Day of the month (1–31).
- `month` – Month (1–12).
- `year` – Year (1900–9999).

**Returns:** `1` if valid, `0` if invalid.

**Example:**

```c
date_validate(29, 2, 2024);  // returns 1 (valid leap day)
date_validate(29, 2, 2023);  // returns 0 (not a leap year)
date_validate(31, 4, 2020);  // returns 0 (April has 30 days)
date_validate(1, 1, 1899);   // returns 0 (year out of range)
```

---

## Parsing

### `date_parse`

```c
int date_parse(const char *str, const char *delims, Date *out);
```

**Description:** Parses a date string in the format `DD<delim>MM<delim>YYYY`
into a `Date` structure.

**Parameters:**

- `str` – Input string to parse. Must not be `NULL`.
- `delims` – String containing allowed delimiter characters (e.g., `"./"`).
  If `NULL`, the function automatically detects the delimiter by looking for
  the first non-digit character in the string. The same delimiter must be
  used consistently throughout the string.
- `out` – Pointer to a `Date` that will receive the parsed result. Must not
  be `NULL`.

**Returns:**

- `DATE_OK` – Success.
- `DATE_ERR_NULL_POINTER` – `str` or `out` is `NULL`.
- `DATE_ERR_PARSE` – String format is invalid (wrong/mixed delimiters,
  missing parts, non-numeric characters where digits expected).
- `DATE_ERR_INVALID_DATE` – The parsed date does not exist (e.g., 32/01/2020
  or 29/02/2023).

**Details:**

- The expected order of components is **day, month, year**.
- Leading zeros are accepted but not required (e.g., `1-1-2020` is valid).
- The delimiter must be consistent unless an explicit delimiter set is
  provided that matches all occurrences.
- The year must be in the range 1900–9999 after parsing.

**Examples:**

```c
Date d;

// Auto-detect delimiter (dot)
date_parse("06.05.2026", NULL, &d);
// d = {6, 5, 2026}

// Explicit delimiter (slash)
date_parse("31/12/2024", "/", &d);
// d = {31, 12, 2024}

// Space as delimiter
date_parse("15 08 1999", " ", &d);
// d = {15, 8, 1999}

// Mixed delimiters (will fail with NULL, but can be allowed by specifying both)
date_parse("01.02-2020", ".-", &d);
// d = {1, 2, 2020}
```

---

## Date Difference

### `date_diff_days`

```c
int date_diff_days(const Date *d1, const Date *d2, int64_t *result);
```

**Description:** Computes the inclusive number of days between two dates.

**Parameters:**

- `d1` – Start date.
- `d2` – End date.
- `result` – Pointer to `int64_t` receiving the day count.

**Returns:** `DATE_OK` on success, or an error code.

**Details:** The count is **inclusive**, meaning the start date is counted as
day 1. If `d1` and `d2` are the same date, the result is `1`. The order does
not matter; the result can be negative if `d2` is before `d1`.

**Example:**

```c
Date d1 = {1, 1, 2020};
Date d2 = {10, 1, 2020};
int64_t days;
date_diff_days(&d1, &d2, &days); // days = 10 (1..10 inclusive)
```

---

### `date_diff_days_excl`

```c
int date_diff_days_excl(const Date *d1, const Date *d2, int64_t *result);
```

**Description:** Computes the exclusive number of days between two dates.

**Parameters:** Same as `date_diff_days`.

**Details:** The count is **exclusive**, meaning the start date is not
counted. If `d1` and `d2` are the same date, the result is `0`.

**Example:**

```c
Date d1 = {1, 1, 2020};
Date d2 = {10, 1, 2020};
int64_t days;
date_diff_days_excl(&d1, &d2, &days); // days = 9 (days between)
```

---

### `date_diff_full`

```c
int date_diff_full(const Date *d1, const Date *d2, DateDiff *out);
```

**Description:** Breaks down the difference between two dates into years,
months, and remaining days.

**Parameters:**

- `d1` – Start date.
- `d2` – End date (must be >= `d1`).
- `out` – Pointer to `DateDiff` receiving the breakdown.

**Returns:**

- `DATE_OK` on success.
- `DATE_ERR_INVALID_DATE` if `d1` > `d2`.

**Details:** The algorithm first adds as many whole years as possible without
exceeding `d2`, then whole months, then counts remaining days. The result is
always such that `d1 + (years, months, days) = d2`, with days less than a
full month. End-of-month pinning is applied during additions (e.g., 31 Jan →
29 Feb in a leap year).

**Example:**

```c
Date d1 = {15, 3, 2020};
Date d2 = {20, 6, 2021};
DateDiff diff;
date_diff_full(&d1, &d2, &diff);
// diff = {1, 3, 5}  (1 year, 3 months, 5 days)
```

---

## Date Manipulation

### `date_add_days`

```c
int date_add_days(const Date *in, int64_t n, Date *out);
```

**Description:** Adds a number of days to a date.

**Parameters:**

- `in` – Input date.
- `n` – Number of days to add (can be negative to subtract).
- `out` – Output date.

**Returns:** `DATE_OK`, or `DATE_ERR_OVERFLOW` if the result is outside
1900–9999.

**Example:**

```c
Date d = {31, 12, 2020};
Date next;
date_add_days(&d, 1, &next); // next = {1, 1, 2021}
```

---

### `date_add_months`

```c
int date_add_months(const Date *in, int n, Date *out);
```

**Description:** Adds a number of months to a date.

**Parameters:**

- `in` – Input date.
- `n` – Number of months to add (can be negative).
- `out` – Output date.

**Details:** If the resulting day exceeds the number of days in the target
month, the day is **pinned** to the last valid day of that month. For
example, January 31 + 1 month → February 28 (or 29 in a leap year).

**Example:**

```c
Date d = {31, 1, 2024};
Date next;
date_add_months(&d, 1, &next); // next = {29, 2, 2024} (pinned)
```

---

### `date_add_years`

```c
int date_add_years(const Date *in, int n, Date *out);
```

**Description:** Adds a number of years to a date.

**Parameters:** Similar to `date_add_months`.

**Details:** For February 29 in a leap year, adding 1 year results in
February 28 if the target year is not a leap year.

**Example:**

```c
Date d = {29, 2, 2024};
Date next;
date_add_years(&d, 1, &next); // next = {28, 2, 2025} (pinned)
```

---

## Date Information

### `date_day_of_week`

```c
int date_day_of_week(const Date *d);
```

**Description:** Returns the day of the week using Zeller's congruence.

**Returns:** `0` for Sunday, `1` for Monday, ..., `6` for Saturday. Returns
`-1` if the date is invalid.

**Example:**

```c
Date d = {1, 1, 2023};
date_day_of_week(&d); // returns 0 (Sunday)
```

---

### `date_day_of_year`

```c
int date_day_of_year(const Date *d);
```

**Description:** Returns the day number within the year (1–365 or 1–366 in
leap years). Returns `0` if the date is invalid.

**Example:**

```c
Date d = {31, 12, 2023};
date_day_of_year(&d); // returns 365
```

---

### `date_week_number_iso`

```c
int date_week_number_iso(const Date *d);
```

**Description:** Calculates the ISO 8601 week number (1–53) for the given
date. The week starts on Monday, and the first week of the year is the one
containing the first Thursday. Returns `0` on error.

**Example:**

```c
Date d = {3, 1, 2023};
date_week_number_iso(&d); // returns 1 (first Monday is Jan 2, week 1)
```

---

## Comparison

### `date_compare`

```c
int date_compare(const Date *d1, const Date *d2);
```

**Description:** Compares two dates.

**Returns:**

- `-1` if `d1` is earlier than `d2`.
- `0` if they are equal.
- `1` if `d1` is later than `d2`.

**Example:**

```c
Date d1 = {1, 1, 2020};
Date d2 = {1, 2, 2020};
date_compare(&d1, &d2); // returns -1
```

---

### `date_equals`

```c
int date_equals(const Date *d1, const Date *d2);
```

**Description:** Checks whether two dates are exactly equal.

**Returns:** `1` if equal, `0` otherwise.

---

## Current Time / Timezone

### `date_now`

```c
int date_now(Date *out);
```

**Description:** Writes the current local date into `out`.

---

### `date_now_utc`

```c
int date_now_utc(Date *out);
```

**Description:** Writes the current UTC date into `out`.

---

### `datetime_now`

```c
int datetime_now(DateTime *out);
```

**Description:** Writes the current local date and time into `out`, including
the local timezone offset in minutes.

---

### `datetime_now_utc`

```c
int datetime_now_utc(DateTime *out);
```

**Description:** Writes the current UTC date and time into `out`. The
`tz_offset` field is set to `0`.

---

### `datetime_now_tz`

```c
int datetime_now_tz(DateTime *out, int offset_minutes);
```

**Description:** Writes the current UTC date and time adjusted by the given
offset into `out`. The `tz_offset` field is set to `offset_minutes`.

**Parameters:**

- `out` – Pointer to `DateTime` receiving the result.
- `offset_minutes` – Offset from UTC in minutes (positive = east, negative =
  west). For example, WIB (UTC+7) is `420`, EST (UTC-5) is `-300`.

---

## Formatting

### `date_format`

```c
int date_format(const Date *d, const char *fmt, char *buf, size_t bufsize);
```

**Description:** Formats a date according to a format string into a
caller-provided buffer.

**Parameters:**

- `d` – Date to format.
- `fmt` – Format string (see [Format Specifiers](format-specifier.md)).
- `buf` – Output buffer.
- `bufsize` – Size of the output buffer in bytes.

**Returns:**

- `DATE_OK` on success.
- `DATE_ERR_NULL_POINTER` if `d`, `fmt`, or `buf` is `NULL`.
- `DATE_ERR_FORMAT` if the format string is invalid or the buffer is too small.

---

### `datetime_format`

```c
int datetime_format(const DateTime *dt, const char *fmt, char *buf,
                    size_t bufsize);
```

**Description:** Formats a date-time according to a format string. Supports
all specifiers of `date_format` plus `%H`, `%M`, `%S`, `%z`.

**Parameters:** Similar to `date_format` but with a `DateTime` instead of
`Date`.

---

## Epoch Conversion

### `date_to_epoch_utc`

```c
int date_to_epoch_utc(const Date *d, int64_t *epoch_secs);
```

**Description:** Converts a date (assumed 00:00:00 UTC) to Unix time (seconds
since 1970-01-01 00:00:00 UTC).

---

### `date_from_epoch_utc`

```c
int date_from_epoch_utc(int64_t epoch_secs, Date *out);
```

**Description:** Converts Unix time to a UTC date. Only non-negative epoch
values are accepted (epoch < 0 returns `DATE_ERR_OUT_OF_RANGE`).

---

### `date_to_epoch_local`

```c
int date_to_epoch_local(const Date *d, int64_t *epoch_secs);
```

**Description:** Converts a date (assumed 00:00:00 local time) to Unix time
by first converting to UTC using the system's local offset.

---

### `date_from_epoch_local`

```c
int date_from_epoch_local(int64_t epoch_secs, Date *out);
```

**Description:** Converts Unix time to a local date using the system's timezone.

---

## Error String

### `date_error_string`

```c
const char *date_error_string(DateError err);
```

**Description:** Returns a static, human-readable string describing the given
error code. The returned pointer must not be freed.

**Example:**

```c
int rc = date_parse("invalid", NULL, &d);
if (rc != DATE_OK) {
    printf("Error: %s\n", date_error_string(rc));
}
```
