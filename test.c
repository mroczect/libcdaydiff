#include <libcdaydiff.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name)                                                             \
  do {                                                                         \
    printf("  TEST: %-50s ", name);                                            \
  } while (0)
#define CHECK(cond)                                                            \
  do {                                                                         \
    if (cond) {                                                                \
      printf("PASS\n");                                                        \
      tests_passed++;                                                          \
    } else {                                                                   \
      printf("FAIL (%s:%d)\n", __FILE__, __LINE__);                            \
      tests_failed++;                                                          \
    }                                                                          \
  } while (0)

/* ================================================================
 * 1. Parsing
 * ================================================================ */
static void test_parsing(void) {
  printf("\n=== Parsing ===\n");
  Date d;

  TEST("parse '06.05.2026' (auto-detect)");
  {
    int rc = date_parse("06.05.2026", NULL, &d);
    CHECK(rc == DATE_OK && d.day == 6 && d.month == 5 && d.year == 2026);
  }

  TEST("parse '08/12/2026' (auto-detect)");
  {
    int rc = date_parse("08/12/2026", NULL, &d);
    CHECK(rc == DATE_OK && d.day == 8 && d.month == 12 && d.year == 2026);
  }

  TEST("parse '31-01-2024' (delim '-')");
  {
    int rc = date_parse("31-01-2024", "-", &d);
    CHECK(rc == DATE_OK && d.day == 31 && d.month == 1 && d.year == 2024);
  }

  TEST("parse '15 08 1999' (delim ' ')");
  {
    int rc = date_parse("15 08 1999", " ", &d);
    CHECK(rc == DATE_OK && d.day == 15 && d.month == 8 && d.year == 1999);
  }

  TEST("parse invalid '32.01.2020' should fail");
  {
    int rc = date_parse("32.01.2020", ".", &d);
    CHECK(rc == DATE_ERR_INVALID_DATE);
  }

  TEST("parse invalid '29.02.2023' (not leap year) should fail");
  {
    int rc = date_parse("29.02.2023", ".", &d);
    CHECK(rc == DATE_ERR_INVALID_DATE);
  }

  TEST("parse '29.02.2024' (leap year) should succeed");
  {
    int rc = date_parse("29.02.2024", NULL, &d);
    CHECK(rc == DATE_OK && d.day == 29 && d.month == 2 && d.year == 2024);
  }

  TEST("parse with mixed delimiters should fail");
  {
    int rc = date_parse("01/02-2020", NULL, &d);
    CHECK(rc == DATE_ERR_PARSE);
  }

  TEST("parse empty string should fail");
  {
    int rc = date_parse("", NULL, &d);
    CHECK(rc != DATE_OK);
  }

  TEST("parse NULL string should fail");
  {
    int rc = date_parse(NULL, NULL, &d);
    CHECK(rc == DATE_ERR_NULL_POINTER);
  }
}

/* ================================================================
 * 2. Core validation
 * ================================================================ */
static void test_core(void) {
  printf("\n=== Core Functions ===\n");

  TEST("leap year 2000");
  CHECK(date_is_leap_year(2000) == 1);
  TEST("leap year 1900 (not)");
  CHECK(date_is_leap_year(1900) == 0);
  TEST("leap year 2024");
  CHECK(date_is_leap_year(2024) == 1);

  TEST("days in month Jan 2023");
  CHECK(date_days_in_month(1, 2023) == 31);
  TEST("days in month Feb 2023");
  CHECK(date_days_in_month(2, 2023) == 28);
  TEST("days in month Feb 2024");
  CHECK(date_days_in_month(2, 2024) == 29);

  TEST("validate 31/12/2022");
  CHECK(date_validate(31, 12, 2022) == 1);
  TEST("validate 0/1/2020 invalid");
  CHECK(date_validate(0, 1, 2020) == 0);
  TEST("validate 29/2/2023 invalid");
  CHECK(date_validate(29, 2, 2023) == 0);
  TEST("validate 29/2/2024 valid");
  CHECK(date_validate(29, 2, 2024) == 1);
  TEST("validate year 1899 out of range");
  CHECK(date_validate(1, 1, 1899) == 0);
  TEST("validate year 10000 out of range");
  CHECK(date_validate(1, 1, 10000) == 0);
}

/* ================================================================
 * 3. Difference (days)
 * ================================================================ */
static void test_diff_days(void) {
  printf("\n=== Day Differences ===\n");
  Date d1, d2;
  int64_t diff;

  date_parse("06.05.2026", ".", &d1);
  date_parse("08.12.2026", ".", &d2);

  TEST("inclusive diff 06.05.2026 -> 08.12.2026");
  {
    int rc = date_diff_days(&d1, &d2, &diff);
    /* Mei 6 to Mei 31 = 26 days? Let's compute manually:
       6 Mei to 31 Mei inclusive = (31-6+1)=26 hari,
       Juni 30, Juli 31, Agustus 31, Sep 30, Okt 31, Nov 30, Des 8 = 8,
       Total: 26+30+31+31+30+31+30+8 = 217? Perlu dicek. */
    CHECK(rc == DATE_OK &&
          diff == 217); // Saya hitung: (31-6+1)=26, lalu 30+31+31+30+31+30+8 =
                        // 191? Total 26+191=217.
  }

  TEST("exclusive diff 06.05.2026 -> 08.12.2026");
  {
    int rc = date_diff_days_excl(&d1, &d2, &diff);
    CHECK(rc == DATE_OK && diff == 216);
  }

  TEST("same date inclusive diff = 1");
  {
    Date same = {6, 5, 2026};
    int rc = date_diff_days(&same, &same, &diff);
    CHECK(rc == DATE_OK && diff == 1);
  }

  TEST("same date exclusive diff = 0");
  {
    Date same = {6, 5, 2026};
    int rc = date_diff_days_excl(&same, &same, &diff);
    CHECK(rc == DATE_OK && diff == 0);
  }

  TEST("NULL pointers return error");
  {
    int rc = date_diff_days(NULL, &d1, &diff);
    CHECK(rc == DATE_ERR_NULL_POINTER);
  }
}

/* ================================================================
 * 4. Full difference (years, months, days)
 * ================================================================ */
static void test_diff_full(void) {
  printf("\n=== Full Difference ===\n");
  Date d1, d2;
  DateDiff out;

  date_parse("01.01.2020", ".", &d1);
  date_parse("01.01.2021", ".", &d2);

  TEST("exactly 1 year");
  {
    int rc = date_diff_full(&d1, &d2, &out);
    CHECK(rc == DATE_OK && out.years == 1 && out.months == 0 && out.days == 0);
  }

  date_parse("15.03.2020", ".", &d1);
  date_parse("15.06.2021", ".", &d2);

  TEST("1 year 3 months");
  {
    int rc = date_diff_full(&d1, &d2, &out);
    CHECK(rc == DATE_OK && out.years == 1 && out.months == 3 && out.days == 0);
  }

  date_parse("31.01.2020", ".", &d1);
  date_parse("29.02.2020", ".", &d2); // leap year

  TEST("31 Jan -> 29 Feb in leap year");
  {
    int rc = date_diff_full(&d1, &d2, &out);
    CHECK(rc == DATE_OK && out.years == 0 && out.months == 1 &&
          out.days == 0); // karena 31 Jan -> 29 Feb, setelah add 1 bulan
                          // menjadi 29 Feb (pin end)
  }

  date_parse("01.01.2020", ".", &d1);
  date_parse("31.12.2020", ".", &d2);

  TEST("full year 2020");
  {
    int rc = date_diff_full(&d1, &d2, &out);
    CHECK(rc == DATE_OK && out.years == 0 && out.months == 11 &&
          out.days == 30);
  }

  TEST("reversed dates returns error");
  {
    int rc = date_diff_full(&d2, &d1, &out);
    CHECK(rc == DATE_ERR_INVALID_DATE);
  }
}

/* ================================================================
 * 5. Manipulation
 * ================================================================ */
static void test_manip(void) {
  printf("\n=== Manipulation ===\n");
  Date in, out;
  date_parse("06.05.2026", ".", &in);

  TEST("add 10 days");
  {
    int rc = date_add_days(&in, 10, &out);
    CHECK(rc == DATE_OK && out.day == 16 && out.month == 5 && out.year == 2026);
  }

  TEST("add 30 days crossing month");
  {
    Date start = {1, 5, 2026};
    int rc = date_add_days(&start, 30, &out);
    CHECK(rc == DATE_OK && out.day == 31 && out.month == 5 && out.year == 2026);
  }

  TEST("add 1 month to 31 Jan (non-leap)");
  {
    Date start = {31, 1, 2023};
    int rc = date_add_months(&start, 1, &out);
    CHECK(rc == DATE_OK && out.day == 28 && out.month == 2 && out.year == 2023);
  }

  TEST("add 1 month to 31 Jan (leap)");
  {
    Date start = {31, 1, 2024};
    int rc = date_add_months(&start, 1, &out);
    CHECK(rc == DATE_OK && out.day == 29 && out.month == 2 && out.year == 2024);
  }

  TEST("add 12 months (1 year)");
  {
    Date start = {15, 6, 2023};
    int rc = date_add_months(&start, 12, &out);
    CHECK(rc == DATE_OK && out.day == 15 && out.month == 6 && out.year == 2024);
  }

  TEST("add years leap day preservation");
  {
    Date start = {29, 2, 2024};
    int rc = date_add_years(&start, 1, &out);
    /* 2025 not leap, should pin to 28 Feb */
    CHECK(rc == DATE_OK && out.day == 28 && out.month == 2 && out.year == 2025);
  }

  TEST("add years overflow boundary");
  {
    Date start = {1, 1, 9999};
    int rc = date_add_years(&start, 1, &out);
    CHECK(rc == DATE_ERR_OVERFLOW);
  }

  TEST("add negative days");
  {
    Date start = {10, 1, 2020};
    int rc = date_add_days(&start, -5, &out);
    CHECK(rc == DATE_OK && out.day == 5 && out.month == 1 && out.year == 2020);
  }
}

/* ================================================================
 * 6. Information
 * ================================================================ */
static void test_info(void) {
  printf("\n=== Information ===\n");
  Date d;

  date_parse("01.01.2023", ".", &d); // Sunday
  TEST("day_of_week 1 Jan 2023 (Sunday)");
  CHECK(date_day_of_week(&d) == 0);

  date_parse("02.01.2023", ".", &d); // Monday
  TEST("day_of_week 2 Jan 2023 (Monday)");
  CHECK(date_day_of_week(&d) == 1);

  date_parse("31.12.2023", ".", &d);
  TEST("day_of_year 31 Dec 2023");
  CHECK(date_day_of_year(&d) == 365);

  date_parse("31.12.2020", ".", &d); // leap year
  TEST("day_of_year 31 Dec 2020 (leap)");
  CHECK(date_day_of_year(&d) == 366);

  // ISO week number tests
  date_parse("01.01.2023", ".", &d);
  TEST("ISO week 1 Jan 2023");
  CHECK(date_week_number_iso(&d) ==
        52); // 2023-01-01 is Sunday, week 52 of 2022

  date_parse("03.01.2023", ".", &d);
  TEST("ISO week 3 Jan 2023");
  CHECK(date_week_number_iso(&d) ==
        1); // first Monday is 2 Jan, week 1 starts 2 Jan

  date_parse("31.12.2023", ".", &d);
  TEST("ISO week 31 Dec 2023");
  CHECK(date_week_number_iso(&d) == 52); // 2023-12-31 is Sunday, week 52
}

/* ================================================================
 * 7. Formatting
 * ================================================================ */
static void test_format(void) {
  printf("\n=== Formatting ===\n");
  Date d;
  DateTime dt;
  char buf[128];

  date_parse("06.05.2026", ".", &d);

  TEST("format %d-%m-%Y");
  {
    date_format(&d, "%d-%m-%Y", buf, sizeof(buf));
    CHECK(strcmp(buf, "06-05-2026") == 0);
  }

  TEST("format %Y/%m/%d");
  {
    date_format(&d, "%Y/%m/%d", buf, sizeof(buf));
    CHECK(strcmp(buf, "2026/05/06") == 0);
  }

  TEST("format %B %d, %Y");
  {
    date_format(&d, "%B %d, %Y", buf, sizeof(buf));
    CHECK(strcmp(buf, "May 06, 2026") == 0);
  }

  TEST("format %a %A");
  {
    date_format(&d, "%a %A", buf, sizeof(buf));
    CHECK(strcmp(buf, "Wed Wednesday") == 0); // 6 May 2026 is Wednesday
  }

  /* DateTime formatting */
  dt.date = d;
  dt.hour = 14;
  dt.minute = 30;
  dt.second = 45;
  dt.tz_offset = 420; // +0700? 420=7*60

  TEST("datetime format %H:%M:%S");
  {
    datetime_format(&dt, "%H:%M:%S", buf, sizeof(buf));
    CHECK(strcmp(buf, "14:30:45") == 0);
  }

  TEST("datetime format %z (timezone)");
  {
    datetime_format(&dt, "%z", buf, sizeof(buf));
    CHECK(strcmp(buf, "+0700") == 0);
  }

  TEST("datetime combined format");
  {
    datetime_format(&dt, "%Y-%m-%dT%H:%M:%S%z", buf, sizeof(buf));
    CHECK(strcmp(buf, "2026-05-06T14:30:45+0700") == 0);
  }

  TEST("format buffer too small returns error");
  {
    int rc = date_format(&d, "%d-%m-%Y", buf, 5); // terlalu kecil
    CHECK(rc == DATE_ERR_FORMAT);
  }
}

/* ================================================================
 * 8. Comparison
 * ================================================================ */
static void test_comparison(void) {
  printf("\n=== Comparison ===\n");
  Date d1 = {1, 1, 2020};
  Date d2 = {1, 1, 2020};
  Date d3 = {2, 1, 2020};

  TEST("equal dates");
  CHECK(date_compare(&d1, &d2) == 0);
  TEST("d1 < d3");
  CHECK(date_compare(&d1, &d3) == -1);
  TEST("d3 > d1");
  CHECK(date_compare(&d3, &d1) == 1);
  TEST("date_equals");
  CHECK(date_equals(&d1, &d2) == 1);
  TEST("date_equals false");
  CHECK(date_equals(&d1, &d3) == 0);
}

/* ================================================================
 * 9. Real-time / Timezone
 * ================================================================ */
static void test_time(void) {
  printf("\n=== Real-time ===\n");
  Date d;
  DateTime dt;

  TEST("date_now returns valid");
  {
    int rc = date_now(&d);
    CHECK(rc == DATE_OK && date_validate(d.day, d.month, d.year));
  }

  TEST("date_now_utc returns valid");
  {
    int rc = date_now_utc(&d);
    CHECK(rc == DATE_OK && date_validate(d.day, d.month, d.year));
  }

  TEST("datetime_now returns valid");
  {
    int rc = datetime_now(&dt);
    CHECK(rc == DATE_OK && dt.hour < 24 && dt.minute < 60 && dt.second < 60);
  }

  TEST("datetime_now_utc offset 0");
  {
    int rc = datetime_now_utc(&dt);
    CHECK(rc == DATE_OK && dt.tz_offset == 0);
  }

  TEST("datetime_now_tz +420");
  {
    int rc = datetime_now_tz(&dt, 420);
    CHECK(rc == DATE_OK && dt.tz_offset == 420);
  }

  TEST("NULL pointer handling");
  {
    CHECK(date_now(NULL) == DATE_ERR_NULL_POINTER);
  }
}

/* ================================================================
 * 10. Epoch conversions
 * ================================================================ */
static void test_epoch(void) {
  printf("\n=== Epoch Conversions ===\n");
  Date d, back;
  int64_t secs;

  date_parse("01.01.2020", ".", &d);

  TEST("date_to_epoch_utc");
  {
    int rc = date_to_epoch_utc(&d, &secs);
    CHECK(rc == DATE_OK && secs > 0);
  }

  TEST("roundtrip UTC epoch");
  {
    int rc = date_to_epoch_utc(&d, &secs);
    if (rc == DATE_OK) {
      rc = date_from_epoch_utc(secs, &back);
      CHECK(rc == DATE_OK && date_compare(&d, &back) == 0);
    } else
      CHECK(0);
  }

  TEST("date_to_epoch_local (at least returns OK)");
  {
    int rc = date_to_epoch_local(&d, &secs);
    CHECK(rc == DATE_OK);
  }

  TEST("roundtrip local epoch");
  {
    int rc = date_to_epoch_local(&d, &secs);
    if (rc == DATE_OK) {
      rc = date_from_epoch_local(secs, &back);
      CHECK(rc == DATE_OK && date_compare(&d, &back) == 0);
    } else
      CHECK(0);
  }

  TEST("negative epoch returns error");
  {
    int rc = date_from_epoch_utc(-100000, &back);
    CHECK(rc == DATE_ERR_OUT_OF_RANGE);
  }
}

/* ================================================================
 * 11. Error string
 * ================================================================ */
static void test_error(void) {
  printf("\n=== Error Strings ===\n");
  TEST("DATE_OK string");
  CHECK(strcmp(date_error_string(DATE_OK), "Success") == 0);
  TEST("DATE_ERR_PARSE string");
  CHECK(strcmp(date_error_string(DATE_ERR_PARSE), "Parse error") == 0);
}

/* ================================================================
 * Main
 * ================================================================ */
int main(void) {
  test_parsing();
  test_core();
  test_diff_days();
  test_diff_full();
  test_manip();
  test_info();
  test_format();
  test_comparison();
  test_time();
  test_epoch();
  test_error();

  printf("\n========================================\n");
  printf(" Tests passed: %d\n", tests_passed);
  printf(" Tests failed: %d\n", tests_failed);
  printf("========================================\n");

  return tests_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
