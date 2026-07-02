#include "./internal/libcdaydiff_internal.h"
#include <stdlib.h>
#include <time.h>

static int compute_local_offset(void) {
  time_t t = time(NULL);
  if (t == (time_t)-1)
    return 0;

  struct tm utc, local;
  gmtime_r(&t, &utc);
  localtime_r(&t, &local);

  return (int)(local.tm_gmtoff / 60);
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
  gmtime_r(&t, &tm);
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
  localtime_r(&t, &tm);
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
  gmtime_r(&t, &tm);
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
  localtime_r(&t, &tm);
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
  gmtime_r(&adj, &tm_adj);
  out->date.day = tm_adj.tm_mday;
  out->date.month = tm_adj.tm_mon + 1;
  out->date.year = tm_adj.tm_year + 1900;
  out->hour = tm_adj.tm_hour;
  out->minute = tm_adj.tm_min;
  out->second = tm_adj.tm_sec;
  out->tz_offset = offset_minutes;
  return DATE_OK;
}
