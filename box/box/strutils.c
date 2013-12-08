/****************************************************************************
 * Copyright (C) 2008 by Matteo Franchin                                    *
 *                                                                          *
 * This file is part of Box.                                                *
 *                                                                          *
 *   Box is free software: you can redistribute it and/or modify it         *
 *   under the terms of the GNU Lesser General Public License as published  *
 *   by the Free Software Foundation, either version 3 of the License, or   *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   Box is distributed in the hope that it will be useful,                 *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Lesser General Public License for more details.                    *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with Box.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "config.h"
#include "types.h"
#include "messages.h"
#include "mem.h"
#include "strutils.h"


static char *My_Str_Cut(const char *s, BoxUInt leng, BoxUInt maxleng,
                        BoxInt start)
{
  if ( leng <= maxleng )
    return Box_Mem_Strdup(s);

  else {
    char *rets, *c;
    rets = (char *) Box_Mem_Alloc(maxleng + 1);

    if ( start < 0 ) start = 0;
      else if ( start > 100 ) start = 100;

    start = (maxleng * start) / 100;
    if ( start > 0 )
        (void) strncpy(rets, s, start);

    c = rets + start;
    start = maxleng - start - 3;
    if ( start < 1 ) {
      for( start = 3 + start; start > 0; start-- )
        *(c++) = '.';

      *c = '\0';
      return rets;
    }

    *(c++) = '.'; *(c++) = '.'; *(c++) = '.';
    (void) strncpy(c, s + (leng - start), start);

    rets[maxleng] = '\0';
    return rets;
  }
}

/* Return an abbreviation of the input string. */
char *Box_Abbrev_Str(const char *s, size_t max_length, int start_percent)
{
  return My_Str_Cut(s, strlen(s), max_length, start_percent);
}

/* DESCRIPTION: This function reads one octal digit, whose ASCII code
 *  is in the character-constant c. In case of errors it sets *status = 1.
 */
int Box_Oct_Digit_To_Int(char digit)
{
  return (digit >= '0' && digit <= '7') ? (int) (digit - '0') : -1;
}

/** Return the hex digit (a char) corresponding to the given int value. */
char Box_Hex_Digit_From_Int(int v)
{
  switch(v) {
  case 0: return '0';
  case 1: return '1';
  case 2: return '2';
  case 3: return '3';
  case 4: return '4';
  case 5: return '5';
  case 6: return '6';
  case 7: return '7';
  case 8: return '8';
  case 9: return '9';
  case 10: return 'a';
  case 11: return 'b';
  case 12: return 'c';
  case 13: return 'd';
  case 14: return 'e';
  case 15: return 'f';
  default: return '?';
  }
}

/* Expand the given escaped character. */
static const char *My_Expand_Escaped_Char(const char *s, size_t s_length,
                                          size_t *consumed_chars, char *out)
{
//static BoxTask My_Reduce_Esc_Char(const char *s, size_t l, size_t *f, char *c)
  char cur_char;

  if (s_length < 1)
    return "Empty character";

  if (s[0] != '\\') {
    *out = s[0];
    *consumed_chars = 1;
    return NULL;
  }

  if (s_length < 2)
    return "Uncomplete escape sequence";

  cur_char = s[1];
  if (cur_char == 'x') {
    int value;

    if (s_length < 3)
      return "Unexpected end in \\x escape sequence";

    value = Box_Hex_Digit_To_Int(s[2]);
    if (value < 0)
      return "Invalid \\x escape sequence";

    if (s_length > 3) {
      int low_digit = Box_Hex_Digit_To_Int(s[3]);
      if (low_digit >= 0) {
        *out = (value << 4) | low_digit;
        *consumed_chars = 4;
        return NULL;
      }
    }

    *out = value;
    *consumed_chars = 3;
    return NULL;
  }

  if (cur_char >= '0' && cur_char <= '9') {
    int value = Box_Oct_Digit_To_Int(cur_char);

    if (s_length >= 3) {
      int mid_digit = Box_Oct_Digit_To_Int(s[2]);

      if (mid_digit >= 0) {
        value = (value << 3) | mid_digit;

        if (s_length > 3) {
          int low_digit = Box_Oct_Digit_To_Int(s[2]);
          if (low_digit >= 0) {
            value = (value << 3) | low_digit;
            if (value > 0xff)
              return "Octal number exceeds maximum value in escape sequence";
            *out = value;
            *consumed_chars = 4;
            return NULL;
          }
        }

        *out = value;
        *consumed_chars = 3;
        return NULL;
      }
    }

    *out = value;
    *consumed_chars = 2;
    return NULL;
  }

  switch (cur_char) {
  case 'a':  *out = '\a'; break;
  case 'b':  *out = '\b'; break;
  case 'f':  *out = '\f'; break;
  case 'n':  *out = '\n'; break;
  case 'r':  *out = '\r'; break;
  case 't':  *out = '\t'; break;
  case 'v':  *out = '\v'; break;
  case '\\': *out = '\\'; break;
  case '\?': *out = '\?'; break;
  case '\'': *out = '\''; break;
  case '\"': *out = '\"'; break;
  default:
    return "Unrecognized escape sequence.";
  }

  *consumed_chars = 2;
  return NULL;
}

/* Expand the given escaped character. */
const char *Box_Expand_Escaped_Char(const char *s, size_t s_length, char *out)
{
  size_t consumed_chars;
  char my_out;
  const char *err =
    My_Expand_Escaped_Char(s, s_length, & consumed_chars, & my_out);

  if (err)
    return err;

  if (consumed_chars != s_length)
    return "Escaped sequence contains too many characters";

  *out = my_out;
  return NULL;
}

/* Expand the given escaped string */
const char *Box_Expand_Escaped_Str(const char *s, size_t s_length,
                                   char **out, size_t *out_length)
{
  /* We can here assume that the out string is always smaller than the input
   * string: expansion always reduces the size of the string!
   */
  char *dst = (char *) Box_Mem_Alloc(s_length + 1);
  char *d = dst;
  size_t d_length = 0, consumed_chars = 0;

  for (; s_length > consumed_chars;
       s_length -= consumed_chars, s += consumed_chars) {
    char expanded_char = s[0];

    if (expanded_char == '\\') {
      const char *err =
        My_Expand_Escaped_Char(s, s_length, & consumed_chars, & expanded_char);
      if (err) {
        Box_Mem_Free(dst);
        return err;
      }
    } else
      consumed_chars = 1;

    /* Write destination string and advance write position. */
    *(d++) = expanded_char;
    d_length++;
  }

  *d = '\0';
  if (out_length)
    *out_length = d_length;

  if (out)
    *out = dst;
  else
    Box_Mem_Free(dst);

  return NULL;
}

/* Return the numerical value of a character when interpreted as hex. */
const char *Box_Str_To_Int(const char *s, BoxUInt s_length, BoxInt *out)
{
  char s_copy[sizeof(BoxInt)*5 + 1], *endptr;
  BoxInt result;

  if (s_length >= sizeof(BoxInt)*5 + 1)
    return "The number is too big to be represented as an Int.";

  strncpy(s_copy, s, s_length);
  s_copy[s_length] = '\0';

  errno = 0;
  result = BoxInt_Of_Str(s_copy, & endptr, 10);
  if (!errno) {
    *out = result;
    return NULL;
  }

  return "The number is too big to be represented as an Int";
}

/* Return the numerical value of a charecter when interpreted. */
int Box_Hex_Digit_To_Int(char digit)
{
  switch(tolower(digit)) {
  case '0': return 0;
  case '1': return 1;
  case '2': return 2;
  case '3': return 3;
  case '4': return 4;
  case '5': return 5;
  case '6': return 6;
  case '7': return 7;
  case '8': return 8;
  case '9': return 9;
  case 'a': return 10;
  case 'b': return 11;
  case 'c': return 12;
  case 'd': return 13;
  case 'e': return 14;
  case 'f': return 15;
  default: return -1;
  }
}

/* Convert a string of a hex number to a BoxInt. */
const char *Box_Hex_Str_To_Int(const char *s, size_t s_length, BoxInt *out)
{
  size_t i;
  BoxUInt result;

  for (i = 0, result = 0; i < s_length; i++) {
    int digit = Box_Hex_Digit_To_Int(*(s++));
    BoxUInt shifted_result = result << 4;

    if (digit < 0)
      return "Bad digit in hexadecimal number";

    if (shifted_result >> 4 != result)
      return "Hexadecimal number is out of bounds";

    result = shifted_result | digit;
  }

  *out = (BoxInt) result;
  return NULL;
}

/* Convert a string of to a BoxReal number. */
const char *Box_Str_To_Real(const char *s, size_t s_length, BoxReal *out)
{
  char *endptr;
  BoxReal result;
  int errno_value = 0;

  if (s_length < 64) {
    char s_copy[64];
    strncpy(s_copy, s, s_length);
    s_copy[s_length] = '\0';

    errno = 0;
    result = BoxReal_Of_Str(s_copy, & endptr);
    errno_value = errno;
  } else {
    char *s_copy = (char *) Box_Mem_Alloc(sizeof(char)*(s_length + 1));
    strncpy(s_copy, s, s_length);
    s_copy[s_length] = '\0';

    errno = 0;
    result = BoxReal_Of_Str(s_copy, & endptr);
    errno_value = errno;
    Box_Mem_Free(s_copy);
  }

  if (!errno_value) {
    *out = result;
    return NULL;
  }

  if (errno_value == ERANGE) {
    if (result == 0)
      return "Number is too small to be represented as Real";
    else
      return "Number is too large to be represented as Real";
  }

  return "Error while parsing Real number";
}

void *Box_Mem_Dup(const void *src, unsigned int length)
{
  void *copy;
  if (length < 1)
    return NULL;
  copy = (void *) Box_Mem_Alloc(length);
  if (copy && src)
    memcpy(copy, src, length);
  return copy;
}

/** Similar to Box_Mem_Dup, but allocate extra space */
void *Box_Mem_Dup_Larger(const void *src, BoxInt src_size, BoxInt dest_size)
{
  void *copy;
  assert(dest_size >= src_size && src_size >= 0);
  if (dest_size < 1) return NULL;
  copy = (void *) Box_Mem_Alloc(dest_size);
  if (copy == NULL) return NULL;
  memcpy(copy, src, src_size);
  return copy;
}

int Box_CStr_Ends_With(const char *src, const char *end)
{
  size_t l_src = strlen(src), l_end = strlen(end);
  if (l_src < l_end)
    return 0;

  else {
    const char *src_end = src + (l_src - l_end);
    return strcmp(src_end, end) == 0;
  }
}
