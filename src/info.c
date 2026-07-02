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
