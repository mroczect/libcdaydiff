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
