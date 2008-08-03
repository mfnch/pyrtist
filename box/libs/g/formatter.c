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
#include <stdio.h>
#include <assert.h>

#include "types.h"
#include "formatter.h"

#define MAX_RECURSION 64
#define BUFFER_SIZE 128

typedef enum {
  STATUS_NORMAL=0,
  STATUS_LITERAL,
  STATUS_WAIT_BRACKET
} Status;

static void _Add_Char(Stack *stack, char c) {
  Fmt *fmt = stack->fmt;
  int i = fmt->buffer_pos++;
  if (fmt->buffer_pos > fmt->buffer_size) {
    if (fmt->buffer == (char *) NULL) {
      fmt->buffer = (char *) malloc(BUFFER_SIZE);
      fmt->buffer_size = BUFFER_SIZE;
    }

    if (fmt->buffer_pos > fmt->buffer_size) {
      while (fmt->buffer_pos > fmt->buffer_size) fmt->buffer_size *= 2;
      fmt->buffer = (char *) realloc(fmt->buffer, fmt->buffer_size);
    }

    assert(fmt->buffer != (char *) NULL);
  }

  fmt->buffer[i] = c;
}

#if 0
static void _Flush(Stack *stack) {
  Fmt *fmt = stack->fmt;
  _Add_Char(stack, '\0');
  printf("FLUSH: '%s'\n", fmt->buffer);
  fmt->buffer_pos = 0;
  stack->text += stack->eye;
  stack->eye = 0;
}
#endif

static int _Text_Formatter(Stack *stack) {
  Fmt *fmt = stack->fmt;
  Status status;
  Stack new_stack;

  status = STATUS_NORMAL;

  for(;;) {
    char c = stack->text[stack->eye];
    if (c == '\0') return stack->eye;

    switch(status) {
    case STATUS_NORMAL:
      ++stack->eye;
      switch(c) {
      case '_':
        status = STATUS_WAIT_BRACKET;
        if (fmt->draw != (FmtAction) NULL) fmt->draw(stack);
        new_stack = *stack;
        if (fmt->save != (FmtAction) NULL) fmt->save(stack);
        if (fmt->subscript != (FmtAction) NULL) fmt->subscript(& new_stack);
        break;
      case '^':
        status = STATUS_WAIT_BRACKET;
        if (fmt->draw != (FmtAction) NULL) fmt->draw(stack);
        new_stack = *stack;
        if (fmt->save != (FmtAction) NULL) fmt->save(stack);
        if (fmt->superscript != (FmtAction) NULL)
          fmt->superscript(& new_stack);
        break;
      case '\\':
        status = STATUS_LITERAL;
        break;
      case '}':
        if (stack->level > 0) {
          fmt->draw(stack);
          return stack->eye;
        }
        _Add_Char(stack, c);
        break;
      case '\n':
        fmt->newline(stack);
        break;
      default:
        _Add_Char(stack, c);
        break;
      }
      break;

    case STATUS_LITERAL:
      status = STATUS_NORMAL;
      _Add_Char(stack, c);
      break;

    case STATUS_WAIT_BRACKET:
      status = STATUS_NORMAL;
      ++stack->eye;
      switch(c) {
      case '{':
        new_stack = *stack;
        ++new_stack.level;
        stack->eye = _Text_Formatter(& new_stack);
        if (fmt->restore != (FmtAction) NULL) fmt->restore(stack);
        if (stack->eye == -1) return stack->eye;
        break;
      default:
        {
          char short_string[3];
          short_string[0] = c;
          short_string[1] = '}';
          short_string[2] = '\0';
          new_stack = *stack;
          ++new_stack.level;
          new_stack.text = short_string;
          new_stack.eye = 0;
          (void) _Text_Formatter(& new_stack);
          if (fmt->restore != (FmtAction) NULL) fmt->restore(stack);
          if (new_stack.eye == -1) return -1;
          status = STATUS_NORMAL;
          break;
        }
      }
      break;
    }
  }
}

void newline(Stack *stack) {
  Fmt *fmt = stack->fmt;
  _Add_Char(stack, '\0');
  printf("newline: '%s'\n", fmt->buffer);
  fmt->buffer_pos = 0;
}

void draw(Stack *stack) {
  Fmt *fmt = stack->fmt;
  _Add_Char(stack, '\0');
  printf("draw: '%s'\n", fmt->buffer);
  fmt->buffer_pos = 0;
}

void superscript(Stack *stack) {
  Fmt *fmt = stack->fmt;
  _Add_Char(stack, '\0');
  printf("superscript: '%s'\n", fmt->buffer);
  fmt->buffer_pos = 0;
}

void subscript(Stack *stack) {
  Fmt *fmt = stack->fmt;
  _Add_Char(stack, '\0');
  printf("subscript: '%s'\n", fmt->buffer);
  fmt->buffer_pos = 0;
}

void Fmt_Text(Fmt *fmt, const char *text) {
  Stack stack;
  stack.level = 0;
  stack.eye = 0;
  stack.text = text;

  fmt->buffer_pos = 0;
  fmt->buffer_size = 0;
  fmt->buffer = (char *) NULL;

  fmt->restore = fmt->save = (FmtAction) NULL;
  fmt->draw = draw;
  fmt->newline = newline;
  fmt->subscript = subscript;
  fmt->superscript = superscript;
  stack.fmt = fmt;

  (void) _Text_Formatter(& stack);
  if (fmt->buffer != (char *) NULL) free(fmt->buffer);
}
