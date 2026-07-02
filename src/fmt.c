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
