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
  gmtime_r(&t, &utc);
  localtime_r(&t, &local);
  return (int)(local.tm_gmtoff / 60);
  int off = (local.tm_hour - utc.tm_hour) * 60 + (local.tm_min - utc.tm_min);
  int day_diff = local.tm_yday - utc.tm_yday;
  if (day_diff < -1)
    day_diff = 1;
  off += day_diff * 1440;
  return off;
}
