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
