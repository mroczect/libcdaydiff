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
