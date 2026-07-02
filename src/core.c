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
