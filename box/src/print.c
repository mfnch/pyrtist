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

/* $Id$ */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "types.h"
#include "mem.h"
#include "print.h"

static char *msg = NULL;

void Print_Finalize(void) {
  BoxMem_Free(msg);
  msg = (char *) NULL;
}

/* A simplified version of sprintf, with a number of desirable features:
 * - handles memory in a nice way: the user does not need to allocate/free
 *   the memory nor to worry about buffer overflow when using the %s
 *   specifier to write substrings. Usage is as follows:
 *       const char *msg = print("string = '%s', number = '%d'\n", "Hi!", 12);
 *       (* No need to call the free(...) function. The user mustn't do it! *)
 * - has %N to handle the Name data type.
 *   ES:
 *       Name my_name = {15, "Matteo Franchin"};
 *       msg = print("My name is %N, nice to meet you!", & my_name);
 * - has %~s, %~N to print a string and deallocate it with free(...).
 *   ES:
 *       msg = print("%S", BoxMem_Strdup("allocated string"));
 */
const char *print(const char *fmt, ...) {
  static int buf_size = 0;
  int do_write = 1, do_read = 1, do_continue = 1, do_dealloc = 1;
  char *str_dealloc = (char *) NULL;
  char cw = '?', cr = '?';
  const char *i;
  char *o;
  enum {STATE_NORMAL, STATE_CMND, STATE_SUBSTRING, STATE_END} state;
  UInt size;
  int substring_size;
  char aux_buf[128], *substring = "?";
  va_list ap;

  if ( msg == (char *) NULL ) {
    buf_size = PRINT_BUF_SIZE;
    msg = (char *) malloc(buf_size);
    if (msg == (char *) NULL) goto print_error;

  } else if (buf_size > 2*PRINT_BUF_SIZE) {
#ifdef DEBUG_PRINT
    printf("The size of the buffer seems to bee to big: reducing it!\n");
#endif
    buf_size = PRINT_BUF_SIZE;
    msg = (char *) realloc(msg, buf_size);
    if (msg == (char *) NULL) goto print_error;
  }

  state = STATE_NORMAL;
  i = fmt;
  o = msg;
  size = 0;
  substring_size = 0;
  va_start(ap, fmt);
  while(do_continue) {
    if (do_read) {
      cr = *(i++);
      if (cr == '\0') state = STATE_END;;
    }

#ifdef DEBUG_PRINT
    printf("---\n");
    printf("state = %d\n", state);
    printf("do_read = %d, cr = '%c'\n", do_read, cr);
#endif

    switch(state) {
    case STATE_NORMAL:
      assert(do_read);
      if (cr != '%') {
        cw = cr;
        do_write = 1;
        break;

      } else {
        do_dealloc = 0;
        do_write = 0;
        state = STATE_CMND;
        break;
      }

    case STATE_CMND:
      assert(do_read && !do_write);
      switch(cr) {
      case '%':
        cw = '%';
        do_write = 1;
        state = STATE_NORMAL;
        break;
      case '~':
        do_dealloc = 1;
        break;
      case 'c':
        cw = va_arg(ap, int);
        do_write = 1;
        state = STATE_NORMAL;
        break;
      case 's':
        do_read = 0;
        substring = va_arg(ap, char *);
        if (substring == (char *) NULL) {
          substring = "(null)";
        } else {
          if (do_dealloc) str_dealloc = substring;
        }
        state = STATE_SUBSTRING;
        break;
      case 'd':
        do_read = 0;
        sprintf(aux_buf, "%d", va_arg(ap, int));
        substring = aux_buf;
        state = STATE_SUBSTRING;
        break;
      case 'f':
        do_read = 0;
        sprintf(aux_buf, "%f", va_arg(ap, double));
        substring = aux_buf;
        state = STATE_SUBSTRING;
        break;
      case 'I':
        do_read = 0;
        sprintf(aux_buf, SInt, va_arg(ap, Int));
        substring = aux_buf;
        state = STATE_SUBSTRING;
        break;
      case 'U':
        do_read = 0;
        sprintf(aux_buf, SUInt, va_arg(ap, UInt));
        substring = aux_buf;
        state = STATE_SUBSTRING;
        break;
      case 'R':
        do_read = 0;
        sprintf(aux_buf, SReal, va_arg(ap, Real));
        substring = aux_buf;
        state = STATE_SUBSTRING;
        break;
      case 'P':
        {
          Point *p = va_arg(ap, Point *);
          do_read = 0;
          sprintf(aux_buf, "(" SReal ", " SReal ")", p->x, p->y);
          substring = aux_buf;
          state = STATE_SUBSTRING;
          break;
        }
      case 'N':
        {
          Name *nm = va_arg(ap, Name *);
          do_read = 0;
          substring = "(null)";
          if (nm != (Name *) NULL) {
            if (nm->text != (char *) NULL) {
              substring = nm->text;
              substring_size = nm->length;
            }
          }
          state = STATE_SUBSTRING;
          break;
        }
      default:
        state = STATE_NORMAL;
        break;
      }
      break;

    case STATE_SUBSTRING:
      assert(!do_read);
      cw = *(substring++);
      if (cw != '\0') {
        do_write = 1;
        if (substring_size == 0) break;
        if (--substring_size == 0) {
          do_read = 1;
          state = STATE_NORMAL;
        }
        break;
      } else {
        if (str_dealloc != (char *) NULL) {
          free(str_dealloc);
          str_dealloc = (char *) NULL;
        }
        do_write = 0;
        do_read = 1;
        state = STATE_NORMAL;
        break;
      }

    case STATE_END:
      cw = '\0';
      do_write = 1;
      do_continue = 0;
    }

#ifdef DEBUG_PRINT
    printf("do_write = %d, cw = '%c'\n", do_write, cw);
#endif

    if (do_write) {
      if (size >= buf_size) {
#ifdef DEBUG_PRINT
        printf("size(%d) > buf_size(%d): expanding buffer\n", size, buf_size);
#endif
        buf_size *= 2;
        msg = (char *) realloc(msg, buf_size);
        if (msg == (char *) NULL) goto print_error;
        o = msg + size;
      }
      *(o++) = cw;
      ++size;
    }
  }
  va_end(ap);
  return msg;

print_error:
  return "print: unexpected error!";
}

#if 0
int main(void) {
  Name my_name = {15, "Matteo Franchin"};
  const char *msg;
  msg = print("Don't worry! I can include '%s' even if "
   "I don't know its length a priori!\nI can also print my name: %N!\n"
   "Or a number. Such as %d or %f!\n",
   "this string", & my_name, 123, 3.1415926);
  printf("%s", msg);
  msg = print("This is a NUL string '%s'. No seg-fault ;-)\n", (char *) NULL);
  printf("%s", msg);
  return 0;
}
#endif
