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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include <box/types.h>
#include <box/mem.h>
#include <box/print.h>

static char *msg = NULL;

void Box_Print_Finish(void) {
  Box_Mem_Free(msg);
  msg = NULL;
}

/* A simplified version of sprintf, with a number of desirable features:
 * - handles memory in a nice way: the user does not need to allocate/free
 *   the memory nor to worry about buffer overflow when using the %s
 *   specifier to write substrings. Usage is as follows:
 *       const char *msg = print("string = '%s', number = '%d'\n", "Hi!", 12);
 *       (* No need to call the free(...) function. The user mustn't do it! *)
 * - has %N to handle the BoxName data type.
 *   ES:
 *       BoxName my_name = {15, "Matteo Franchin"};
 *       msg = print("My name is %N, nice to meet you!", & my_name);
 * - has %~s, %~N to print a string and deallocate it with free(...).
 *   ES:
 *       msg = print("%S", Box_Mem_Strdup("allocated string"));
 */
const char *Box_Print(const char *fmt, ...) {
  static int buf_size = 0;
  unsigned int do_write = 1, do_read = 1, do_continue = 1, do_dealloc = 1,
    do_long = 0;
  char *str_dealloc = (char *) NULL;
  char cw = '?', cr = '?';
  const char *i;
  char *o;
  enum {STATE_NORMAL, STATE_CMND, STATE_SUBSTRING, STATE_END} state;
  BoxUInt size;
  int substring_size;
  char aux_buf[128], *substring = "?";
  va_list ap;

  if (!msg) {
    buf_size = BOX_PRINT_BUF_SIZE;
    msg = (char *) malloc(buf_size);
    if (!msg)
      goto print_error;
  } else if (buf_size > 2*BOX_PRINT_BUF_SIZE) {
    buf_size = BOX_PRINT_BUF_SIZE;
    msg = (char *) realloc(msg, buf_size);
    if (!msg)
      goto print_error;
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
      if (cr == '\0')
        state = STATE_END;;
    }

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
        do_long = 0;
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
      case 'l':
        do_long = 1;
        break;
      case 'c':
        cw = va_arg(ap, int);
        do_write = 1;
        state = STATE_NORMAL;
        break;
      case 's':
        do_read = 0;
        substring = va_arg(ap, char *);
        if (!substring) {
          substring = "(null)";
        } else {
          if (do_dealloc)
            str_dealloc = substring;
        }
        state = STATE_SUBSTRING;
        break;
      case 'd':
        do_read = 0;
        if (do_long)
          sprintf(aux_buf, "%ld", va_arg(ap, long));
        else
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
      case 'p':
        do_read = 0;
        sprintf(aux_buf, "%p", va_arg(ap, void *));
        substring = aux_buf;
        state = STATE_SUBSTRING;
        break;
      case 'I':
        do_read = 0;
        sprintf(aux_buf, SInt, va_arg(ap, BoxInt));
        substring = aux_buf;
        state = STATE_SUBSTRING;
        break;
      case 'U':
        do_read = 0;
        sprintf(aux_buf, SUInt, va_arg(ap, BoxUInt));
        substring = aux_buf;
        state = STATE_SUBSTRING;
        break;
      case 'R':
        do_read = 0;
        sprintf(aux_buf, SReal, va_arg(ap, BoxReal));
        substring = aux_buf;
        state = STATE_SUBSTRING;
        break;
      case 'P':
        {
          BoxPoint *p = va_arg(ap, BoxPoint *);
          do_read = 0;
          sprintf(aux_buf, "(" SReal ", " SReal ")", p->x, p->y);
          substring = aux_buf;
          state = STATE_SUBSTRING;
          break;
        }
      case 'N':
        {
          BoxName *nm = va_arg(ap, BoxName *);
          do_read = 0;
          substring = "(null)";
          if (nm && nm->text) {
            substring = nm->text;
            substring_size = nm->length;
          }
          state = STATE_SUBSTRING;
          break;
        }
      case 'T':
        {
          BoxType *t = va_arg(ap, BoxType *);
          if (t) {
            /* BoxType_Get_Repr() may call Box_Print(). We then need to save
             * the buffer. This is a temporary hack, while waiting for a proper
             * fix.
             */
            char *save_msg = msg;
            msg = NULL;
            str_dealloc = substring = BoxType_Get_Repr(t);
            free(msg);
            msg = save_msg;
          } else
            substring = "(BoxType*)0";

          do_read = 0;
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
        if (substring_size == 0)
          break;
        if (--substring_size == 0) {
          do_read = 1;
          state = STATE_NORMAL;
        }
        break;
      } else {
        if (str_dealloc) {
          free(str_dealloc);
          str_dealloc = NULL;
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
        if (!msg)
          goto print_error;
        o = msg + size;
      }
      *(o++) = cw;
      ++size;
    }
  }
  va_end(ap);
  return msg;

print_error:
  return "Box_Print: unexpected error!";
}

#if 0
int main(void) {
  BoxName my_name = {15, "Matteo Franchin"};
  const char *msg;
  msg = Box_Print("Don't worry! I can include '%s' even if "
                  "I don't know its length!\nI can also print my name: %N!\n"
                  "Or a number. Such as %d or %f!\n",
                  "this string", & my_name, 123, 3.1415926);
  printf("%s", msg);
  msg = Box_Print("This is a NUL string '%s'. No seg-fault ;-)\n", NULL);
  printf("%s", msg);
  return 0;
}
#endif
