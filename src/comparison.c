#include "./internal/libcdaydiff_internal.h"

int date_compare(const Date *d1, const Date *d2) {
    if (d1->year != d2->year) return (d1->year < d2->year) ? -1 : 1;
    if (d1->month != d2->month) return (d1->month < d2->month) ? -1 : 1;
    if (d1->day != d2->day) return (d1->day < d2->day) ? -1 : 1;
    return 0;
}

int date_equals(const Date *d1, const Date *d2) {
    return date_compare(d1, d2) == 0;
}