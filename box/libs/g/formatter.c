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

#define MAX_STACK_LEVEL 10
#define BUFFER_SIZE 128

typedef enum {
  STATUS_NORMAL=0,
  STATUS_STACK_FULL,
  STATUS_LITERAL,
  STATUS_WAIT_BRACKET
} Status;

static void _Add_Char(FmtStack *stack, char c) {
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

char *Fmt_Buffer_Get(FmtStack *stack) {
  Fmt *fmt = stack->fmt;
  _Add_Char(stack, '\0');
  --fmt->buffer_pos;
  return fmt->buffer;
}

void Fmt_Buffer_Clear(FmtStack *stack) {
  Fmt *fmt = stack->fmt;
  fmt->buffer_pos = 0;
}

static int _Draw(FmtStack *stack) {
  Fmt *fmt = stack->fmt;
  if (fmt->buffer_pos < 1) return stack->eye;
  if (fmt->draw != (FmtAction) NULL) fmt->draw(stack);
  return stack->eye;
}

static int _Text_Formatter(FmtStack *stack) {
  Fmt *fmt = stack->fmt;
  Status status;
  FmtStack new_stack;

  status = (stack->level >= MAX_STACK_LEVEL) ? STATUS_STACK_FULL : STATUS_NORMAL;

  for(;;) {
    char c = stack->text[stack->eye];
    if (c == '\0') return _Draw(stack);

    switch(status) {
    case STATUS_STACK_FULL:
      ++stack->eye;
      switch(c) {
      case '}':
        if (stack->level == MAX_STACK_LEVEL) return stack->eye;
        --stack->level;
        break;
      case '{':
        ++stack->level;
        break;
      }
      break;

    case STATUS_NORMAL:
      ++stack->eye;
      switch(c) {
      case '_':
        status = STATUS_WAIT_BRACKET;
        (void) _Draw(stack);
        new_stack = *stack;
        if (fmt->save != (FmtAction) NULL) fmt->save(stack);
        if (fmt->subscript != (FmtAction) NULL) fmt->subscript(& new_stack);
        break;
      case '^':
        status = STATUS_WAIT_BRACKET;
        (void) _Draw(stack);
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
        (void) _Draw(stack);
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

void Fmt_Init(Fmt *fmt) {
  fmt->restore = fmt->save = fmt->draw = fmt->newline =
    fmt->subscript = fmt->superscript = (FmtAction) NULL;
  fmt->private_data = (void *) NULL;
}

Fmt *Fmt_Get(FmtStack *stack) {
  return stack->fmt;
}

void *Fmt_Private_Get(Fmt *fmt) {
  return fmt->private_data;
}

void Fmt_Private_Set(Fmt *fmt, void *private_data) {
  fmt->private_data = private_data;
}

void Fmt_Text(Fmt *fmt, const char *text) {
  FmtStack stack;
  stack.level = 0;
  stack.eye = 0;
  stack.text = text;

  fmt->buffer_pos = 0;
  fmt->buffer_size = 0;
  fmt->buffer = (char *) NULL;
  stack.fmt = fmt;

  (void) _Text_Formatter(& stack);
  if (fmt->buffer != (char *) NULL) free(fmt->buffer);
}
