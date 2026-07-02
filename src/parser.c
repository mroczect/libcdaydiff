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
