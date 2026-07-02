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
