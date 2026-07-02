# Full Examples

This page demonstrates practical uses of the `libcdaydiff` library.

## 1. Countdown to New Year

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    Date today, new_year = {1, 1, 2027};
    date_now(&today);

    int64_t remaining;
    date_diff_days(&today, &new_year, &remaining);
    printf("Days until 2027: %lld\n", (long long)remaining);

    return 0;
}
```

## 2. Calculate Age

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    Date birth = {15, 8, 1990};
    Date today;
    date_now(&today);

    DateDiff diff;
    date_diff_full(&birth, &today, &diff);
    printf("Age: %d years, %d months, %d days\n",
           diff.years, diff.months, diff.days);

    return 0;
}
```

## 3. Add Business Days (skip weekends)

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    Date d = {1, 7, 2026};  // Wednesday
    int days_to_add = 5;
    int added = 0;

    while (added < days_to_add) {
        date_add_days(&d, 1, &d);
        int dow = date_day_of_week(&d);
        if (dow != 0 && dow != 6)  // skip Sunday (0) and Saturday (6)
            added++;
    }
    char buf[50];
    date_format(&d, "%A, %d %B %Y", buf, sizeof(buf));
    printf("Result: %s\n", buf);
    return 0;
}
```

## 4. Format Current Time in Several Timezones

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    DateTime dt;
    char buf[100];

    // UTC
    datetime_now_utc(&dt);
    datetime_format(&dt, "%Y-%m-%d %H:%M:%S UTC", buf, sizeof(buf));
    printf("%s\n", buf);

    // Jakarta (UTC+7)
    datetime_now_tz(&dt, 420);  // 7 * 60 = 420
    datetime_format(&dt, "%Y-%m-%d %H:%M:%S WIB", buf, sizeof(buf));
    printf("%s\n", buf);

    // New York (UTC-5, standard time)
    datetime_now_tz(&dt, -300);
    datetime_format(&dt, "%Y-%m-%d %H:%M:%S EST", buf, sizeof(buf));
    printf("%s\n", buf);

    return 0;
}
```

## 5. Parse a Date and Convert to Epoch

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    Date d;
    date_parse("01.01.2020", NULL, &d);

    int64_t epoch;
    date_to_epoch_utc(&d, &epoch);
    printf("2020-01-01 00:00:00 UTC = %lld\n", (long long)epoch);

    // Convert back
    Date copy;
    date_from_epoch_utc(epoch, &copy);
    printf("Back to date: %04d-%02d-%02d\n", copy.year, copy.month, copy.day);

    return 0;
}
```

## 6. Simple Event Countdown (days only)

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    Date event = {25, 12, 2026}; // Christmas
    Date today;
    date_now(&today);

    int64_t days;
    date_diff_days(&today, &event, &days);
    if (days > 0)
        printf("%lld days until Christmas!\n", (long long)days);
    else if (days == 0)
        printf("Today is Christmas!\n");
    else
        printf("Christmas was %lld days ago.\n", (long long)-days);

    return 0;
}
```

## 7. Find the Last Day of the Month

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    Date d = {1, 2, 2024}; // February in a leap year
    int dim = date_days_in_month(d.month, d.year);
    d.day = dim;
    printf("Last day: %02d-%02d-%04d\n", d.day, d.month, d.year);
    return 0;
}
```

## 8. Check if a Year is a Leap Year

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    int year = 2024;
    if (date_is_leap_year(year))
        printf("%d is a leap year.\n", year);
    else
        printf("%d is not a leap year.\n", year);
    return 0;
}
```

## 9. Build a Simple Calendar Header

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    Date d = {1, 1, 2026};
    char buf[100];
    date_format(&d, "%B %Y", buf, sizeof(buf));
    printf("%s\n", buf);
    printf("Su Mo Tu We Th Fr Sa\n");

    // Print leading spaces
    int dow = date_day_of_week(&d);
    for (int i = 0; i < dow; i++) printf("   ");

    int dim = date_days_in_month(1, 2026);
    for (int day = 1; day <= dim; day++) {
        printf("%2d ", day);
        if ((dow + day) % 7 == 6) printf("\n");
    }
    printf("\n");
    return 0;
}
```

## 10. Validate User Input

```c
#include <libcdaydiff.h>
#include <stdio.h>

int main(void) {
    char input[20];
    Date d;

    printf("Enter date (dd.mm.yyyy): ");
    fgets(input, sizeof(input), stdin);

    int rc = date_parse(input, NULL, &d);
    if (rc != DATE_OK) {
        printf("Invalid date: %s\n", date_error_string(rc));
        return 1;
    }
    printf("Valid date: %02d-%02d-%04d\n", d.day, d.month, d.year);
    return 0;
}
```

These examples cover the majority of the library's functionality. Combine
them as needed for your own applications.
