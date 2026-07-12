```
.  # .
├── CMakeLists.txt
├── docs
│   ├── config.yml
│   ├── content
│   │   ├── api-reference.md
│   │   ├── contac.md
│   │   ├── contributing.md
│   │   ├── example.md
│   │   ├── format-specifier.md
│   │   ├── index.md
│   │   ├── installation.md
│   │   ├── license.md
│   │   ├── quick-start.md
│   │   ├── thread-savety.md
├── license
├── readme
├── snapcat.md
├── src
│   ├── comparison.c
│   ├── core.c
│   ├── diff.c
│   ├── epoch.c
│   ├── error.c
│   ├── fmt.c
│   ├── include
│   │   ├── libcdaydiff.h
│   ├── info.c
│   ├── internal
│   │   ├── libcdaydiff_internal.h
│   ├── manip.c
│   ├── parser.c
│   ├── tz.c
├── test.c
```
## ./src/include/libcdaydiff.h

```c
#ifndef LIBCDAYDIFF_H
#define LIBCDAYDIFF_H

#include <stdint.h>
#include <time.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int day;
    int month;
    int year;
} Date;

typedef struct {
    Date date;
    int hour;
    int minute;
    int second;
    int tz_offset;
} DateTime;

typedef struct {
    int years;
    int months;
    int days;
} DateDiff;

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

const char* date_error_string(DateError err);

int date_is_leap_year(int year);
int date_days_in_month(int month, int year);
int date_validate(int day, int month, int year);


int date_parse(const char *str, const char *delims, Date *out);

int date_diff_days(const Date *d1, const Date *d2, int64_t *result);
int date_diff_days_excl(const Date *d1, const Date *d2, int64_t *result);
int date_diff_full(const Date *d1, const Date *d2, DateDiff *out);

int date_add_days(const Date *in, int64_t n, Date *out);
int date_add_months(const Date *in, int n, Date *out);
int date_add_years(const Date *in, int n, Date *out);

int date_day_of_week(const Date *d);
int date_day_of_year(const Date *d);
int date_week_number_iso(const Date *d);

int date_compare(const Date *d1, const Date *d2);
int date_equals(const Date *d1, const Date *d2);

int date_now(Date *out);
int date_now_utc(Date *out);
int datetime_now(DateTime *out);
int datetime_now_utc(DateTime *out);
int datetime_now_tz(DateTime *out, int offset_minutes);

int date_format(const Date *d, const char *fmt, char *buf, size_t bufsize);
int datetime_format(const DateTime *dt, const char *fmt, char *buf, size_t bufsize);

int date_to_epoch_utc(const Date *d, int64_t *epoch_secs);
int date_from_epoch_utc(int64_t epoch_secs, Date *out);
int date_to_epoch_local(const Date *d, int64_t *epoch_secs);
int date_from_epoch_local(int64_t epoch_secs, Date *out);

#ifdef __cplusplus
}
#endif
#endif
```
## ./src/error.c

```c
#include "./include/libcdaydiff.h"

const char *date_error_string(DateError err) {
  switch (err) {
  case DATE_OK:
    return "Success";
  case DATE_ERR_NULL_POINTER:
    return "Null pointer";
  case DATE_ERR_PARSE:
    return "Parse error";
  case DATE_ERR_INVALID_DATE:
    return "Invalid date";
  case DATE_ERR_OVERFLOW:
    return "Overflow";
  case DATE_ERR_OUT_OF_RANGE:
    return "Out of range";
  case DATE_ERR_TIMEZONE:
    return "Timezone error";
  case DATE_ERR_FORMAT:
    return "Format error";
  default:
    return "Unknown error";
  }
}
```
## ./src/parser.c

```c
#include "./internal/libcdaydiff_internal.h"
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

static int parse_date_parts(const char *s, const char *delims, int *d, int *m,
                            int *y) {
  if (!s || !d || !m || !y)
    return DATE_ERR_NULL_POINTER;

  char delim_set[256] = {0};
  if (delims) {
    while (*delims) {
      delim_set[(unsigned char)*delims] = 1;
      delims++;
    }
  } else {
    const char *p = s;
    char first_delim = 0;
    bool inconsistent = false;
    while (*p) {
      if (!isdigit((unsigned char)*p)) {
        if (first_delim == 0) {
          first_delim = *p;
        } else if (*p != first_delim) {
          inconsistent = true;
          break;
        }
      }
      p++;
    }
    if (inconsistent || first_delim == 0) {
      return DATE_ERR_PARSE;
    }
    delim_set[(unsigned char)first_delim] = 1;
  }

  int parts[3] = {0};
  int part_index = 0;
  int current = 0;
  bool has_digit = false;

  const char *ptr = s;
  while (*ptr && part_index < 3) {
    if (isdigit((unsigned char)*ptr)) {
      current = current * 10 + (*ptr - '0');
      has_digit = true;
      ptr++;
    } else if (delim_set[(unsigned char)*ptr]) {
      if (!has_digit) {
        return DATE_ERR_PARSE;
      }
      parts[part_index++] = current;
      current = 0;
      has_digit = false;
      ptr++;
      if (*ptr && delim_set[(unsigned char)*ptr]) {
        return DATE_ERR_PARSE;
      }
    } else {
      return DATE_ERR_PARSE;
    }
  }
  if (has_digit && part_index < 3) {
    parts[part_index++] = current;
  }
  if (part_index != 3)
    return DATE_ERR_PARSE;

  *d = parts[0];
  *m = parts[1];
  *y = parts[2];
  return DATE_OK;
}

int date_parse(const char *str, const char *delims, Date *out) {
  if (!str || !out)
    return DATE_ERR_NULL_POINTER;
  int d, m, y;
  int rc = parse_date_parts(str, delims, &d, &m, &y);
  if (rc != DATE_OK)
    return rc;
  if (!date_validate(d, m, y))
    return DATE_ERR_INVALID_DATE;
  out->day = d;
  out->month = m;
  out->year = y;
  return DATE_OK;
}
```
## ./src/core.c

```c
#include "./internal/libcdaydiff_internal.h"
#include <stddef.h>

static const int dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int date_is_leap_year(int year) {
  if (year < 1)
    return 0;
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int date_days_in_month(int month, int year) {
  if (month < 1 || month > 12)
    return 0;
  if (month == 2 && date_is_leap_year(year))
    return 29;
  return dim[month - 1];
}

int date_validate(int day, int month, int year) {
  if (year < 1900 || year > 9999)
    return 0;
  if (month < 1 || month > 12)
    return 0;
  if (day < 1 || day > date_days_in_month(month, year))
    return 0;
  return 1;
}

int _days_since_epoch(const Date *d, int64_t *days) {
  if (!d || !days)
    return DATE_ERR_NULL_POINTER;
  if (!date_validate(d->day, d->month, d->year))
    return DATE_ERR_INVALID_DATE;

  int64_t total = 0;
  int y = 1900;
  while (y < d->year) {
    total += date_is_leap_year(y) ? 366 : 365;
    y++;
  }
  for (int m = 1; m < d->month; m++) {
    total += date_days_in_month(m, d->year);
  }
  total += (int64_t)d->day - 1;
  *days = total;
  return DATE_OK;
}

int _date_from_days_since_epoch(int64_t days, Date *out) {
  if (!out)
    return DATE_ERR_NULL_POINTER;
  if (days < 0)
    return DATE_ERR_OUT_OF_RANGE;

  int y = 1900;
  while (1) {
    int dim_y = date_is_leap_year(y) ? 366 : 365;
    if (days < dim_y)
      break;
    days -= dim_y;
    y++;
    if (y > 9999)
      return DATE_ERR_OVERFLOW;
  }
  int m = 1;
  while (1) {
    int dim_m = date_days_in_month(m, y);
    if (days < dim_m)
      break;
    days -= dim_m;
    m++;
    if (m > 12)
      return DATE_ERR_OVERFLOW;
  }
  out->year = y;
  out->month = m;
  out->day = (int)days + 1;
  return DATE_OK;
}

int _int64_add_overflow(int64_t a, int64_t b, int64_t *res) {
  if (res == NULL)
    return DATE_ERR_NULL_POINTER;
  if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b))
    return DATE_ERR_OVERFLOW;
  *res = a + b;
  return DATE_OK;
}
```
## ./src/internal/libcdaydiff_internal.h

```c
#ifndef LIBCDAYDIFF_INTERNAL_H
#define LIBCDAYDIFF_INTERNAL_H

#include "../include/libcdaydiff.h"

#ifdef _WIN32
  #define GMTIME_R(tp, tm)   gmtime_s(tm, tp)
  #define LOCALTIME_R(tp, tm) localtime_s(tm, tp)
#else
  #define GMTIME_R(tp, tm)   gmtime_r(tp, tm)
  #define LOCALTIME_R(tp, tm) localtime_r(tp, tm)
#endif

int _days_since_epoch(const Date *d, int64_t *days);
int _date_from_days_since_epoch(int64_t days, Date *out);
int _int64_add_overflow(int64_t a, int64_t b, int64_t *res);
int _get_local_timezone_offset_minutes(void);

#endif
```
## ./src/diff.c

```c
#include "./internal/libcdaydiff_internal.h"
#include <stdlib.h>

int date_diff_days(const Date *d1, const Date *d2, int64_t *result) {
  if (!d1 || !d2 || !result)
    return DATE_ERR_NULL_POINTER;
  int64_t e1, e2;
  int rc;
  rc = _days_since_epoch(d1, &e1);
  if (rc != DATE_OK)
    return rc;
  rc = _days_since_epoch(d2, &e2);
  if (rc != DATE_OK)
    return rc;
  *result = e2 - e1 + 1;
  return DATE_OK;
}

int date_diff_days_excl(const Date *d1, const Date *d2, int64_t *result) {
  if (!d1 || !d2 || !result)
    return DATE_ERR_NULL_POINTER;
  int64_t e1, e2;
  int rc;
  rc = _days_since_epoch(d1, &e1);
  if (rc != DATE_OK)
    return rc;
  rc = _days_since_epoch(d2, &e2);
  if (rc != DATE_OK)
    return rc;
  *result = e2 - e1;
  return DATE_OK;
}

int date_diff_full(const Date *d1, const Date *d2, DateDiff *out) {
  if (!d1 || !d2 || !out)
    return DATE_ERR_NULL_POINTER;
  int cmp = date_compare(d1, d2);
  if (cmp > 0)
    return DATE_ERR_INVALID_DATE;

  Date temp = *d1;
  int years = 0;
  int months = 0;
  int rc;

  while (1) {
    Date next;
    rc = date_add_years(&temp, 1, &next);
    if (rc != DATE_OK)
      return rc;
    if (date_compare(&next, d2) > 0)
      break;
    temp = next;
    years++;
  }

  while (1) {
    Date next;
    rc = date_add_months(&temp, 1, &next);
    if (rc != DATE_OK)
      return rc;
    if (date_compare(&next, d2) > 0)
      break;
    temp = next;
    months++;
  }

  int64_t days_diff;
  rc = date_diff_days_excl(&temp, d2, &days_diff);
  if (rc != DATE_OK)
    return rc;

  out->years = years;
  out->months = months;
  out->days = (int)days_diff;
  return DATE_OK;
}
```
## ./src/manip.c

```c
#include "./internal/libcdaydiff_internal.h"
#include <limits.h>

static int normalize_day(int *day, int month, int year) {
  int max = date_days_in_month(month, year);
  if (max == 0)
    return DATE_ERR_INVALID_DATE;
  if (*day > max)
    *day = max;
  return DATE_OK;
}

int date_add_days(const Date *in, int64_t n, Date *out) {
  if (!in || !out)
    return DATE_ERR_NULL_POINTER;
  if (!date_validate(in->day, in->month, in->year))
    return DATE_ERR_INVALID_DATE;

  int64_t epoch;
  int rc = _days_since_epoch(in, &epoch);
  if (rc != DATE_OK)
    return rc;
  int64_t new_epoch;
  rc = _int64_add_overflow(epoch, n, &new_epoch);
  if (rc != DATE_OK)
    return rc;
  if (new_epoch < 0)
    return DATE_ERR_OVERFLOW;
  rc = _date_from_days_since_epoch(new_epoch, out);
  return rc;
}

int date_add_months(const Date *in, int n, Date *out) {
  if (!in || !out)
    return DATE_ERR_NULL_POINTER;
  if (!date_validate(in->day, in->month, in->year))
    return DATE_ERR_INVALID_DATE;

  int64_t total_months = (int64_t)in->year * 12 + (in->month - 1) + n;
  if (total_months < 1900 * 12 || total_months > 9999 * 12 + 11)
    return DATE_ERR_OVERFLOW;

  int new_year = (int)(total_months / 12);
  int new_month = (int)(total_months % 12) + 1;

  int new_day = in->day;
  int rc = normalize_day(&new_day, new_month, new_year);
  if (rc != DATE_OK)
    return rc;

  out->day = new_day;
  out->month = new_month;
  out->year = new_year;
  return DATE_OK;
}

int date_add_years(const Date *in, int n, Date *out) {
  if (!in || !out)
    return DATE_ERR_NULL_POINTER;
  if (!date_validate(in->day, in->month, in->year))
    return DATE_ERR_INVALID_DATE;

  int64_t y = (int64_t)in->year + n;
  if (y < 1900 || y > 9999)
    return DATE_ERR_OVERFLOW;
  int new_year = (int)y;
  int new_day = in->day;
  int rc = normalize_day(&new_day, in->month, new_year);
  if (rc != DATE_OK)
    return rc;

  out->day = new_day;
  out->month = in->month;
  out->year = new_year;
  return DATE_OK;
}
```
## ./src/fmt.c

```c
#include "./internal/libcdaydiff_internal.h"
#include <stdio.h>
#include <string.h>

static const char *const MONTHS_FULL[] = {
    "January", "February", "March",     "April",   "May",      "June",
    "July",    "August",   "September", "October", "November", "December"};
static const char *const MONTHS_ABBR[] = {"Jan", "Feb", "Mar", "Apr",
                                          "May", "Jun", "Jul", "Aug",
                                          "Sep", "Oct", "Nov", "Dec"};
static const char *const WEEKDAYS[] = {"Sunday",    "Monday",   "Tuesday",
                                       "Wednesday", "Thursday", "Friday",
                                       "Saturday"};
static const char *const WEEKDAYS_ABBR[] = {"Sun", "Mon", "Tue", "Wed",
                                            "Thu", "Fri", "Sat"};

static int append_str(char *buf, size_t bufsize, size_t *pos, const char *s) {
  size_t len = strlen(s);
  if (*pos + len + 1 > bufsize)
    return DATE_ERR_FORMAT;
  strcpy(buf + *pos, s);
  *pos += len;
  return DATE_OK;
}

static int append_int(char *buf, size_t bufsize, size_t *pos, int val,
                      int min_digits) {
  char tmp[32];
  int ret = snprintf(tmp, sizeof(tmp), "%0*d", min_digits, val);
  if (ret < 0)
    return DATE_ERR_FORMAT;
  return append_str(buf, bufsize, pos, tmp);
}

int date_format(const Date *d, const char *fmt, char *buf, size_t bufsize) {
  if (!d || !fmt || !buf)
    return DATE_ERR_NULL_POINTER;
  if (bufsize == 0)
    return DATE_ERR_FORMAT;
  size_t pos = 0;
  buf[0] = '\0';

  while (*fmt) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt) {
      case 'Y':
        if (append_int(buf, bufsize, &pos, d->year, 4) != DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case 'm':
        if (append_int(buf, bufsize, &pos, d->month, 2) != DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case 'd':
        if (append_int(buf, bufsize, &pos, d->day, 2) != DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case 'B':
        if (append_str(buf, bufsize, &pos, MONTHS_FULL[d->month - 1]) !=
            DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case 'b':
        if (append_str(buf, bufsize, &pos, MONTHS_ABBR[d->month - 1]) !=
            DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case 'A':
        if (append_str(buf, bufsize, &pos, WEEKDAYS[date_day_of_week(d)]) !=
            DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case 'a':
        if (append_str(buf, bufsize, &pos,
                       WEEKDAYS_ABBR[date_day_of_week(d)]) != DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case '%':
        if (append_str(buf, bufsize, &pos, "%") != DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      default:
        return DATE_ERR_FORMAT;
      }
      fmt++;
    } else {
      char ch[2] = {*fmt, '\0'};
      if (append_str(buf, bufsize, &pos, ch) != DATE_OK)
        return DATE_ERR_FORMAT;
      fmt++;
    }
  }
  if (pos >= bufsize)
    return DATE_ERR_FORMAT;
  buf[pos] = '\0';
  return DATE_OK;
}

int datetime_format(const DateTime *dt, const char *fmt, char *buf,
                    size_t bufsize) {
  if (!dt || !fmt || !buf)
    return DATE_ERR_NULL_POINTER;
  if (bufsize == 0)
    return DATE_ERR_FORMAT;
  size_t pos = 0;
  buf[0] = '\0';

  while (*fmt) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt) {
      case 'Y':
        if (append_int(buf, bufsize, &pos, dt->date.year, 4) != DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case 'm':
        if (append_int(buf, bufsize, &pos, dt->date.month, 2) != DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case 'd':
        if (append_int(buf, bufsize, &pos, dt->date.day, 2) != DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case 'B':
        if (append_str(buf, bufsize, &pos, MONTHS_FULL[dt->date.month - 1]) !=
            DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case 'b':
        if (append_str(buf, bufsize, &pos, MONTHS_ABBR[dt->date.month - 1]) !=
            DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case 'A':
        if (append_str(buf, bufsize, &pos,
                       WEEKDAYS[date_day_of_week(&dt->date)]) != DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case 'a':
        if (append_str(buf, bufsize, &pos,
                       WEEKDAYS_ABBR[date_day_of_week(&dt->date)]) != DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case 'H':
        if (append_int(buf, bufsize, &pos, dt->hour, 2) != DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case 'M':
        if (append_int(buf, bufsize, &pos, dt->minute, 2) != DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case 'S':
        if (append_int(buf, bufsize, &pos, dt->second, 2) != DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      case 'z': {
        int off = dt->tz_offset;
        char sign = (off >= 0) ? '+' : '-';
        if (off < 0)
          off = -off;
        int h = off / 60, m = off % 60;
        char tz[12];
        int ret = snprintf(tz, sizeof(tz), "%c%02d%02d", sign, h, m);
        if (ret < 0 || (size_t)ret >= sizeof(tz))
          return DATE_ERR_FORMAT;
        if (append_str(buf, bufsize, &pos, tz) != DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      }
      case '%':
        if (append_str(buf, bufsize, &pos, "%") != DATE_OK)
          return DATE_ERR_FORMAT;
        break;
      default:
        return DATE_ERR_FORMAT;
      }
      fmt++;
    } else {
      char ch[2] = {*fmt, '\0'};
      if (append_str(buf, bufsize, &pos, ch) != DATE_OK)
        return DATE_ERR_FORMAT;
      fmt++;
    }
  }
  if (pos >= bufsize)
    return DATE_ERR_FORMAT;
  buf[pos] = '\0';
  return DATE_OK;
}
```
## ./src/tz.c

```c
#include "./internal/libcdaydiff_internal.h"
#include <stdlib.h>
#include <time.h>

static int compute_local_offset(void) {
  time_t t = time(NULL);
  if (t == (time_t)-1)
    return 0;

  struct tm utc, local;
  GMTIME_R(&t, &utc);
  LOCALTIME_R(&t, &local);
  int offset_min =
      (local.tm_hour - utc.tm_hour) * 60 + (local.tm_min - utc.tm_min);
  int day_diff = local.tm_yday - utc.tm_yday;
  if (day_diff < -1)
    day_diff = 1;
  offset_min += day_diff * 1440;
  return offset_min;
}

int date_now_utc(Date *out) {
  if (!out)
    return DATE_ERR_NULL_POINTER;
  time_t t = time(NULL);
  struct tm tm;
  GMTIME_R(&t, &tm);
  out->day = tm.tm_mday;
  out->month = tm.tm_mon + 1;
  out->year = tm.tm_year + 1900;
  return DATE_OK;
}

int date_now(Date *out) {
  if (!out)
    return DATE_ERR_NULL_POINTER;
  time_t t = time(NULL);
  struct tm tm;
  LOCALTIME_R(&t, &tm);
  out->day = tm.tm_mday;
  out->month = tm.tm_mon + 1;
  out->year = tm.tm_year + 1900;
  return DATE_OK;
}

int datetime_now_utc(DateTime *out) {
  if (!out)
    return DATE_ERR_NULL_POINTER;
  time_t t = time(NULL);
  struct tm tm;
  GMTIME_R(&t, &tm);
  out->date.day = tm.tm_mday;
  out->date.month = tm.tm_mon + 1;
  out->date.year = tm.tm_year + 1900;
  out->hour = tm.tm_hour;
  out->minute = tm.tm_min;
  out->second = tm.tm_sec;
  out->tz_offset = 0;
  return DATE_OK;
}

int datetime_now(DateTime *out) {
  if (!out)
    return DATE_ERR_NULL_POINTER;
  time_t t = time(NULL);
  struct tm tm;
  LOCALTIME_R(&t, &tm);
  out->date.day = tm.tm_mday;
  out->date.month = tm.tm_mon + 1;
  out->date.year = tm.tm_year + 1900;
  out->hour = tm.tm_hour;
  out->minute = tm.tm_min;
  out->second = tm.tm_sec;
  out->tz_offset = compute_local_offset();
  return DATE_OK;
}

int datetime_now_tz(DateTime *out, int offset_minutes) {
  if (!out)
    return DATE_ERR_NULL_POINTER;
  time_t t = time(NULL);
  int64_t epoch_secs = (int64_t)t + (int64_t)offset_minutes * 60;
  time_t adj = (time_t)epoch_secs;
  struct tm tm_adj;
  GMTIME_R(&adj, &tm_adj);
  out->date.day = tm_adj.tm_mday;
  out->date.month = tm_adj.tm_mon + 1;
  out->date.year = tm_adj.tm_year + 1900;
  out->hour = tm_adj.tm_hour;
  out->minute = tm_adj.tm_min;
  out->second = tm_adj.tm_sec;
  out->tz_offset = offset_minutes;
  return DATE_OK;
}
```
## ./src/info.c

```c
#include "./internal/libcdaydiff_internal.h"

int date_day_of_week(const Date *d) {
  if (!date_validate(d->day, d->month, d->year))
    return -1;
  int y = d->year;
  int m = d->month;
  if (m < 3) {
    m += 12;
    y--;
  }
  int k = y % 100;
  int j = y / 100;
  int h = (d->day + 13 * (m + 1) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
  return (h + 6) % 7;
}

int date_day_of_year(const Date *d) {
  if (!date_validate(d->day, d->month, d->year))
    return 0;
  int doy = 0;
  for (int m = 1; m < d->month; m++)
    doy += date_days_in_month(m, d->year);
  doy += d->day;
  return doy;
}

int date_week_number_iso(const Date *d) {
  if (!date_validate(d->day, d->month, d->year))
    return 0;

  int weekday = date_day_of_week(d);
  int iso_weekday = (weekday == 0) ? 7 : weekday;

  Date thursday;
  int rc = date_add_days(d, 4 - iso_weekday, &thursday);
  if (rc != DATE_OK)
    return 0;

  int iso_year = thursday.year;

  Date jan4 = {4, 1, iso_year};
  int jan4_weekday = date_day_of_week(&jan4);
  int jan4_iso = (jan4_weekday == 0) ? 7 : jan4_weekday;

  int64_t diff;
  rc = date_diff_days_excl(&jan4, &thursday, &diff);
  if (rc != DATE_OK)
    return 0;

  int week = (int)((diff + jan4_iso - 1) / 7) + 1;

  return week;
}
```
## ./src/comparison.c

```c
#include "./internal/libcdaydiff_internal.h"

int date_compare(const Date *d1, const Date *d2) {
    if (d1->year != d2->year) return (d1->year < d2->year) ? -1 : 1;
    if (d1->month != d2->month) return (d1->month < d2->month) ? -1 : 1;
    if (d1->day != d2->day) return (d1->day < d2->day) ? -1 : 1;
    return 0;
}

int date_equals(const Date *d1, const Date *d2) {
    return date_compare(d1, d2) == 0;
}
```
## ./src/epoch.c

```c
#include "./internal/libcdaydiff_internal.h"
#include <time.h>

int date_to_epoch_utc(const Date *d, int64_t *epoch_secs) {
  if (!d || !epoch_secs)
    return DATE_ERR_NULL_POINTER;
  int64_t days;
  int rc = _days_since_epoch(d, &days);
  if (rc != DATE_OK)
    return rc;
  int64_t unix_days = days - 25567;
  *epoch_secs = unix_days * 86400;
  return DATE_OK;
}

int date_from_epoch_utc(int64_t epoch_secs, Date *out) {
  if (!out)
    return DATE_ERR_NULL_POINTER;
  if (epoch_secs < 0)
    return DATE_ERR_OUT_OF_RANGE;
  int64_t unix_days = epoch_secs / 86400;
  int64_t days_since_1900 = unix_days + 25567;
  return _date_from_days_since_epoch(days_since_1900, out);
}

int date_to_epoch_local(const Date *d, int64_t *epoch_secs) {
  if (!d || !epoch_secs)
    return DATE_ERR_NULL_POINTER;
  int offset = _get_local_timezone_offset_minutes();
  int rc = date_to_epoch_utc(d, epoch_secs);
  if (rc != DATE_OK)
    return rc;
  *epoch_secs -= (int64_t)offset * 60;
  return DATE_OK;
}

int date_from_epoch_local(int64_t epoch_secs, Date *out) {
  if (!out)
    return DATE_ERR_NULL_POINTER;
  int offset = _get_local_timezone_offset_minutes();
  int64_t utc_secs = epoch_secs + (int64_t)offset * 60;
  return date_from_epoch_utc(utc_secs, out);
}

int _get_local_timezone_offset_minutes(void) {
  time_t t = time(NULL);
  struct tm utc, local;
  GMTIME_R(&t, &utc);
  LOCALTIME_R(&t, &local);

  int off = (local.tm_hour - utc.tm_hour) * 60 + (local.tm_min - utc.tm_min);
  int day_diff = local.tm_yday - utc.tm_yday;
  if (day_diff < -1)
    day_diff = 1;
  off += day_diff * 1440;
  return off;
}
```
## ./test.c

```c
#include <libcdaydiff.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name)                                                             \
  do {                                                                         \
    printf("  TEST: %-50s ", name);                                            \
  } while (0)
#define CHECK(cond)                                                            \
  do {                                                                         \
    if (cond) {                                                                \
      printf("PASS\n");                                                        \
      tests_passed++;                                                          \
    } else {                                                                   \
      printf("FAIL (%s:%d)\n", __FILE__, __LINE__);                            \
      tests_failed++;                                                          \
    }                                                                          \
  } while (0)

/* ================================================================
 * 1. Parsing
 * ================================================================ */
static void test_parsing(void) {
  printf("\n=== Parsing ===\n");
  Date d;

  TEST("parse '06.05.2026' (auto-detect)");
  {
    int rc = date_parse("06.05.2026", NULL, &d);
    CHECK(rc == DATE_OK && d.day == 6 && d.month == 5 && d.year == 2026);
  }

  TEST("parse '08/12/2026' (auto-detect)");
  {
    int rc = date_parse("08/12/2026", NULL, &d);
    CHECK(rc == DATE_OK && d.day == 8 && d.month == 12 && d.year == 2026);
  }

  TEST("parse '31-01-2024' (delim '-')");
  {
    int rc = date_parse("31-01-2024", "-", &d);
    CHECK(rc == DATE_OK && d.day == 31 && d.month == 1 && d.year == 2024);
  }

  TEST("parse '15 08 1999' (delim ' ')");
  {
    int rc = date_parse("15 08 1999", " ", &d);
    CHECK(rc == DATE_OK && d.day == 15 && d.month == 8 && d.year == 1999);
  }

  TEST("parse invalid '32.01.2020' should fail");
  {
    int rc = date_parse("32.01.2020", ".", &d);
    CHECK(rc == DATE_ERR_INVALID_DATE);
  }

  TEST("parse invalid '29.02.2023' (not leap year) should fail");
  {
    int rc = date_parse("29.02.2023", ".", &d);
    CHECK(rc == DATE_ERR_INVALID_DATE);
  }

  TEST("parse '29.02.2024' (leap year) should succeed");
  {
    int rc = date_parse("29.02.2024", NULL, &d);
    CHECK(rc == DATE_OK && d.day == 29 && d.month == 2 && d.year == 2024);
  }

  TEST("parse with mixed delimiters should fail");
  {
    int rc = date_parse("01/02-2020", NULL, &d);
    CHECK(rc == DATE_ERR_PARSE);
  }

  TEST("parse empty string should fail");
  {
    int rc = date_parse("", NULL, &d);
    CHECK(rc != DATE_OK);
  }

  TEST("parse NULL string should fail");
  {
    int rc = date_parse(NULL, NULL, &d);
    CHECK(rc == DATE_ERR_NULL_POINTER);
  }
}

/* ================================================================
 * 2. Core validation
 * ================================================================ */
static void test_core(void) {
  printf("\n=== Core Functions ===\n");

  TEST("leap year 2000");
  CHECK(date_is_leap_year(2000) == 1);
  TEST("leap year 1900 (not)");
  CHECK(date_is_leap_year(1900) == 0);
  TEST("leap year 2024");
  CHECK(date_is_leap_year(2024) == 1);

  TEST("days in month Jan 2023");
  CHECK(date_days_in_month(1, 2023) == 31);
  TEST("days in month Feb 2023");
  CHECK(date_days_in_month(2, 2023) == 28);
  TEST("days in month Feb 2024");
  CHECK(date_days_in_month(2, 2024) == 29);

  TEST("validate 31/12/2022");
  CHECK(date_validate(31, 12, 2022) == 1);
  TEST("validate 0/1/2020 invalid");
  CHECK(date_validate(0, 1, 2020) == 0);
  TEST("validate 29/2/2023 invalid");
  CHECK(date_validate(29, 2, 2023) == 0);
  TEST("validate 29/2/2024 valid");
  CHECK(date_validate(29, 2, 2024) == 1);
  TEST("validate year 1899 out of range");
  CHECK(date_validate(1, 1, 1899) == 0);
  TEST("validate year 10000 out of range");
  CHECK(date_validate(1, 1, 10000) == 0);
}

/* ================================================================
 * 3. Difference (days)
 * ================================================================ */
static void test_diff_days(void) {
  printf("\n=== Day Differences ===\n");
  Date d1, d2;
  int64_t diff;

  date_parse("06.05.2026", ".", &d1);
  date_parse("08.12.2026", ".", &d2);

  TEST("inclusive diff 06.05.2026 -> 08.12.2026");
  {
    int rc = date_diff_days(&d1, &d2, &diff);
    /* Mei 6 to Mei 31 = 26 days? Let's compute manually:
       6 Mei to 31 Mei inclusive = (31-6+1)=26 hari,
       Juni 30, Juli 31, Agustus 31, Sep 30, Okt 31, Nov 30, Des 8 = 8,
       Total: 26+30+31+31+30+31+30+8 = 217? Perlu dicek. */
    CHECK(rc == DATE_OK &&
          diff == 217); // Saya hitung: (31-6+1)=26, lalu 30+31+31+30+31+30+8 =
                        // 191? Total 26+191=217.
  }

  TEST("exclusive diff 06.05.2026 -> 08.12.2026");
  {
    int rc = date_diff_days_excl(&d1, &d2, &diff);
    CHECK(rc == DATE_OK && diff == 216);
  }

  TEST("same date inclusive diff = 1");
  {
    Date same = {6, 5, 2026};
    int rc = date_diff_days(&same, &same, &diff);
    CHECK(rc == DATE_OK && diff == 1);
  }

  TEST("same date exclusive diff = 0");
  {
    Date same = {6, 5, 2026};
    int rc = date_diff_days_excl(&same, &same, &diff);
    CHECK(rc == DATE_OK && diff == 0);
  }

  TEST("NULL pointers return error");
  {
    int rc = date_diff_days(NULL, &d1, &diff);
    CHECK(rc == DATE_ERR_NULL_POINTER);
  }
}

/* ================================================================
 * 4. Full difference (years, months, days)
 * ================================================================ */
static void test_diff_full(void) {
  printf("\n=== Full Difference ===\n");
  Date d1, d2;
  DateDiff out;

  date_parse("01.01.2020", ".", &d1);
  date_parse("01.01.2021", ".", &d2);

  TEST("exactly 1 year");
  {
    int rc = date_diff_full(&d1, &d2, &out);
    CHECK(rc == DATE_OK && out.years == 1 && out.months == 0 && out.days == 0);
  }

  date_parse("15.03.2020", ".", &d1);
  date_parse("15.06.2021", ".", &d2);

  TEST("1 year 3 months");
  {
    int rc = date_diff_full(&d1, &d2, &out);
    CHECK(rc == DATE_OK && out.years == 1 && out.months == 3 && out.days == 0);
  }

  date_parse("31.01.2020", ".", &d1);
  date_parse("29.02.2020", ".", &d2); // leap year

  TEST("31 Jan -> 29 Feb in leap year");
  {
    int rc = date_diff_full(&d1, &d2, &out);
    CHECK(rc == DATE_OK && out.years == 0 && out.months == 1 &&
          out.days == 0); // karena 31 Jan -> 29 Feb, setelah add 1 bulan
                          // menjadi 29 Feb (pin end)
  }

  date_parse("01.01.2020", ".", &d1);
  date_parse("31.12.2020", ".", &d2);

  TEST("full year 2020");
  {
    int rc = date_diff_full(&d1, &d2, &out);
    CHECK(rc == DATE_OK && out.years == 0 && out.months == 11 &&
          out.days == 30);
  }

  TEST("reversed dates returns error");
  {
    int rc = date_diff_full(&d2, &d1, &out);
    CHECK(rc == DATE_ERR_INVALID_DATE);
  }
}

/* ================================================================
 * 5. Manipulation
 * ================================================================ */
static void test_manip(void) {
  printf("\n=== Manipulation ===\n");
  Date in, out;
  date_parse("06.05.2026", ".", &in);

  TEST("add 10 days");
  {
    int rc = date_add_days(&in, 10, &out);
    CHECK(rc == DATE_OK && out.day == 16 && out.month == 5 && out.year == 2026);
  }

  TEST("add 30 days crossing month");
  {
    Date start = {1, 5, 2026};
    int rc = date_add_days(&start, 30, &out);
    CHECK(rc == DATE_OK && out.day == 31 && out.month == 5 && out.year == 2026);
  }

  TEST("add 1 month to 31 Jan (non-leap)");
  {
    Date start = {31, 1, 2023};
    int rc = date_add_months(&start, 1, &out);
    CHECK(rc == DATE_OK && out.day == 28 && out.month == 2 && out.year == 2023);
  }

  TEST("add 1 month to 31 Jan (leap)");
  {
    Date start = {31, 1, 2024};
    int rc = date_add_months(&start, 1, &out);
    CHECK(rc == DATE_OK && out.day == 29 && out.month == 2 && out.year == 2024);
  }

  TEST("add 12 months (1 year)");
  {
    Date start = {15, 6, 2023};
    int rc = date_add_months(&start, 12, &out);
    CHECK(rc == DATE_OK && out.day == 15 && out.month == 6 && out.year == 2024);
  }

  TEST("add years leap day preservation");
  {
    Date start = {29, 2, 2024};
    int rc = date_add_years(&start, 1, &out);
    /* 2025 not leap, should pin to 28 Feb */
    CHECK(rc == DATE_OK && out.day == 28 && out.month == 2 && out.year == 2025);
  }

  TEST("add years overflow boundary");
  {
    Date start = {1, 1, 9999};
    int rc = date_add_years(&start, 1, &out);
    CHECK(rc == DATE_ERR_OVERFLOW);
  }

  TEST("add negative days");
  {
    Date start = {10, 1, 2020};
    int rc = date_add_days(&start, -5, &out);
    CHECK(rc == DATE_OK && out.day == 5 && out.month == 1 && out.year == 2020);
  }
}

/* ================================================================
 * 6. Information
 * ================================================================ */
static void test_info(void) {
  printf("\n=== Information ===\n");
  Date d;

  date_parse("01.01.2023", ".", &d); // Sunday
  TEST("day_of_week 1 Jan 2023 (Sunday)");
  CHECK(date_day_of_week(&d) == 0);

  date_parse("02.01.2023", ".", &d); // Monday
  TEST("day_of_week 2 Jan 2023 (Monday)");
  CHECK(date_day_of_week(&d) == 1);

  date_parse("31.12.2023", ".", &d);
  TEST("day_of_year 31 Dec 2023");
  CHECK(date_day_of_year(&d) == 365);

  date_parse("31.12.2020", ".", &d); // leap year
  TEST("day_of_year 31 Dec 2020 (leap)");
  CHECK(date_day_of_year(&d) == 366);

  // ISO week number tests
  date_parse("01.01.2023", ".", &d);
  TEST("ISO week 1 Jan 2023");
  CHECK(date_week_number_iso(&d) ==
        52); // 2023-01-01 is Sunday, week 52 of 2022

  date_parse("03.01.2023", ".", &d);
  TEST("ISO week 3 Jan 2023");
  CHECK(date_week_number_iso(&d) ==
        1); // first Monday is 2 Jan, week 1 starts 2 Jan

  date_parse("31.12.2023", ".", &d);
  TEST("ISO week 31 Dec 2023");
  CHECK(date_week_number_iso(&d) == 52); // 2023-12-31 is Sunday, week 52
}

/* ================================================================
 * 7. Formatting
 * ================================================================ */
static void test_format(void) {
  printf("\n=== Formatting ===\n");
  Date d;
  DateTime dt;
  char buf[128];

  date_parse("06.05.2026", ".", &d);

  TEST("format %d-%m-%Y");
  {
    date_format(&d, "%d-%m-%Y", buf, sizeof(buf));
    CHECK(strcmp(buf, "06-05-2026") == 0);
  }

  TEST("format %Y/%m/%d");
  {
    date_format(&d, "%Y/%m/%d", buf, sizeof(buf));
    CHECK(strcmp(buf, "2026/05/06") == 0);
  }

  TEST("format %B %d, %Y");
  {
    date_format(&d, "%B %d, %Y", buf, sizeof(buf));
    CHECK(strcmp(buf, "May 06, 2026") == 0);
  }

  TEST("format %a %A");
  {
    date_format(&d, "%a %A", buf, sizeof(buf));
    CHECK(strcmp(buf, "Wed Wednesday") == 0); // 6 May 2026 is Wednesday
  }

  /* DateTime formatting */
  dt.date = d;
  dt.hour = 14;
  dt.minute = 30;
  dt.second = 45;
  dt.tz_offset = 420; // +0700? 420=7*60

  TEST("datetime format %H:%M:%S");
  {
    datetime_format(&dt, "%H:%M:%S", buf, sizeof(buf));
    CHECK(strcmp(buf, "14:30:45") == 0);
  }

  TEST("datetime format %z (timezone)");
  {
    datetime_format(&dt, "%z", buf, sizeof(buf));
    CHECK(strcmp(buf, "+0700") == 0);
  }

  TEST("datetime combined format");
  {
    datetime_format(&dt, "%Y-%m-%dT%H:%M:%S%z", buf, sizeof(buf));
    CHECK(strcmp(buf, "2026-05-06T14:30:45+0700") == 0);
  }

  TEST("format buffer too small returns error");
  {
    int rc = date_format(&d, "%d-%m-%Y", buf, 5); // terlalu kecil
    CHECK(rc == DATE_ERR_FORMAT);
  }
}

/* ================================================================
 * 8. Comparison
 * ================================================================ */
static void test_comparison(void) {
  printf("\n=== Comparison ===\n");
  Date d1 = {1, 1, 2020};
  Date d2 = {1, 1, 2020};
  Date d3 = {2, 1, 2020};

  TEST("equal dates");
  CHECK(date_compare(&d1, &d2) == 0);
  TEST("d1 < d3");
  CHECK(date_compare(&d1, &d3) == -1);
  TEST("d3 > d1");
  CHECK(date_compare(&d3, &d1) == 1);
  TEST("date_equals");
  CHECK(date_equals(&d1, &d2) == 1);
  TEST("date_equals false");
  CHECK(date_equals(&d1, &d3) == 0);
}

/* ================================================================
 * 9. Real-time / Timezone
 * ================================================================ */
static void test_time(void) {
  printf("\n=== Real-time ===\n");
  Date d;
  DateTime dt;

  TEST("date_now returns valid");
  {
    int rc = date_now(&d);
    CHECK(rc == DATE_OK && date_validate(d.day, d.month, d.year));
  }

  TEST("date_now_utc returns valid");
  {
    int rc = date_now_utc(&d);
    CHECK(rc == DATE_OK && date_validate(d.day, d.month, d.year));
  }

  TEST("datetime_now returns valid");
  {
    int rc = datetime_now(&dt);
    CHECK(rc == DATE_OK && dt.hour < 24 && dt.minute < 60 && dt.second < 60);
  }

  TEST("datetime_now_utc offset 0");
  {
    int rc = datetime_now_utc(&dt);
    CHECK(rc == DATE_OK && dt.tz_offset == 0);
  }

  TEST("datetime_now_tz +420");
  {
    int rc = datetime_now_tz(&dt, 420);
    CHECK(rc == DATE_OK && dt.tz_offset == 420);
  }

  TEST("NULL pointer handling");
  {
    CHECK(date_now(NULL) == DATE_ERR_NULL_POINTER);
  }
}

/* ================================================================
 * 10. Epoch conversions
 * ================================================================ */
static void test_epoch(void) {
  printf("\n=== Epoch Conversions ===\n");
  Date d, back;
  int64_t secs;

  date_parse("01.01.2020", ".", &d);

  TEST("date_to_epoch_utc");
  {
    int rc = date_to_epoch_utc(&d, &secs);
    CHECK(rc == DATE_OK && secs > 0);
  }

  TEST("roundtrip UTC epoch");
  {
    int rc = date_to_epoch_utc(&d, &secs);
    if (rc == DATE_OK) {
      rc = date_from_epoch_utc(secs, &back);
      CHECK(rc == DATE_OK && date_compare(&d, &back) == 0);
    } else
      CHECK(0);
  }

  TEST("date_to_epoch_local (at least returns OK)");
  {
    int rc = date_to_epoch_local(&d, &secs);
    CHECK(rc == DATE_OK);
  }

  TEST("roundtrip local epoch");
  {
    int rc = date_to_epoch_local(&d, &secs);
    if (rc == DATE_OK) {
      rc = date_from_epoch_local(secs, &back);
      CHECK(rc == DATE_OK && date_compare(&d, &back) == 0);
    } else
      CHECK(0);
  }

  TEST("negative epoch returns error");
  {
    int rc = date_from_epoch_utc(-100000, &back);
    CHECK(rc == DATE_ERR_OUT_OF_RANGE);
  }
}

/* ================================================================
 * 11. Error string
 * ================================================================ */
static void test_error(void) {
  printf("\n=== Error Strings ===\n");
  TEST("DATE_OK string");
  CHECK(strcmp(date_error_string(DATE_OK), "Success") == 0);
  TEST("DATE_ERR_PARSE string");
  CHECK(strcmp(date_error_string(DATE_ERR_PARSE), "Parse error") == 0);
}

/* ================================================================
 * Main
 * ================================================================ */
int main(void) {
  test_parsing();
  test_core();
  test_diff_days();
  test_diff_full();
  test_manip();
  test_info();
  test_format();
  test_comparison();
  test_time();
  test_epoch();
  test_error();

  printf("\n========================================\n");
  printf(" Tests passed: %d\n", tests_passed);
  printf(" Tests failed: %d\n", tests_failed);
  printf("========================================\n");

  return tests_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
```
## ./readme

```
================================================================================
LIBCDAYDIFF – Portable C Date & Time Library
================================================================================

Version: 1.1.0
License: GNU General Public License v3.0
Repository: https://github.com/mroczect/libcdaydiff

================================================================================
TABLE OF CONTENTS
================================================================================

1. OVERVIEW
2. FEATURES
3. SYSTEM REQUIREMENTS
4. INSTALLATION
   4.1 Using CMake (recommended)
   4.2 Using GNU Make
   4.3 Manual Compilation
5. QUICK START EXAMPLE
6. API REFERENCE
   6.1 Data Types
   6.2 Error Codes
   6.3 Core Validation Functions
   6.4 Parsing
   6.5 Date Difference
   6.6 Date Manipulation
   6.7 Date Information
   6.8 Comparison
   6.9 Current Time / Timezone
   6.10 Formatting
   6.11 Epoch Conversion
   6.12 Error String
7. FORMAT SPECIFIERS
8. FULL USAGE EXAMPLES
9. THREAD SAFETY
10. FREQUENTLY ASKED QUESTIONS (FAQ)
11. CONTRIBUTING
12. LICENSE
13. CONTACT

================================================================================

1. OVERVIEW
   \================================================================================

libcdaydiff is a pure C (C99) library that provides a complete set of
functions for working with dates and times. It requires no external
dependencies and compiles on any platform with a standard C compiler.

Capabilities include:

- Parsing dates from strings with automatic or user‑defined delimiters
- Validation of dates (including leap year rules)
- Date differences: inclusive / exclusive day count, full year‑month‑day
  breakdown
- Date arithmetic: add/subtract days, months, years (with end‑of‑month pinning)
- Day‑of‑week, day‑of‑year, ISO 8601 week number
- Comparison (equals, before/after)
- String formatting with printf‑like specifiers
- Real‑time clock: local, UTC, and custom timezone offset
- Unix epoch conversion (UTC and local)
- Clear error handling with descriptive strings

The library builds as both a static and a shared library.

================================================================================ 2. FEATURES
================================================================================

- Robust parsing: auto‑detects delimiter (`.`, `/`, `-`, ` `, etc.) or
  accepts a custom delimiter set.
- Full validation: checks day‑of‑month, month range, year range (1900–9999),
  and leap years.
- Inclusive & exclusive day difference with int64_t output.
- Full difference breakdown into years, months, and remaining days.
- Date manipulation: add/subtract days (int64_t), months, years.
  Handles end‑of‑month pinning (e.g., 31 Jan + 1 month → 28/29 Feb).
- Day‑of‑week (0=Sunday), day‑of‑year (1‑366), ISO week number.
- String formatting with specifiers: %Y, %m, %d, %B, %b, %A, %a, %H, %M, %S,
  %z, %%.
- Current time functions: local, UTC, or with explicit offset in minutes.
- Unix epoch conversion: date to seconds and seconds to date (both UTC and
  local).
- Error codes for every function; error descriptions via date_error_string().
- C++ compatible: extern "C" wrapper in header.
- Portable: tested on Linux, macOS, Windows (MSVC ≥ 2013, MinGW‑w64).
- Build system: CMake (primary) and GNU Make (legacy).

================================================================================ 3. SYSTEM REQUIREMENTS
================================================================================

- A C99 compiler (GCC, Clang, MSVC 2013 or later)
- CMake 3.10+ (for CMake build) — optional if using Make
- GNU Make (for legacy Makefile)
- Standard C library and POSIX headers (time.h, string.h, etc.)

================================================================================ 4. INSTALLATION
================================================================================

---

4.1 Using CMake (recommended)
----------------------------------------------------------------------

This method builds both static and shared libraries, runs unit tests,
and installs CMake configuration files for use with find_package().

Step 1: Clone the repository

    git clone https://github.com/mroczect/libcdaydiff.git
    cd libcdaydiff

Step 2: Configure and build

    mkdir build && cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
    cmake --build .

Step 3: Run tests (optional but recommended)

    ctest

    All 73 tests should pass.

Step 4: Install

    sudo cmake --install .

    Installed files:
      - Header: /usr/local/include/libcdaydiff.h
      - Static library: /usr/local/lib/libcdaydiff.a
      - Shared library: /usr/local/lib/libcdaydiff.so (or .dylib on macOS)
      - CMake package: /usr/local/lib/cmake/libcdaydiff/

On Windows (using Visual Studio Developer Command Prompt):

    mkdir build && cd build
    cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_INSTALL_PREFIX=C:\libs\libcdaydiff
    cmake --build . --config Release
    ctest -C Release
    cmake --install . --config Release

On Windows (MinGW‑w64):

    mkdir build && cd build
    cmake .. -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=C:/libs/libcdaydiff
    mingw32-make
    mingw32-make install

---

4.2 Using GNU Make (legacy)
----------------------------------------------------------------------

A basic Makefile is provided for quick builds on Unix‑like systems.
It builds only the static library.

    git clone https://github.com/mroczect/libcdaydiff.git
    cd libcdaydiff
    make

This produces libcdaydiff.a in the project root. Object files are placed
in obj/. To install:

    sudo make install

The library and header are copied to /usr/local.

To override compiler or flags:

    make CC=clang CFLAGS="-O3 -march=native"

---

4.3 Manual Compilation
----------------------------------------------------------------------

If you prefer not to use any build system, you can compile all .c files
directly. Example for a static library:

    gcc -std=c99 -Wall -Wextra -c src/core.c src/parser.c src/diff.c \
        src/manip.c src/fmt.c src/tz.c src/epoch.c src/error.c \
        src/info.c src/comparison.c -Isrc/include -Isrc/internal
    ar rcs libcdaydiff.a *.o

Then link your program with -lcdaydiff.

================================================================================ 5. QUICK START EXAMPLE
================================================================================

Save the following as example.c:

    #include <libcdaydiff.h>
    #include <stdio.h>

    int main() {
        Date d1, d2;
        date_parse("06.05.2026", NULL, &d1);   // auto‑detect delimiter
        date_parse("08.12.2026", NULL, &d2);

        int64_t days;
        date_diff_days(&d1, &d2, &days);        // inclusive count
        printf("Difference: %lld days\n", (long long)days); // 217

        char buf[100];
        date_format(&d1, "%A, %d %B %Y", buf, sizeof(buf));
        printf("Date: %s\n", buf);              // Wednesday, 06 May 2026

        return 0;
    }

Compile and run (if installed system‑wide):

    gcc -std=c99 example.c -lcdaydiff -o example
    ./example

If you built locally without installing, adjust include and library paths:

    gcc -std=c99 -Isrc/include example.c libcdaydiff.a -o example

================================================================================ 6. API REFERENCE
================================================================================

All functions return a value of type "DateError" (an int) unless otherwise
noted. On success they return DATE_OK (0). On error they return one of the
error codes described below.

---

6.1 Data Types
----------------------------------------------------------------------

    typedef struct {
        int day;    // 1‑31
        int month;  // 1‑12
        int year;   // 1900‑9999
    } Date;

    typedef struct {
        Date date;
        int hour;        // 0‑23
        int minute;      // 0‑59
        int second;      // 0‑59
        int tz_offset;   // offset from UTC in minutes (e.g., +420 for WIB)
    } DateTime;

    typedef struct {
        int years;
        int months;
        int days;        // remaining days after years and months
    } DateDiff;

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

---

6.2 Error Codes
----------------------------------------------------------------------

    DATE_OK                (0) – Operation succeeded.
    DATE_ERR_NULL_POINTER      – A required pointer argument was NULL.
    DATE_ERR_PARSE             – Failed to parse the date string.
    DATE_ERR_INVALID_DATE      – The date (day/month/year) is invalid.
    DATE_ERR_OVERFLOW          – Arithmetic overflow or out of supported range.
    DATE_ERR_OUT_OF_RANGE      – Value is outside allowed year range.
    DATE_ERR_TIMEZONE          – Timezone error (currently unused).
    DATE_ERR_FORMAT            – Format string invalid or buffer too small.

---

6.3 Core Validation Functions
----------------------------------------------------------------------

int date_is_leap_year(int year);
Returns 1 if 'year' is a leap year, 0 otherwise.

int date_days_in_month(int month, int year);
Returns the number of days in the given month (1–12) of the given year.
Returns 0 for invalid month.

int date_validate(int day, int month, int year);
Returns 1 if the date is valid (1900 ≤ year ≤ 9999, etc.), else 0.

---

6.4 Parsing
----------------------------------------------------------------------

int date_parse(const char *str, const char *delims, Date *out);

    Parses a date string into a Date structure.

    Parameters:
        str    – input string, expected format: "DD<delim>MM<delim>YYYY"
        delims – a string containing allowed delimiter characters (e.g. "./\\- ").
                 If NULL, the function automatically detects the first non‑digit
                 character as the delimiter (must be consistent throughout).
        out    – pointer to Date that receives the result.

    Returns:
        DATE_OK              – success.
        DATE_ERR_NULL_POINTER – str or out is NULL.
        DATE_ERR_PARSE        – string format invalid (wrong delimiters, too few parts).
        DATE_ERR_INVALID_DATE – date does not exist (e.g., 32/01/2020).

    Note: The order of components is day, month, year. Library uses this
    convention because many real‑world applications prefer it. If you need
    year‑first, you can swap the fields after parsing.

---

6.5 Date Difference
----------------------------------------------------------------------

int date_diff_days(const Date *d1, const Date *d2, int64_t *result);
Inclusive difference. The start date is counted as 1. If d1 == d2,
result = 1. Example: 01.01–01.01 → 1.

int date_diff_days_excl(const Date *d1, const Date *d2, int64_t *result);
Exclusive difference. The start date is not counted. Same day gives 0.

int date_diff_full(const Date *d1, const Date *d2, DateDiff *out);
Computes the difference in years, months, and remaining days. d1 must
be earlier than d2 (or equal). If d1 > d2, returns DATE_ERR_INVALID_DATE.
The algorithm first adds years, then months, then counts remaining days.
Result is always such that d1 + (years, months, days) = d2, with days
less than a month.

---

6.6 Date Manipulation
----------------------------------------------------------------------

int date_add_days(const Date *in, int64_t n, Date *out);
Adds n days to 'in'. Negative n subtracts. Uses internal epoch
calculation, supports large range as long as final year stays within
1900–9999.

int date_add_months(const Date *in, int n, Date *out);
Adds n months. If the resulting day exceeds the month's length, the day
is pinned to the last valid day (e.g., 31 Jan + 1 month → 28/29 Feb).

int date_add_years(const Date *in, int n, Date *out);
Adds n years. For dates like 29 Feb, if the target year is not a leap year,
the day becomes 28 Feb.

All manipulation functions return DATE_ERR_OVERFLOW if the resulting year
falls outside [1900, 9999].

---

6.7 Date Information
----------------------------------------------------------------------

int date_day_of_week(const Date *d);
Returns 0 for Sunday, 1 for Monday, ..., 6 for Saturday.
Uses Zeller's congruence.

int date_day_of_year(const Date *d);
Returns 1–365 (366 in leap years).

int date_week_number_iso(const Date *d);
Returns ISO 8601 week number (1–53). Uses the "Thursday of the week"
method for correctness across year boundaries.

---

6.8 Comparison
----------------------------------------------------------------------

int date_compare(const Date *d1, const Date *d2);
Compares two dates. Returns:
-1 if d1 is earlier than d2,
0 if equal,
1 if d1 is later than d2.

int date_equals(const Date *d1, const Date *d2);
Returns 1 if the dates are exactly equal, 0 otherwise.

---

6.9 Current Time / Timezone
----------------------------------------------------------------------

int date_now(Date *out);
Writes today's date (local time) into out.

int date_now_utc(Date *out);
Writes today's date (UTC) into out.

int datetime_now(DateTime *out);
Writes current local date and time, including timezone offset (minutes).

int datetime_now_utc(DateTime *out);
Writes current UTC date and time, offset set to 0.

int datetime_now_tz(DateTime *out, int offset_minutes);
Writes current UTC date/time adjusted by the given offset.
offset_minutes can be positive (east of UTC) or negative.
The resulting 'out' contains the date/time at that offset.

All time functions return DATE_ERR_NULL_POINTER if out is NULL.
On platforms that lack gmtime_r/localtime_r (older MSVC), the library
uses the equivalent _s functions internally for thread safety.

---

6.10 Formatting
----------------------------------------------------------------------

int date_format(const Date *d, const char *fmt, char *buf, size_t bufsize);
Formats a date into buf according to fmt. See FORMAT SPECIFIERS below.

int datetime_format(const DateTime *dt, const char *fmt, char *buf,
size_t bufsize);
Formats a date‑time into buf. Supports additional time specifiers.

Both functions return:
DATE_OK on success.
DATE_ERR_NULL_POINTER if d (or dt), fmt, or buf is NULL.
DATE_ERR_FORMAT if bufsize is 0, the format string is invalid,
or the output does not fit (buffer too small).

---

6.11 Epoch Conversion
----------------------------------------------------------------------

int date_to_epoch_utc(const Date *d, int64_t *epoch_secs);
Converts a date (assumed 00:00:00 UTC) to Unix seconds since 1970‑01‑01.

int date_from_epoch_utc(int64_t epoch_secs, Date *out);
Converts Unix seconds back to a UTC date. Only non‑negative epoch
values are accepted.

int date_to_epoch_local(const Date *d, int64_t *epoch_secs);
Converts a date (local time 00:00:00) to Unix seconds.

int date_from_epoch_local(int64_t epoch_secs, Date *out);
Converts Unix seconds back to a local date.

These functions use the system's local time offset. On platforms where
the offset cannot be determined, they behave like the UTC versions.

---

6.12 Error String
----------------------------------------------------------------------

const char *date_error_string(DateError err);
Returns a static, human‑readable description of the error code.
Do not free the returned pointer.

================================================================================ 7. FORMAT SPECIFIERS
================================================================================

The following specifiers are supported by date_format() and
datetime_format(). They are case‑sensitive.

    %Y   – year as a 4‑digit number (e.g., 2026)
    %m   – month as a 2‑digit number (01–12)
    %d   – day as a 2‑digit number (01–31)
    %B   – full month name (January, February, ...)
    %b   – abbreviated month name (Jan, Feb, ...)
    %A   – full weekday name (Sunday, Monday, ...)
    %a   – abbreviated weekday name (Sun, Mon, ...)
    %H   – hour as 2‑digit 24‑hour clock (00–23)   [datetime only]
    %M   – minute as 2‑digit (00–59)               [datetime only]
    %S   – second as 2‑digit (00–59)               [datetime only]
    %z   – timezone offset as ±HHMM (e.g., +0700)  [datetime only]
    %%   – a literal '%' character

Unrecognized specifiers cause the function to return DATE_ERR_FORMAT.

Example format strings:
"%Y-%m-%d" → "2026-05-06"
"%d/%m/%Y" → "06/05/2026"
"%A, %B %d, %Y" → "Wednesday, May 06, 2026"
"%Y-%m-%dT%H:%M:%S%z" → "2026-05-06T14:30:45+0700"

================================================================================ 8. FULL USAGE EXAMPLES
================================================================================

Example 1: Countdown to New Year

    #include <libcdaydiff.h>
    #include <stdio.h>

    int main() {
        Date today, new_year = {1, 1, 2027};
        date_now(&today);

        int64_t remaining;
        date_diff_days(&today, &new_year, &remaining);
        printf("Days until 2027: %lld\n", (long long)remaining);

        return 0;
    }

Example 2: Calculate age

    #include <libcdaydiff.h>
    #include <stdio.h>

    int main() {
        Date birth = {15, 8, 1990};
        Date today;
        date_now(&today);

        DateDiff diff;
        date_diff_full(&birth, &today, &diff);
        printf("Age: %d years, %d months, %d days\n",
               diff.years, diff.months, diff.days);

        return 0;
    }

Example 3: Add business days (skipping weekends) – demonstration only

    #include <libcdaydiff.h>
    #include <stdio.h>

    int main() {
        Date d = {1, 7, 2026};  // Wednesday
        int days_to_add = 5;
        int added = 0;

        while (added < days_to_add) {
            date_add_days(&d, 1, &d);
            int dow = date_day_of_week(&d);
            if (dow != 0 && dow != 6)  // skip Sunday(0) and Saturday(6)
                added++;
        }
        char buf[50];
        date_format(&d, "%A, %d %B %Y", buf, sizeof(buf));
        printf("Result: %s\n", buf);
        return 0;
    }

Example 4: Format current time in several timezones

    #include <libcdaydiff.h>
    #include <stdio.h>

    int main() {
        DateTime dt;
        char buf[100];

        // UTC
        datetime_now_utc(&dt);
        datetime_format(&dt, "%Y-%m-%d %H:%M:%S UTC", buf, sizeof(buf));
        printf("%s\n", buf);

        // Jakarta (UTC+7)
        datetime_now_tz(&dt, 420);  // 7*60 = 420
        datetime_format(&dt, "%Y-%m-%d %H:%M:%S WIB", buf, sizeof(buf));
        printf("%s\n", buf);

        // New York (UTC-5, assuming no DST)
        datetime_now_tz(&dt, -300);
        datetime_format(&dt, "%Y-%m-%d %H:%M:%S EST", buf, sizeof(buf));
        printf("%s\n", buf);

        return 0;
    }

================================================================================ 9. THREAD SAFETY
================================================================================

- The library does not use any internal static buffers for formatting;
  all output is written to caller‑provided buffers. Therefore, date_format
  and datetime_format are inherently thread‑safe.

- Time functions (date_now, datetime_now, etc.) use gmtime_r/localtime_r
  on POSIX systems and their _s counterparts on Windows, avoiding
  the non‑reentrant gmtime/localtime. They are safe to call from multiple
  threads concurrently.

- Functions that do not modify global state (all except time functions)
  are trivially thread‑safe.

================================================================================ 10. FREQUENTLY ASKED QUESTIONS (FAQ)
================================================================================

Q: Why does the parser expect day‑month‑year and not year‑month‑day?
A: The library was originally designed for locales where day‑month‑year
is common. If you need a different order, you can either rearrange the
fields after parsing or write a small wrapper.

Q: Is there support for dates before 1900?
A: No. The valid range is 1900‑01‑01 to 9999‑12‑31. Attempting to use dates
outside this range results in an error.

Q: Can I use this library in an embedded project?
A: Yes. The code is pure C99, uses no dynamic memory allocation (you provide
all buffers), and can be compiled with minimal standard library support.

Q: How does the library handle leap seconds?
A: It doesn't. The library works with calendar days; leap seconds are
irrelevant for date calculations.

Q: The 'datetime_format' function doesn't support my custom format. What can I do?
A: You can extend src/fmt.c by adding a new case for your specifier.
Contributions are welcome.

Q: Why does the library not use time_t internally for date differences?
A: To avoid the Year 2038 problem on systems with 32‑bit time_t, the library
uses its own epoch (1900‑01‑01) and int64_t arithmetic, ensuring safe
calculations up to year 9999.

Q: How do I link against the shared library?
A: After installation (cmake --install), use -lcdaydiff. Your linker will
pick up the shared library if available. If you want to force static
linking, use -l:libcdaydiff.a or link the .a file directly.

================================================================================ 11. CONTRIBUTING
================================================================================

Contributions are highly appreciated. To contribute:

1. Fork the repository.
2. Create a topic branch (git checkout -b feature/my-feature).
3. Make your changes. Ensure:
   - The code compiles cleanly with -Wall -Wextra -std=c99.
   - All existing unit tests pass (ctest or ./test_date).
   - New functionality is covered by additional tests in test.c.
4. Push to your branch and open a Pull Request on GitHub.

By submitting code, you agree that your contributions will be licensed
under the same GPL‑3.0 license.

================================================================================ 12. LICENSE
================================================================================

This project is released under the GNU General Public License version 3.
See the file LICENSE in the repository for the full license text.

================================================================================ 13. CONTACT
================================================================================

- Repository: https://github.com/mroczect/libcdaydiff
- Issues: https://github.com/mroczect/libcdaydiff/issues
- Author: mroczect
```
## ./license

```
                    GNU GENERAL PUBLIC LICENSE
                       Version 3, 29 June 2007

 Copyright (C) 2007 Free Software Foundation, Inc. <http://fsf.org/>
 Everyone is permitted to copy and distribute verbatim copies
 of this license document, but changing it is not allowed.

                            Preamble

  The GNU General Public License is a free, copyleft license for
software and other kinds of works.

  The licenses for most software and other practical works are designed
to take away your freedom to share and change the works.  By contrast,
the GNU General Public License is intended to guarantee your freedom to
share and change all versions of a program--to make sure it remains free
software for all its users.  We, the Free Software Foundation, use the
GNU General Public License for most of our software; it applies also to
any other work released this way by its authors.  You can apply it to
your programs, too.

  When we speak of free software, we are referring to freedom, not
price.  Our General Public Licenses are designed to make sure that you
have the freedom to distribute copies of free software (and charge for
them if you wish), that you receive source code or can get it if you
want it, that you can change the software or use pieces of it in new
free programs, and that you know you can do these things.

  To protect your rights, we need to prevent others from denying you
these rights or asking you to surrender the rights.  Therefore, you have
certain responsibilities if you distribute copies of the software, or if
you modify it: responsibilities to respect the freedom of others.

  For example, if you distribute copies of such a program, whether
gratis or for a fee, you must pass on to the recipients the same
freedoms that you received.  You must make sure that they, too, receive
or can get the source code.  And you must show them these terms so they
know their rights.

  Developers that use the GNU GPL protect your rights with two steps:
(1) assert copyright on the software, and (2) offer you this License
giving you legal permission to copy, distribute and/or modify it.

  For the developers' and authors' protection, the GPL clearly explains
that there is no warranty for this free software.  For both users' and
authors' sake, the GPL requires that modified versions be marked as
changed, so that their problems will not be attributed erroneously to
authors of previous versions.

  Some devices are designed to deny users access to install or run
modified versions of the software inside them, although the manufacturer
can do so.  This is fundamentally incompatible with the aim of
protecting users' freedom to change the software.  The systematic
pattern of such abuse occurs in the area of products for individuals to
use, which is precisely where it is most unacceptable.  Therefore, we
have designed this version of the GPL to prohibit the practice for those
products.  If such problems arise substantially in other domains, we
stand ready to extend this provision to those domains in future versions
of the GPL, as needed to protect the freedom of users.

  Finally, every program is threatened constantly by software patents.
States should not allow patents to restrict development and use of
software on general-purpose computers, but in those that do, we wish to
avoid the special danger that patents applied to a free program could
make it effectively proprietary.  To prevent this, the GPL assures that
patents cannot be used to render the program non-free.

  The precise terms and conditions for copying, distribution and
modification follow.

                       TERMS AND CONDITIONS

  0. Definitions.

  "This License" refers to version 3 of the GNU General Public License.

  "Copyright" also means copyright-like laws that apply to other kinds of
works, such as semiconductor masks.

  "The Program" refers to any copyrightable work licensed under this
License.  Each licensee is addressed as "you".  "Licensees" and
"recipients" may be individuals or organizations.

  To "modify" a work means to copy from or adapt all or part of the work
in a fashion requiring copyright permission, other than the making of an
exact copy.  The resulting work is called a "modified version" of the
earlier work or a work "based on" the earlier work.

  A "covered work" means either the unmodified Program or a work based
on the Program.

  To "propagate" a work means to do anything with it that, without
permission, would make you directly or secondarily liable for
infringement under applicable copyright law, except executing it on a
computer or modifying a private copy.  Propagation includes copying,
distribution (with or without modification), making available to the
public, and in some countries other activities as well.

  To "convey" a work means any kind of propagation that enables other
parties to make or receive copies.  Mere interaction with a user through
a computer network, with no transfer of a copy, is not conveying.

  An interactive user interface displays "Appropriate Legal Notices"
to the extent that it includes a convenient and prominently visible
feature that (1) displays an appropriate copyright notice, and (2)
tells the user that there is no warranty for the work (except to the
extent that warranties are provided), that licensees may convey the
work under this License, and how to view a copy of this License.  If
the interface presents a list of user commands or options, such as a
menu, a prominent item in the list meets this criterion.

  1. Source Code.

  The "source code" for a work means the preferred form of the work
for making modifications to it.  "Object code" means any non-source
form of a work.

  A "Standard Interface" means an interface that either is an official
standard defined by a recognized standards body, or, in the case of
interfaces specified for a particular programming language, one that
is widely used among developers working in that language.

  The "System Libraries" of an executable work include anything, other
than the work as a whole, that (a) is included in the normal form of
packaging a Major Component, but which is not part of that Major
Component, and (b) serves only to enable use of the work with that
Major Component, or to implement a Standard Interface for which an
implementation is available to the public in source code form.  A
"Major Component", in this context, means a major essential component
(kernel, window system, and so on) of the specific operating system
(if any) on which the executable work runs, or a compiler used to
produce the work, or an object code interpreter used to run it.

  The "Corresponding Source" for a work in object code form means all
the source code needed to generate, install, and (for an executable
work) run the object code and to modify the work, including scripts to
control those activities.  However, it does not include the work's
System Libraries, or general-purpose tools or generally available free
programs which are used unmodified in performing those activities but
which are not part of the work.  For example, Corresponding Source
includes interface definition files associated with source files for
the work, and the source code for shared libraries and dynamically
linked subprograms that the work is specifically designed to require,
such as by intimate data communication or control flow between those
subprograms and other parts of the work.

  The Corresponding Source need not include anything that users
can regenerate automatically from other parts of the Corresponding
Source.

  The Corresponding Source for a work in source code form is that
same work.

  2. Basic Permissions.

  All rights granted under this License are granted for the term of
copyright on the Program, and are irrevocable provided the stated
conditions are met.  This License explicitly affirms your unlimited
permission to run the unmodified Program.  The output from running a
covered work is covered by this License only if the output, given its
content, constitutes a covered work.  This License acknowledges your
rights of fair use or other equivalent, as provided by copyright law.

  You may make, run and propagate covered works that you do not
convey, without conditions so long as your license otherwise remains
in force.  You may convey covered works to others for the sole purpose
of having them make modifications exclusively for you, or provide you
with facilities for running those works, provided that you comply with
the terms of this License in conveying all material for which you do
not control copyright.  Those thus making or running the covered works
for you must do so exclusively on your behalf, under your direction
and control, on terms that prohibit them from making any copies of
your copyrighted material outside their relationship with you.

  Conveying under any other circumstances is permitted solely under
the conditions stated below.  Sublicensing is not allowed; section 10
makes it unnecessary.

  3. Protecting Users' Legal Rights From Anti-Circumvention Law.

  No covered work shall be deemed part of an effective technological
measure under any applicable law fulfilling obligations under article
11 of the WIPO copyright treaty adopted on 20 December 1996, or
similar laws prohibiting or restricting circumvention of such
measures.

  When you convey a covered work, you waive any legal power to forbid
circumvention of technological measures to the extent such circumvention
is effected by exercising rights under this License with respect to
the covered work, and you disclaim any intention to limit operation or
modification of the work as a means of enforcing, against the work's
users, your or third parties' legal rights to forbid circumvention of
technological measures.

  4. Conveying Verbatim Copies.

  You may convey verbatim copies of the Program's source code as you
receive it, in any medium, provided that you conspicuously and
appropriately publish on each copy an appropriate copyright notice;
keep intact all notices stating that this License and any
non-permissive terms added in accord with section 7 apply to the code;
keep intact all notices of the absence of any warranty; and give all
recipients a copy of this License along with the Program.

  You may charge any price or no price for each copy that you convey,
and you may offer support or warranty protection for a fee.

  5. Conveying Modified Source Versions.

  You may convey a work based on the Program, or the modifications to
produce it from the Program, in the form of source code under the
terms of section 4, provided that you also meet all of these conditions:

    a) The work must carry prominent notices stating that you modified
    it, and giving a relevant date.

    b) The work must carry prominent notices stating that it is
    released under this License and any conditions added under section
    7.  This requirement modifies the requirement in section 4 to
    "keep intact all notices".

    c) You must license the entire work, as a whole, under this
    License to anyone who comes into possession of a copy.  This
    License will therefore apply, along with any applicable section 7
    additional terms, to the whole of the work, and all its parts,
    regardless of how they are packaged.  This License gives no
    permission to license the work in any other way, but it does not
    invalidate such permission if you have separately received it.

    d) If the work has interactive user interfaces, each must display
    Appropriate Legal Notices; however, if the Program has interactive
    interfaces that do not display Appropriate Legal Notices, your
    work need not make them do so.

  A compilation of a covered work with other separate and independent
works, which are not by their nature extensions of the covered work,
and which are not combined with it such as to form a larger program,
in or on a volume of a storage or distribution medium, is called an
"aggregate" if the compilation and its resulting copyright are not
used to limit the access or legal rights of the compilation's users
beyond what the individual works permit.  Inclusion of a covered work
in an aggregate does not cause this License to apply to the other
parts of the aggregate.

  6. Conveying Non-Source Forms.

  You may convey a covered work in object code form under the terms
of sections 4 and 5, provided that you also convey the
machine-readable Corresponding Source under the terms of this License,
in one of these ways:

    a) Convey the object code in, or embodied in, a physical product
    (including a physical distribution medium), accompanied by the
    Corresponding Source fixed on a durable physical medium
    customarily used for software interchange.

    b) Convey the object code in, or embodied in, a physical product
    (including a physical distribution medium), accompanied by a
    written offer, valid for at least three years and valid for as
    long as you offer spare parts or customer support for that product
    model, to give anyone who possesses the object code either (1) a
    copy of the Corresponding Source for all the software in the
    product that is covered by this License, on a durable physical
    medium customarily used for software interchange, for a price no
    more than your reasonable cost of physically performing this
    conveying of source, or (2) access to copy the
    Corresponding Source from a network server at no charge.

    c) Convey individual copies of the object code with a copy of the
    written offer to provide the Corresponding Source.  This
    alternative is allowed only occasionally and noncommercially, and
    only if you received the object code with such an offer, in accord
    with subsection 6b.

    d) Convey the object code by offering access from a designated
    place (gratis or for a charge), and offer equivalent access to the
    Corresponding Source in the same way through the same place at no
    further charge.  You need not require recipients to copy the
    Corresponding Source along with the object code.  If the place to
    copy the object code is a network server, the Corresponding Source
    may be on a different server (operated by you or a third party)
    that supports equivalent copying facilities, provided you maintain
    clear directions next to the object code saying where to find the
    Corresponding Source.  Regardless of what server hosts the
    Corresponding Source, you remain obligated to ensure that it is
    available for as long as needed to satisfy these requirements.

    e) Convey the object code using peer-to-peer transmission, provided
    you inform other peers where the object code and Corresponding
    Source of the work are being offered to the general public at no
    charge under subsection 6d.

  A separable portion of the object code, whose source code is excluded
from the Corresponding Source as a System Library, need not be
included in conveying the object code work.

  A "User Product" is either (1) a "consumer product", which means any
tangible personal property which is normally used for personal, family,
or household purposes, or (2) anything designed or sold for incorporation
into a dwelling.  In determining whether a product is a consumer product,
doubtful cases shall be resolved in favor of coverage.  For a particular
product received by a particular user, "normally used" refers to a
typical or common use of that class of product, regardless of the status
of the particular user or of the way in which the particular user
actually uses, or expects or is expected to use, the product.  A product
is a consumer product regardless of whether the product has substantial
commercial, industrial or non-consumer uses, unless such uses represent
the only significant mode of use of the product.

  "Installation Information" for a User Product means any methods,
procedures, authorization keys, or other information required to install
and execute modified versions of a covered work in that User Product from
a modified version of its Corresponding Source.  The information must
suffice to ensure that the continued functioning of the modified object
code is in no case prevented or interfered with solely because
modification has been made.

  If you convey an object code work under this section in, or with, or
specifically for use in, a User Product, and the conveying occurs as
part of a transaction in which the right of possession and use of the
User Product is transferred to the recipient in perpetuity or for a
fixed term (regardless of how the transaction is characterized), the
Corresponding Source conveyed under this section must be accompanied
by the Installation Information.  But this requirement does not apply
if neither you nor any third party retains the ability to install
modified object code on the User Product (for example, the work has
been installed in ROM).

  The requirement to provide Installation Information does not include a
requirement to continue to provide support service, warranty, or updates
for a work that has been modified or installed by the recipient, or for
the User Product in which it has been modified or installed.  Access to a
network may be denied when the modification itself materially and
adversely affects the operation of the network or violates the rules and
protocols for communication across the network.

  Corresponding Source conveyed, and Installation Information provided,
in accord with this section must be in a format that is publicly
documented (and with an implementation available to the public in
source code form), and must require no special password or key for
unpacking, reading or copying.

  7. Additional Terms.

  "Additional permissions" are terms that supplement the terms of this
License by making exceptions from one or more of its conditions.
Additional permissions that are applicable to the entire Program shall
be treated as though they were included in this License, to the extent
that they are valid under applicable law.  If additional permissions
apply only to part of the Program, that part may be used separately
under those permissions, but the entire Program remains governed by
this License without regard to the additional permissions.

  When you convey a copy of a covered work, you may at your option
remove any additional permissions from that copy, or from any part of
it.  (Additional permissions may be written to require their own
removal in certain cases when you modify the work.)  You may place
additional permissions on material, added by you to a covered work,
for which you have or can give appropriate copyright permission.

  Notwithstanding any other provision of this License, for material you
add to a covered work, you may (if authorized by the copyright holders of
that material) supplement the terms of this License with terms:

    a) Disclaiming warranty or limiting liability differently from the
    terms of sections 15 and 16 of this License; or

    b) Requiring preservation of specified reasonable legal notices or
    author attributions in that material or in the Appropriate Legal
    Notices displayed by works containing it; or

    c) Prohibiting misrepresentation of the origin of that material, or
    requiring that modified versions of such material be marked in
    reasonable ways as different from the original version; or

    d) Limiting the use for publicity purposes of names of licensors or
    authors of the material; or

    e) Declining to grant rights under trademark law for use of some
    trade names, trademarks, or service marks; or

    f) Requiring indemnification of licensors and authors of that
    material by anyone who conveys the material (or modified versions of
    it) with contractual assumptions of liability to the recipient, for
    any liability that these contractual assumptions directly impose on
    those licensors and authors.

  All other non-permissive additional terms are considered "further
restrictions" within the meaning of section 10.  If the Program as you
received it, or any part of it, contains a notice stating that it is
governed by this License along with a term that is a further
restriction, you may remove that term.  If a license document contains
a further restriction but permits relicensing or conveying under this
License, you may add to a covered work material governed by the terms
of that license document, provided that the further restriction does
not survive such relicensing or conveying.

  If you add terms to a covered work in accord with this section, you
must place, in the relevant source files, a statement of the
additional terms that apply to those files, or a notice indicating
where to find the applicable terms.

  Additional terms, permissive or non-permissive, may be stated in the
form of a separately written license, or stated as exceptions;
the above requirements apply either way.

  8. Termination.

  You may not propagate or modify a covered work except as expressly
provided under this License.  Any attempt otherwise to propagate or
modify it is void, and will automatically terminate your rights under
this License (including any patent licenses granted under the third
paragraph of section 11).

  However, if you cease all violation of this License, then your
license from a particular copyright holder is reinstated (a)
provisionally, unless and until the copyright holder explicitly and
finally terminates your license, and (b) permanently, if the copyright
holder fails to notify you of the violation by some reasonable means
prior to 60 days after the cessation.

  Moreover, your license from a particular copyright holder is
reinstated permanently if the copyright holder notifies you of the
violation by some reasonable means, this is the first time you have
received notice of violation of this License (for any work) from that
copyright holder, and you cure the violation prior to 30 days after
your receipt of the notice.

  Termination of your rights under this section does not terminate the
licenses of parties who have received copies or rights from you under
this License.  If your rights have been terminated and not permanently
reinstated, you do not qualify to receive new licenses for the same
material under section 10.

  9. Acceptance Not Required for Having Copies.

  You are not required to accept this License in order to receive or
run a copy of the Program.  Ancillary propagation of a covered work
occurring solely as a consequence of using peer-to-peer transmission
to receive a copy likewise does not require acceptance.  However,
nothing other than this License grants you permission to propagate or
modify any covered work.  These actions infringe copyright if you do
not accept this License.  Therefore, by modifying or propagating a
covered work, you indicate your acceptance of this License to do so.

  10. Automatic Licensing of Downstream Recipients.

  Each time you convey a covered work, the recipient automatically
receives a license from the original licensors, to run, modify and
propagate that work, subject to this License.  You are not responsible
for enforcing compliance by third parties with this License.

  An "entity transaction" is a transaction transferring control of an
organization, or substantially all assets of one, or subdividing an
organization, or merging organizations.  If propagation of a covered
work results from an entity transaction, each party to that
transaction who receives a copy of the work also receives whatever
licenses to the work the party's predecessor in interest had or could
give under the previous paragraph, plus a right to possession of the
Corresponding Source of the work from the predecessor in interest, if
the predecessor has it or can get it with reasonable efforts.

  You may not impose any further restrictions on the exercise of the
rights granted or affirmed under this License.  For example, you may
not impose a license fee, royalty, or other charge for exercise of
rights granted under this License, and you may not initiate litigation
(including a cross-claim or counterclaim in a lawsuit) alleging that
any patent claim is infringed by making, using, selling, offering for
sale, or importing the Program or any portion of it.

  11. Patents.

  A "contributor" is a copyright holder who authorizes use under this
License of the Program or a work on which the Program is based.  The
work thus licensed is called the contributor's "contributor version".

  A contributor's "essential patent claims" are all patent claims
owned or controlled by the contributor, whether already acquired or
hereafter acquired, that would be infringed by some manner, permitted
by this License, of making, using, or selling its contributor version,
but do not include claims that would be infringed only as a
consequence of further modification of the contributor version.  For
purposes of this definition, "control" includes the right to grant
patent sublicenses in a manner consistent with the requirements of
this License.

  Each contributor grants you a non-exclusive, worldwide, royalty-free
patent license under the contributor's essential patent claims, to
make, use, sell, offer for sale, import and otherwise run, modify and
propagate the contents of its contributor version.

  In the following three paragraphs, a "patent license" is any express
agreement or commitment, however denominated, not to enforce a patent
(such as an express permission to practice a patent or covenant not to
sue for patent infringement).  To "grant" such a patent license to a
party means to make such an agreement or commitment not to enforce a
patent against the party.

  If you convey a covered work, knowingly relying on a patent license,
and the Corresponding Source of the work is not available for anyone
to copy, free of charge and under the terms of this License, through a
publicly available network server or other readily accessible means,
then you must either (1) cause the Corresponding Source to be so
available, or (2) arrange to deprive yourself of the benefit of the
patent license for this particular work, or (3) arrange, in a manner
consistent with the requirements of this License, to extend the patent
license to downstream recipients.  "Knowingly relying" means you have
actual knowledge that, but for the patent license, your conveying the
covered work in a country, or your recipient's use of the covered work
in a country, would infringe one or more identifiable patents in that
country that you have reason to believe are valid.

  If, pursuant to or in connection with a single transaction or
arrangement, you convey, or propagate by procuring conveyance of, a
covered work, and grant a patent license to some of the parties
receiving the covered work authorizing them to use, propagate, modify
or convey a specific copy of the covered work, then the patent license
you grant is automatically extended to all recipients of the covered
work and works based on it.

  A patent license is "discriminatory" if it does not include within
the scope of its coverage, prohibits the exercise of, or is
conditioned on the non-exercise of one or more of the rights that are
specifically granted under this License.  You may not convey a covered
work if you are a party to an arrangement with a third party that is
in the business of distributing software, under which you make payment
to the third party based on the extent of your activity of conveying
the work, and under which the third party grants, to any of the
parties who would receive the covered work from you, a discriminatory
patent license (a) in connection with copies of the covered work
conveyed by you (or copies made from those copies), or (b) primarily
for and in connection with specific products or compilations that
contain the covered work, unless you entered into that arrangement,
or that patent license was granted, prior to 28 March 2007.

  Nothing in this License shall be construed as excluding or limiting
any implied license or other defenses to infringement that may
otherwise be available to you under applicable patent law.

  12. No Surrender of Others' Freedom.

  If conditions are imposed on you (whether by court order, agreement or
otherwise) that contradict the conditions of this License, they do not
excuse you from the conditions of this License.  If you cannot convey a
covered work so as to satisfy simultaneously your obligations under this
License and any other pertinent obligations, then as a consequence you may
not convey it at all.  For example, if you agree to terms that obligate you
to collect a royalty for further conveying from those to whom you convey
the Program, the only way you could satisfy both those terms and this
License would be to refrain entirely from conveying the Program.

  13. Use with the GNU Affero General Public License.

  Notwithstanding any other provision of this License, you have
permission to link or combine any covered work with a work licensed
under version 3 of the GNU Affero General Public License into a single
combined work, and to convey the resulting work.  The terms of this
License will continue to apply to the part which is the covered work,
but the special requirements of the GNU Affero General Public License,
section 13, concerning interaction through a network will apply to the
combination as such.

  14. Revised Versions of this License.

  The Free Software Foundation may publish revised and/or new versions of
the GNU General Public License from time to time.  Such new versions will
be similar in spirit to the present version, but may differ in detail to
address new problems or concerns.

  Each version is given a distinguishing version number.  If the
Program specifies that a certain numbered version of the GNU General
Public License "or any later version" applies to it, you have the
option of following the terms and conditions either of that numbered
version or of any later version published by the Free Software
Foundation.  If the Program does not specify a version number of the
GNU General Public License, you may choose any version ever published
by the Free Software Foundation.

  If the Program specifies that a proxy can decide which future
versions of the GNU General Public License can be used, that proxy's
public statement of acceptance of a version permanently authorizes you
to choose that version for the Program.

  Later license versions may give you additional or different
permissions.  However, no additional obligations are imposed on any
author or copyright holder as a result of your choosing to follow a
later version.

  15. Disclaimer of Warranty.

  THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY
APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT
HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY
OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM
IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF
ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

  16. Limitation of Liability.

  IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS
THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY
GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE
USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF
DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD
PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS),
EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGES.

  17. Interpretation of Sections 15 and 16.

  If the disclaimer of warranty and limitation of liability provided
above cannot be given local legal effect according to their terms,
reviewing courts shall apply local law that most closely approximates
an absolute waiver of all civil liability in connection with the
Program, unless a warranty or assumption of liability accompanies a
copy of the Program in return for a fee.

                     END OF TERMS AND CONDITIONS

            How to Apply These Terms to Your New Programs

  If you develop a new program, and you want it to be of the greatest
possible use to the public, the best way to achieve this is to make it
free software which everyone can redistribute and change under these terms.

  To do so, attach the following notices to the program.  It is safest
to attach them to the start of each source file to most effectively
state the exclusion of warranty; and each file should have at least
the "copyright" line and a pointer to where the full notice is found.

    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2026  mroczect

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

Also add information on how to contact you by electronic and paper mail.

  If the program does terminal interaction, make it output a short
notice like this when it starts in an interactive mode:

    libcftext  Copyright (C) 2026  mroczect
    This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
    This is free software, and you are welcome to redistribute it
    under certain conditions; type `show c' for details.

The hypothetical commands `show w' and `show c' should show the appropriate
parts of the General Public License.  Of course, your program's commands
might be different; for a GUI interface, you would use an "about box".

  You should also get your employer (if you work as a programmer) or school,
if any, to sign a "copyright disclaimer" for the program, if necessary.
For more information on this, and how to apply and follow the GNU GPL, see
<http://www.gnu.org/licenses/>.

  The GNU General Public License does not permit incorporating your program
into proprietary programs.  If your program is a subroutine library, you
may consider it more useful to permit linking proprietary applications with
the library.  If this is what you want to do, use the GNU Lesser General
Public License instead of this License.  But first, please read
<http://www.gnu.org/philosophy/why-not-lgpl.html>.
```
## ./CMakeLists.txt

```text
cmake_minimum_required(VERSION 3.10)
project(libcdaydiff VERSION 1.1.0 LANGUAGES C)

# C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Warning flags (platform-agnostic)
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Define POSIX source on Unix-like systems
if(UNIX)
    add_definitions(-D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE)
endif()

# Collect source files
set(SOURCES
    src/core.c
    src/parser.c
    src/diff.c
    src/manip.c
    src/fmt.c
    src/tz.c
    src/epoch.c
    src/error.c
    src/info.c
    src/comparison.c
)

# Internal include directory
include_directories(src/include src/internal)

# Static library
add_library(cdaydiff_static STATIC ${SOURCES})
set_target_properties(cdaydiff_static PROPERTIES
    OUTPUT_NAME cdaydiff
    POSITION_INDEPENDENT_CODE ON
)

# Shared library
add_library(cdaydiff_shared SHARED ${SOURCES})
set_target_properties(cdaydiff_shared PROPERTIES
    OUTPUT_NAME cdaydiff
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# Export public header
target_include_directories(cdaydiff_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(cdaydiff_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
    $<INSTALL_INTERFACE:include>
)

# Alias (convenience)
add_library(libcdaydiff::static ALIAS cdaydiff_static)
add_library(libcdaydiff::shared ALIAS cdaydiff_shared)

# Unit test executable
add_executable(test_date test.c)
target_link_libraries(test_date PRIVATE cdaydiff_static)
# If you want to use ctest, add:
enable_testing()
add_test(NAME TestAll COMMAND test_date)

# Installation rules
include(GNUInstallDirs)

install(TARGETS cdaydiff_static cdaydiff_shared
    EXPORT libcdaydiff-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # for DLL on Windows
)

install(FILES src/include/libcdaydiff.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT libcdaydiff-targets
    FILE libcdaydiffTargets.cmake
    NAMESPACE libcdaydiff::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libcdaydiff
)

# Create a Version file for find_package
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libcdaydiffConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libcdaydiffConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libcdaydiff
)
```
## ./docs/content/api-reference.md

```markdown
---
title: "API Reference"
desc: "Complete API documentation for libcdaydiff"
author: "mroczect"
license: "GPL-3.0"
---

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
```
## ./docs/content/contac.md

```markdown
---
title: "Contact"
desc: "Get in touch with libcdaydiff maintainer"
author: "mroczect"
license: "GPL-3.0"
---

# Contact

If you have questions, suggestions, or need help with `libcdaydiff`, there are
several ways to reach out.

## GitHub

The primary place for bug reports, feature requests, and code contributions is
the GitHub repository:

> [https://github.com/mroczect/libcdaydiff](https://github.com/mroczect/libcdaydiff)

- **Issues**: Use the [issue tracker](https://github.com/mroczect/libcdaydiff/issues)
  to report a bug or suggest an improvement.
- **Pull Requests**: Submit code changes via
  [pull requests](https://github.com/mroczect/libcdaydiff/pulls). See the
  [Contributing Guide](contributing.md) for guidelines.

## Discussions

For general questions, ideas, or community discussions, use the
[GitHub Discussions](https://github.com/mroczect/libcdaydiff/discussions) tab
(if enabled). This is the best place to ask "how do I…" questions.

## Email

You can also contact the maintainer directly by email at:

> mroczect@proton.me

Please include `libcdaydiff` in the subject line for faster processing.

## Response Time

I aim to respond to issues and pull requests within a few days. However,
this is a side project, so there may be occasional delays.

> Thank you for using `libcdaydiff`.
```
## ./docs/content/contributing.md

```markdown
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
```
## ./docs/content/example.md

```markdown
# Full Examples

This page demonstrates practical uses of the `libcdaydiff` library.

## 1. Countdown to New Year

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    Date today, new_year = {1, 1, 2027};
    date_now(&today);

    int64_t remaining;
    date_diff_days(&today, &new_year, &remaining);
    printf("Days until 2027: %lld\n", (long long)remaining);

    return 0;
}
```

## 2. Calculate Age

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    Date birth = {15, 8, 1990};
    Date today;
    date_now(&today);

    DateDiff diff;
    date_diff_full(&birth, &today, &diff);
    printf("Age: %d years, %d months, %d days\n",
           diff.years, diff.months, diff.days);

    return 0;
}
```

## 3. Add Business Days (skip weekends)

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    Date d = {1, 7, 2026};  // Wednesday
    int days_to_add = 5;
    int added = 0;

    while (added < days_to_add) {
        date_add_days(&d, 1, &d);
        int dow = date_day_of_week(&d);
        if (dow != 0 && dow != 6)  // skip Sunday (0) and Saturday (6)
            added++;
    }
    char buf[50];
    date_format(&d, "%A, %d %B %Y", buf, sizeof(buf));
    printf("Result: %s\n", buf);
    return 0;
}
```

## 4. Format Current Time in Several Timezones

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    DateTime dt;
    char buf[100];

    // UTC
    datetime_now_utc(&dt);
    datetime_format(&dt, "%Y-%m-%d %H:%M:%S UTC", buf, sizeof(buf));
    printf("%s\n", buf);

    // Jakarta (UTC+7)
    datetime_now_tz(&dt, 420);  // 7 * 60 = 420
    datetime_format(&dt, "%Y-%m-%d %H:%M:%S WIB", buf, sizeof(buf));
    printf("%s\n", buf);

    // New York (UTC-5, standard time)
    datetime_now_tz(&dt, -300);
    datetime_format(&dt, "%Y-%m-%d %H:%M:%S EST", buf, sizeof(buf));
    printf("%s\n", buf);

    return 0;
}
```

## 5. Parse a Date and Convert to Epoch

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    Date d;
    date_parse("01.01.2020", NULL, &d);

    int64_t epoch;
    date_to_epoch_utc(&d, &epoch);
    printf("2020-01-01 00:00:00 UTC = %lld\n", (long long)epoch);

    // Convert back
    Date copy;
    date_from_epoch_utc(epoch, &copy);
    printf("Back to date: %04d-%02d-%02d\n", copy.year, copy.month, copy.day);

    return 0;
}
```

## 6. Simple Event Countdown (days only)

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    Date event = {25, 12, 2026}; // Christmas
    Date today;
    date_now(&today);

    int64_t days;
    date_diff_days(&today, &event, &days);
    if (days > 0)
        printf("%lld days until Christmas!\n", (long long)days);
    else if (days == 0)
        printf("Today is Christmas!\n");
    else
        printf("Christmas was %lld days ago.\n", (long long)-days);

    return 0;
}
```

## 7. Find the Last Day of the Month

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    Date d = {1, 2, 2024}; // February in a leap year
    int dim = date_days_in_month(d.month, d.year);
    d.day = dim;
    printf("Last day: %02d-%02d-%04d\n", d.day, d.month, d.year);
    return 0;
}
```

## 8. Check if a Year is a Leap Year

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    int year = 2024;
    if (date_is_leap_year(year))
        printf("%d is a leap year.\n", year);
    else
        printf("%d is not a leap year.\n", year);
    return 0;
}
```

## 9. Build a Simple Calendar Header

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    Date d = {1, 1, 2026};
    char buf[100];
    date_format(&d, "%B %Y", buf, sizeof(buf));
    printf("%s\n", buf);
    printf("Su Mo Tu We Th Fr Sa\n");

    // Print leading spaces
    int dow = date_day_of_week(&d);
    for (int i = 0; i < dow; i++) printf("   ");

    int dim = date_days_in_month(1, 2026);
    for (int day = 1; day <= dim; day++) {
        printf("%2d ", day);
        if ((dow + day) % 7 == 6) printf("\n");
    }
    printf("\n");
    return 0;
}
```

## 10. Validate User Input

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    char input[20];
    Date d;

    printf("Enter date (dd.mm.yyyy): ");
    fgets(input, sizeof(input), stdin);

    int rc = date_parse(input, NULL, &d);
    if (rc != DATE_OK) {
        printf("Invalid date: %s\n", date_error_string(rc));
        return 1;
    }
    printf("Valid date: %02d-%02d-%04d\n", d.day, d.month, d.year);
    return 0;
}
```

These examples cover the majority of the library's functionality. Combine
them as needed for your own applications.
```
## ./docs/content/format-specifier.md

```markdown
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
```
## ./docs/content/installation.md

```markdown
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
```
## ./docs/content/license.md

```markdown
# License

> _This project is released under the **GNU General Public License version 3**.
> See the file [LICENSE](https://github.com/mroczect/libcdaydiff?tab=GPL-3.0-1-ov-file) in the repository for the full license text._

```license
                    GNU GENERAL PUBLIC LICENSE
                       Version 3, 29 June 2007

 Copyright (C) 2007 Free Software Foundation, Inc. <http://fsf.org/>
 Everyone is permitted to copy and distribute verbatim copies
 of this license document, but changing it is not allowed.

                            Preamble

  The GNU General Public License is a free, copyleft license for
software and other kinds of works.

  The licenses for most software and other practical works are designed
to take away your freedom to share and change the works.  By contrast,
the GNU General Public License is intended to guarantee your freedom to
share and change all versions of a program--to make sure it remains free
software for all its users.  We, the Free Software Foundation, use the
GNU General Public License for most of our software; it applies also to
any other work released this way by its authors.  You can apply it to
your programs, too.

  When we speak of free software, we are referring to freedom, not
price.  Our General Public Licenses are designed to make sure that you
have the freedom to distribute copies of free software (and charge for
them if you wish), that you receive source code or can get it if you
want it, that you can change the software or use pieces of it in new
free programs, and that you know you can do these things.

  To protect your rights, we need to prevent others from denying you
these rights or asking you to surrender the rights.  Therefore, you have
certain responsibilities if you distribute copies of the software, or if
you modify it: responsibilities to respect the freedom of others.

  For example, if you distribute copies of such a program, whether
gratis or for a fee, you must pass on to the recipients the same
freedoms that you received.  You must make sure that they, too, receive
or can get the source code.  And you must show them these terms so they
know their rights.

  Developers that use the GNU GPL protect your rights with two steps:
(1) assert copyright on the software, and (2) offer you this License
giving you legal permission to copy, distribute and/or modify it.

  For the developers' and authors' protection, the GPL clearly explains
that there is no warranty for this free software.  For both users' and
authors' sake, the GPL requires that modified versions be marked as
changed, so that their problems will not be attributed erroneously to
authors of previous versions.

  Some devices are designed to deny users access to install or run
modified versions of the software inside them, although the manufacturer
can do so.  This is fundamentally incompatible with the aim of
protecting users' freedom to change the software.  The systematic
pattern of such abuse occurs in the area of products for individuals to
use, which is precisely where it is most unacceptable.  Therefore, we
have designed this version of the GPL to prohibit the practice for those
products.  If such problems arise substantially in other domains, we
stand ready to extend this provision to those domains in future versions
of the GPL, as needed to protect the freedom of users.

  Finally, every program is threatened constantly by software patents.
States should not allow patents to restrict development and use of
software on general-purpose computers, but in those that do, we wish to
avoid the special danger that patents applied to a free program could
make it effectively proprietary.  To prevent this, the GPL assures that
patents cannot be used to render the program non-free.

  The precise terms and conditions for copying, distribution and
modification follow.

                       TERMS AND CONDITIONS

  0. Definitions.

  "This License" refers to version 3 of the GNU General Public License.

  "Copyright" also means copyright-like laws that apply to other kinds of
works, such as semiconductor masks.

  "The Program" refers to any copyrightable work licensed under this
License.  Each licensee is addressed as "you".  "Licensees" and
"recipients" may be individuals or organizations.

  To "modify" a work means to copy from or adapt all or part of the work
in a fashion requiring copyright permission, other than the making of an
exact copy.  The resulting work is called a "modified version" of the
earlier work or a work "based on" the earlier work.

  A "covered work" means either the unmodified Program or a work based
on the Program.

  To "propagate" a work means to do anything with it that, without
permission, would make you directly or secondarily liable for
infringement under applicable copyright law, except executing it on a
computer or modifying a private copy.  Propagation includes copying,
distribution (with or without modification), making available to the
public, and in some countries other activities as well.

  To "convey" a work means any kind of propagation that enables other
parties to make or receive copies.  Mere interaction with a user through
a computer network, with no transfer of a copy, is not conveying.

  An interactive user interface displays "Appropriate Legal Notices"
to the extent that it includes a convenient and prominently visible
feature that (1) displays an appropriate copyright notice, and (2)
tells the user that there is no warranty for the work (except to the
extent that warranties are provided), that licensees may convey the
work under this License, and how to view a copy of this License.  If
the interface presents a list of user commands or options, such as a
menu, a prominent item in the list meets this criterion.

  1. Source Code.

  The "source code" for a work means the preferred form of the work
for making modifications to it.  "Object code" means any non-source
form of a work.

  A "Standard Interface" means an interface that either is an official
standard defined by a recognized standards body, or, in the case of
interfaces specified for a particular programming language, one that
is widely used among developers working in that language.

  The "System Libraries" of an executable work include anything, other
than the work as a whole, that (a) is included in the normal form of
packaging a Major Component, but which is not part of that Major
Component, and (b) serves only to enable use of the work with that
Major Component, or to implement a Standard Interface for which an
implementation is available to the public in source code form.  A
"Major Component", in this context, means a major essential component
(kernel, window system, and so on) of the specific operating system
(if any) on which the executable work runs, or a compiler used to
produce the work, or an object code interpreter used to run it.

  The "Corresponding Source" for a work in object code form means all
the source code needed to generate, install, and (for an executable
work) run the object code and to modify the work, including scripts to
control those activities.  However, it does not include the work's
System Libraries, or general-purpose tools or generally available free
programs which are used unmodified in performing those activities but
which are not part of the work.  For example, Corresponding Source
includes interface definition files associated with source files for
the work, and the source code for shared libraries and dynamically
linked subprograms that the work is specifically designed to require,
such as by intimate data communication or control flow between those
subprograms and other parts of the work.

  The Corresponding Source need not include anything that users
can regenerate automatically from other parts of the Corresponding
Source.

  The Corresponding Source for a work in source code form is that
same work.

  2. Basic Permissions.

  All rights granted under this License are granted for the term of
copyright on the Program, and are irrevocable provided the stated
conditions are met.  This License explicitly affirms your unlimited
permission to run the unmodified Program.  The output from running a
covered work is covered by this License only if the output, given its
content, constitutes a covered work.  This License acknowledges your
rights of fair use or other equivalent, as provided by copyright law.

  You may make, run and propagate covered works that you do not
convey, without conditions so long as your license otherwise remains
in force.  You may convey covered works to others for the sole purpose
of having them make modifications exclusively for you, or provide you
with facilities for running those works, provided that you comply with
the terms of this License in conveying all material for which you do
not control copyright.  Those thus making or running the covered works
for you must do so exclusively on your behalf, under your direction
and control, on terms that prohibit them from making any copies of
your copyrighted material outside their relationship with you.

  Conveying under any other circumstances is permitted solely under
the conditions stated below.  Sublicensing is not allowed; section 10
makes it unnecessary.

  3. Protecting Users' Legal Rights From Anti-Circumvention Law.

  No covered work shall be deemed part of an effective technological
measure under any applicable law fulfilling obligations under article
11 of the WIPO copyright treaty adopted on 20 December 1996, or
similar laws prohibiting or restricting circumvention of such
measures.

  When you convey a covered work, you waive any legal power to forbid
circumvention of technological measures to the extent such circumvention
is effected by exercising rights under this License with respect to
the covered work, and you disclaim any intention to limit operation or
modification of the work as a means of enforcing, against the work's
users, your or third parties' legal rights to forbid circumvention of
technological measures.

  4. Conveying Verbatim Copies.

  You may convey verbatim copies of the Program's source code as you
receive it, in any medium, provided that you conspicuously and
appropriately publish on each copy an appropriate copyright notice;
keep intact all notices stating that this License and any
non-permissive terms added in accord with section 7 apply to the code;
keep intact all notices of the absence of any warranty; and give all
recipients a copy of this License along with the Program.

  You may charge any price or no price for each copy that you convey,
and you may offer support or warranty protection for a fee.

  5. Conveying Modified Source Versions.

  You may convey a work based on the Program, or the modifications to
produce it from the Program, in the form of source code under the
terms of section 4, provided that you also meet all of these conditions:

    a) The work must carry prominent notices stating that you modified
    it, and giving a relevant date.

    b) The work must carry prominent notices stating that it is
    released under this License and any conditions added under section
    7.  This requirement modifies the requirement in section 4 to
    "keep intact all notices".

    c) You must license the entire work, as a whole, under this
    License to anyone who comes into possession of a copy.  This
    License will therefore apply, along with any applicable section 7
    additional terms, to the whole of the work, and all its parts,
    regardless of how they are packaged.  This License gives no
    permission to license the work in any other way, but it does not
    invalidate such permission if you have separately received it.

    d) If the work has interactive user interfaces, each must display
    Appropriate Legal Notices; however, if the Program has interactive
    interfaces that do not display Appropriate Legal Notices, your
    work need not make them do so.

  A compilation of a covered work with other separate and independent
works, which are not by their nature extensions of the covered work,
and which are not combined with it such as to form a larger program,
in or on a volume of a storage or distribution medium, is called an
"aggregate" if the compilation and its resulting copyright are not
used to limit the access or legal rights of the compilation's users
beyond what the individual works permit.  Inclusion of a covered work
in an aggregate does not cause this License to apply to the other
parts of the aggregate.

  6. Conveying Non-Source Forms.

  You may convey a covered work in object code form under the terms
of sections 4 and 5, provided that you also convey the
machine-readable Corresponding Source under the terms of this License,
in one of these ways:

    a) Convey the object code in, or embodied in, a physical product
    (including a physical distribution medium), accompanied by the
    Corresponding Source fixed on a durable physical medium
    customarily used for software interchange.

    b) Convey the object code in, or embodied in, a physical product
    (including a physical distribution medium), accompanied by a
    written offer, valid for at least three years and valid for as
    long as you offer spare parts or customer support for that product
    model, to give anyone who possesses the object code either (1) a
    copy of the Corresponding Source for all the software in the
    product that is covered by this License, on a durable physical
    medium customarily used for software interchange, for a price no
    more than your reasonable cost of physically performing this
    conveying of source, or (2) access to copy the
    Corresponding Source from a network server at no charge.

    c) Convey individual copies of the object code with a copy of the
    written offer to provide the Corresponding Source.  This
    alternative is allowed only occasionally and noncommercially, and
    only if you received the object code with such an offer, in accord
    with subsection 6b.

    d) Convey the object code by offering access from a designated
    place (gratis or for a charge), and offer equivalent access to the
    Corresponding Source in the same way through the same place at no
    further charge.  You need not require recipients to copy the
    Corresponding Source along with the object code.  If the place to
    copy the object code is a network server, the Corresponding Source
    may be on a different server (operated by you or a third party)
    that supports equivalent copying facilities, provided you maintain
    clear directions next to the object code saying where to find the
    Corresponding Source.  Regardless of what server hosts the
    Corresponding Source, you remain obligated to ensure that it is
    available for as long as needed to satisfy these requirements.

    e) Convey the object code using peer-to-peer transmission, provided
    you inform other peers where the object code and Corresponding
    Source of the work are being offered to the general public at no
    charge under subsection 6d.

  A separable portion of the object code, whose source code is excluded
from the Corresponding Source as a System Library, need not be
included in conveying the object code work.

  A "User Product" is either (1) a "consumer product", which means any
tangible personal property which is normally used for personal, family,
or household purposes, or (2) anything designed or sold for incorporation
into a dwelling.  In determining whether a product is a consumer product,
doubtful cases shall be resolved in favor of coverage.  For a particular
product received by a particular user, "normally used" refers to a
typical or common use of that class of product, regardless of the status
of the particular user or of the way in which the particular user
actually uses, or expects or is expected to use, the product.  A product
is a consumer product regardless of whether the product has substantial
commercial, industrial or non-consumer uses, unless such uses represent
the only significant mode of use of the product.

  "Installation Information" for a User Product means any methods,
procedures, authorization keys, or other information required to install
and execute modified versions of a covered work in that User Product from
a modified version of its Corresponding Source.  The information must
suffice to ensure that the continued functioning of the modified object
code is in no case prevented or interfered with solely because
modification has been made.

  If you convey an object code work under this section in, or with, or
specifically for use in, a User Product, and the conveying occurs as
part of a transaction in which the right of possession and use of the
User Product is transferred to the recipient in perpetuity or for a
fixed term (regardless of how the transaction is characterized), the
Corresponding Source conveyed under this section must be accompanied
by the Installation Information.  But this requirement does not apply
if neither you nor any third party retains the ability to install
modified object code on the User Product (for example, the work has
been installed in ROM).

  The requirement to provide Installation Information does not include a
requirement to continue to provide support service, warranty, or updates
for a work that has been modified or installed by the recipient, or for
the User Product in which it has been modified or installed.  Access to a
network may be denied when the modification itself materially and
adversely affects the operation of the network or violates the rules and
protocols for communication across the network.

  Corresponding Source conveyed, and Installation Information provided,
in accord with this section must be in a format that is publicly
documented (and with an implementation available to the public in
source code form), and must require no special password or key for
unpacking, reading or copying.

  7. Additional Terms.

  "Additional permissions" are terms that supplement the terms of this
License by making exceptions from one or more of its conditions.
Additional permissions that are applicable to the entire Program shall
be treated as though they were included in this License, to the extent
that they are valid under applicable law.  If additional permissions
apply only to part of the Program, that part may be used separately
under those permissions, but the entire Program remains governed by
this License without regard to the additional permissions.

  When you convey a copy of a covered work, you may at your option
remove any additional permissions from that copy, or from any part of
it.  (Additional permissions may be written to require their own
removal in certain cases when you modify the work.)  You may place
additional permissions on material, added by you to a covered work,
for which you have or can give appropriate copyright permission.

  Notwithstanding any other provision of this License, for material you
add to a covered work, you may (if authorized by the copyright holders of
that material) supplement the terms of this License with terms:

    a) Disclaiming warranty or limiting liability differently from the
    terms of sections 15 and 16 of this License; or

    b) Requiring preservation of specified reasonable legal notices or
    author attributions in that material or in the Appropriate Legal
    Notices displayed by works containing it; or

    c) Prohibiting misrepresentation of the origin of that material, or
    requiring that modified versions of such material be marked in
    reasonable ways as different from the original version; or

    d) Limiting the use for publicity purposes of names of licensors or
    authors of the material; or

    e) Declining to grant rights under trademark law for use of some
    trade names, trademarks, or service marks; or

    f) Requiring indemnification of licensors and authors of that
    material by anyone who conveys the material (or modified versions of
    it) with contractual assumptions of liability to the recipient, for
    any liability that these contractual assumptions directly impose on
    those licensors and authors.

  All other non-permissive additional terms are considered "further
restrictions" within the meaning of section 10.  If the Program as you
received it, or any part of it, contains a notice stating that it is
governed by this License along with a term that is a further
restriction, you may remove that term.  If a license document contains
a further restriction but permits relicensing or conveying under this
License, you may add to a covered work material governed by the terms
of that license document, provided that the further restriction does
not survive such relicensing or conveying.

  If you add terms to a covered work in accord with this section, you
must place, in the relevant source files, a statement of the
additional terms that apply to those files, or a notice indicating
where to find the applicable terms.

  Additional terms, permissive or non-permissive, may be stated in the
form of a separately written license, or stated as exceptions;
the above requirements apply either way.

  8. Termination.

  You may not propagate or modify a covered work except as expressly
provided under this License.  Any attempt otherwise to propagate or
modify it is void, and will automatically terminate your rights under
this License (including any patent licenses granted under the third
paragraph of section 11).

  However, if you cease all violation of this License, then your
license from a particular copyright holder is reinstated (a)
provisionally, unless and until the copyright holder explicitly and
finally terminates your license, and (b) permanently, if the copyright
holder fails to notify you of the violation by some reasonable means
prior to 60 days after the cessation.

  Moreover, your license from a particular copyright holder is
reinstated permanently if the copyright holder notifies you of the
violation by some reasonable means, this is the first time you have
received notice of violation of this License (for any work) from that
copyright holder, and you cure the violation prior to 30 days after
your receipt of the notice.

  Termination of your rights under this section does not terminate the
licenses of parties who have received copies or rights from you under
this License.  If your rights have been terminated and not permanently
reinstated, you do not qualify to receive new licenses for the same
material under section 10.

  9. Acceptance Not Required for Having Copies.

  You are not required to accept this License in order to receive or
run a copy of the Program.  Ancillary propagation of a covered work
occurring solely as a consequence of using peer-to-peer transmission
to receive a copy likewise does not require acceptance.  However,
nothing other than this License grants you permission to propagate or
modify any covered work.  These actions infringe copyright if you do
not accept this License.  Therefore, by modifying or propagating a
covered work, you indicate your acceptance of this License to do so.

  10. Automatic Licensing of Downstream Recipients.

  Each time you convey a covered work, the recipient automatically
receives a license from the original licensors, to run, modify and
propagate that work, subject to this License.  You are not responsible
for enforcing compliance by third parties with this License.

  An "entity transaction" is a transaction transferring control of an
organization, or substantially all assets of one, or subdividing an
organization, or merging organizations.  If propagation of a covered
work results from an entity transaction, each party to that
transaction who receives a copy of the work also receives whatever
licenses to the work the party's predecessor in interest had or could
give under the previous paragraph, plus a right to possession of the
Corresponding Source of the work from the predecessor in interest, if
the predecessor has it or can get it with reasonable efforts.

  You may not impose any further restrictions on the exercise of the
rights granted or affirmed under this License.  For example, you may
not impose a license fee, royalty, or other charge for exercise of
rights granted under this License, and you may not initiate litigation
(including a cross-claim or counterclaim in a lawsuit) alleging that
any patent claim is infringed by making, using, selling, offering for
sale, or importing the Program or any portion of it.

  11. Patents.

  A "contributor" is a copyright holder who authorizes use under this
License of the Program or a work on which the Program is based.  The
work thus licensed is called the contributor's "contributor version".

  A contributor's "essential patent claims" are all patent claims
owned or controlled by the contributor, whether already acquired or
hereafter acquired, that would be infringed by some manner, permitted
by this License, of making, using, or selling its contributor version,
but do not include claims that would be infringed only as a
consequence of further modification of the contributor version.  For
purposes of this definition, "control" includes the right to grant
patent sublicenses in a manner consistent with the requirements of
this License.

  Each contributor grants you a non-exclusive, worldwide, royalty-free
patent license under the contributor's essential patent claims, to
make, use, sell, offer for sale, import and otherwise run, modify and
propagate the contents of its contributor version.

  In the following three paragraphs, a "patent license" is any express
agreement or commitment, however denominated, not to enforce a patent
(such as an express permission to practice a patent or covenant not to
sue for patent infringement).  To "grant" such a patent license to a
party means to make such an agreement or commitment not to enforce a
patent against the party.

  If you convey a covered work, knowingly relying on a patent license,
and the Corresponding Source of the work is not available for anyone
to copy, free of charge and under the terms of this License, through a
publicly available network server or other readily accessible means,
then you must either (1) cause the Corresponding Source to be so
available, or (2) arrange to deprive yourself of the benefit of the
patent license for this particular work, or (3) arrange, in a manner
consistent with the requirements of this License, to extend the patent
license to downstream recipients.  "Knowingly relying" means you have
actual knowledge that, but for the patent license, your conveying the
covered work in a country, or your recipient's use of the covered work
in a country, would infringe one or more identifiable patents in that
country that you have reason to believe are valid.

  If, pursuant to or in connection with a single transaction or
arrangement, you convey, or propagate by procuring conveyance of, a
covered work, and grant a patent license to some of the parties
receiving the covered work authorizing them to use, propagate, modify
or convey a specific copy of the covered work, then the patent license
you grant is automatically extended to all recipients of the covered
work and works based on it.

  A patent license is "discriminatory" if it does not include within
the scope of its coverage, prohibits the exercise of, or is
conditioned on the non-exercise of one or more of the rights that are
specifically granted under this License.  You may not convey a covered
work if you are a party to an arrangement with a third party that is
in the business of distributing software, under which you make payment
to the third party based on the extent of your activity of conveying
the work, and under which the third party grants, to any of the
parties who would receive the covered work from you, a discriminatory
patent license (a) in connection with copies of the covered work
conveyed by you (or copies made from those copies), or (b) primarily
for and in connection with specific products or compilations that
contain the covered work, unless you entered into that arrangement,
or that patent license was granted, prior to 28 March 2007.

  Nothing in this License shall be construed as excluding or limiting
any implied license or other defenses to infringement that may
otherwise be available to you under applicable patent law.

  12. No Surrender of Others' Freedom.

  If conditions are imposed on you (whether by court order, agreement or
otherwise) that contradict the conditions of this License, they do not
excuse you from the conditions of this License.  If you cannot convey a
covered work so as to satisfy simultaneously your obligations under this
License and any other pertinent obligations, then as a consequence you may
not convey it at all.  For example, if you agree to terms that obligate you
to collect a royalty for further conveying from those to whom you convey
the Program, the only way you could satisfy both those terms and this
License would be to refrain entirely from conveying the Program.

  13. Use with the GNU Affero General Public License.

  Notwithstanding any other provision of this License, you have
permission to link or combine any covered work with a work licensed
under version 3 of the GNU Affero General Public License into a single
combined work, and to convey the resulting work.  The terms of this
License will continue to apply to the part which is the covered work,
but the special requirements of the GNU Affero General Public License,
section 13, concerning interaction through a network will apply to the
combination as such.

  14. Revised Versions of this License.

  The Free Software Foundation may publish revised and/or new versions of
the GNU General Public License from time to time.  Such new versions will
be similar in spirit to the present version, but may differ in detail to
address new problems or concerns.

  Each version is given a distinguishing version number.  If the
Program specifies that a certain numbered version of the GNU General
Public License "or any later version" applies to it, you have the
option of following the terms and conditions either of that numbered
version or of any later version published by the Free Software
Foundation.  If the Program does not specify a version number of the
GNU General Public License, you may choose any version ever published
by the Free Software Foundation.

  If the Program specifies that a proxy can decide which future
versions of the GNU General Public License can be used, that proxy's
public statement of acceptance of a version permanently authorizes you
to choose that version for the Program.

  Later license versions may give you additional or different
permissions.  However, no additional obligations are imposed on any
author or copyright holder as a result of your choosing to follow a
later version.

  15. Disclaimer of Warranty.

  THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY
APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT
HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY
OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM
IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF
ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

  16. Limitation of Liability.

  IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS
THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY
GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE
USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF
DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD
PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS),
EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGES.

  17. Interpretation of Sections 15 and 16.

  If the disclaimer of warranty and limitation of liability provided
above cannot be given local legal effect according to their terms,
reviewing courts shall apply local law that most closely approximates
an absolute waiver of all civil liability in connection with the
Program, unless a warranty or assumption of liability accompanies a
copy of the Program in return for a fee.

                     END OF TERMS AND CONDITIONS

            How to Apply These Terms to Your New Programs

  If you develop a new program, and you want it to be of the greatest
possible use to the public, the best way to achieve this is to make it
free software which everyone can redistribute and change under these terms.

  To do so, attach the following notices to the program.  It is safest
to attach them to the start of each source file to most effectively
state the exclusion of warranty; and each file should have at least
the "copyright" line and a pointer to where the full notice is found.

    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2026  mroczect

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

Also add information on how to contact you by electronic and paper mail.

  If the program does terminal interaction, make it output a short
notice like this when it starts in an interactive mode:

    libcftext  Copyright (C) 2026  mroczect
    This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
    This is free software, and you are welcome to redistribute it
    under certain conditions; type `show c' for details.

The hypothetical commands `show w' and `show c' should show the appropriate
parts of the General Public License.  Of course, your program's commands
might be different; for a GUI interface, you would use an "about box".

  You should also get your employer (if you work as a programmer) or school,
if any, to sign a "copyright disclaimer" for the program, if necessary.
For more information on this, and how to apply and follow the GNU GPL, see
<http://www.gnu.org/licenses/>.

  The GNU General Public License does not permit incorporating your program
into proprietary programs.  If your program is a subroutine library, you
may consider it more useful to permit linking proprietary applications with
the library.  If this is what you want to do, use the GNU Lesser General
Public License instead of this License.  But first, please read
<http://www.gnu.org/philosophy/why-not-lgpl.html>.
```
```
## ./docs/content/quick-start.md

```markdown
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
```
## ./docs/content/thread-savety.md

```markdown
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
```
## ./docs/content/index.md

```markdown
# libcdaydiff

> Portable C99 Date & Time Library

## Overview

libcdaydiff provides a comprehensive set of functions for date arithmetic,
parsing, formatting, and timezone-aware operations. No external dependencies,
compiles on any platform.

## Features

- Date parsing with auto-detection or custom delimiters
- Full validation including leap year rules
- Date difference (inclusive/exclusive days, full breakdown)
- Date arithmetic (add days, months, years)
- ISO week number, day of week, day of year
- Flexible formatting with printf-like specifiers
- Timezone support (local, UTC, arbitrary offset)
- Unix epoch conversion
- Clear error codes and messages
- Thread-safe design

[Get Started](quick-start.md)
```
## ./docs/config.yml

```yaml
title: "libcdaydiff"
desc: "Portable C99 Date & Time Library"
author: "mroczect"
repo_url: "https://github.com/mroczect/libcdaydiff"
license: "GPL-3.0"
site_name: "libcdaydiff"
base_url: "https://mroczect.github.io/libcdaydiff/"
content_dir: "content"
output_dir: "dist"
navbar:
  - label: Home
    url: index.html
  - label: Quick Start
    url: quick-start.html
  - label: API Reference
    url: api-reference.html
  - label: Examples
    url: example.html
  - label: Installation
    url: installation.html
  - label: Contributing
    url: contributing.html
  - label: License
    url: license.html
  - label: Contact
    url: contac.html
sidebar:
  - label: Getting Started
    items:
      - label: Home
        url: index.html
      - label: Quick Start
        url: quick-start.html
      - label: Installation
        url: installation.html
  - label: Documentation
    items:
      - label: API Reference
        url: api-reference.html
      - label: Format Specifiers
        url: format-specifier.html
      - label: Examples
        url: example.html
      - label: Thread Safety
        url: thread-savety.html
  - label: Project
    items:
      - label: Contributing
        url: contributing.html
      - label: License
        url: license.html
      - label: Contact
        url: contac.html
```
## ./snapcat.md

```markdown

```
