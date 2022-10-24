/**
 * (C) KetteQ, Inc.
 */

#include "imcx/src/include/util.h"

#include <ctype.h>
#include <math.h>
#include <stdbool.h>

void double_to_str(char *target, double number, int32 precision) {
  char format_str[32] = "%.0f";
  if (rint(number) != number) { // This means the number is not an integer inside a double.
    // Now we prepare the template
    sprintf(format_str, "%%.%df", precision);
  }
  sprintf(target, format_str, number);
}

void int32_to_str(char *target, int32 source) {
  sprintf(target, "%d", source);
}

void bool_to_str(char *target, bool source) {
  sprintf(target, "%s", source ? "True" : "False");
}

void str_to_lowercase(char *string) {
  for (; *string; ++string) *string = (char)tolower(*string);
}