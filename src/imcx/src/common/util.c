/**
 * (C) KetteQ, Inc.
 */

#include "imcx/src/include/util.h"

#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <utils/palloc.h>

// TODO: Modify to comply with postgres, use fixed size strings

char *convert_double_to_str (double number, int32 precision)
{
  char *format_str;
  bool dynamic_template = false;
  if (rint (number) != number)
    {
      int precision_len = snprintf (NULL, 0, "%d", precision);
      format_str = (char *)palloc ((precision_len + 3 + 1) * sizeof (char));
      sprintf (format_str, "%%.%df", precision);
      dynamic_template = true;
    }
  else
    {
      format_str = "%.0f";
    }
  char *return_str;
  int return_len = snprintf (NULL, 0, format_str, number);
  return_str = (char *)palloc ((return_len + 1) * sizeof (char));
  snprintf (return_str, return_len + 1, format_str, number);
  if (dynamic_template)
    {
      free (format_str);
    }
  return return_str;
}

void int_to_str (char *target, int source)
{
  sprintf (target, "%d", source);
}

char *convert_int_to_str (int number)
{
  char *returnStr;
  int number_len = snprintf (NULL, 0, "%d", number);
  returnStr = (char *)palloc ((number_len + 1) * sizeof (char));
  snprintf (returnStr, number_len + 1, "%d", number);
  return returnStr;
}

char *convert_long_to_str (long number)
{
  char *returnStr;
  int number_len = snprintf (NULL, 0, "%ld", number);
  returnStr = (char *)palloc ((number_len + 1) * sizeof (char));
  snprintf (returnStr, number_len + 1, "%ld", number);
  return returnStr;
}

char *convert_u_long_to_str (ulong number)
{
  char *returnStr;
  int number_len = snprintf (NULL, 0, "%ld", number);
  returnStr = (char *)palloc ((number_len + 1) * sizeof (char));
  snprintf (returnStr, number_len + 1, "%ld", number);
  return returnStr;
}

char *convert_u_int_to_str (uint number)
{
  char *returnStr;
  int number_len = snprintf (NULL, 0, "%d", number);
  returnStr = (char *)palloc ((number_len + 1) * sizeof (char));
  snprintf (returnStr, number_len + 1, "%d", number);
  return returnStr;
}

void str_to_lowercase (char *string)
{
  for (; *string; ++string) *string = (char)tolower (*string);
}