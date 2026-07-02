#include "./include/libcdaydiff.h"

const char *date_error_string(DateError err) {
  switch (err) {
  case DATE_OK:
    return "Success";
  case DATE_ERR_NULL_POINTER:
    return "Null pointer";
  case DATE_ERR_PARSE:
    return "Parse error";
  case DATE_ERR_INVALID_DATE:
    return "Invalid date";
  case DATE_ERR_OVERFLOW:
    return "Overflow";
  case DATE_ERR_OUT_OF_RANGE:
    return "Out of range";
  case DATE_ERR_TIMEZONE:
    return "Timezone error";
  case DATE_ERR_FORMAT:
    return "Format error";
  default:
    return "Unknown error";
  }
}
