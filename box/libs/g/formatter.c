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
  STATUS_WAIT_SUB,
  STATUS_WAIT_SUP
} Status;

static void _Add_Char(BoxGFmtStack *stack, char c) {
  BoxGFmt *fmt = stack->fmt;
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

char *BoxGFmt_Get_Buffer(BoxGFmtStack *stack) {
  BoxGFmt *fmt = stack->fmt;
  _Add_Char(stack, '\0');
  --fmt->buffer_pos;
  return fmt->buffer;
}

void BoxGFmt_Clear_Buffer(BoxGFmtStack *stack) {
  BoxGFmt *fmt = stack->fmt;
  fmt->buffer_pos = 0;
}

static int _Draw(BoxGFmtStack *stack) {
  BoxGFmt *fmt = stack->fmt;
  if (fmt->buffer_pos < 1) return stack->eye;
  if (fmt->draw != NULL) fmt->draw(stack);
  return stack->eye;
}

static void _Save(BoxGFmtStack *stack) {
  BoxGFmt *fmt = stack->fmt;
  if (fmt->save != NULL) fmt->save(stack);
}

static void _Restore(BoxGFmtStack *stack) {
  BoxGFmt *fmt = stack->fmt;
  if (fmt->restore != NULL) fmt->restore(stack);
}

static void _Subscript(BoxGFmt *fmt, BoxGFmtStack *stack) {
  if (fmt->subscript != NULL) fmt->subscript(stack);
}

static void _Superscript(BoxGFmt *fmt, BoxGFmtStack *stack) {
  if (fmt->superscript != NULL) fmt->superscript(stack);
}

static int _Text_Formatter(BoxGFmtStack *stack) {
  BoxGFmt *fmt = stack->fmt;
  Status status;
  BoxGFmtStack new_stack;

  status = (stack->level >= MAX_STACK_LEVEL) ?
           STATUS_STACK_FULL : STATUS_NORMAL;

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
        status = STATUS_WAIT_SUB;
        break;
      case '^':
        status = STATUS_WAIT_SUP;
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
      ++stack->eye;
      _Add_Char(stack, c);
      break;

    case STATUS_WAIT_SUB:
    case STATUS_WAIT_SUP:
      ++stack->eye;
      switch(c) {
      case '^':
      case '_':
        _Add_Char(stack, c);
        status = STATUS_NORMAL;
        break;

      default:
        (void) _Draw(stack);
        _Save(stack);
        new_stack = *stack;
        ++new_stack.level;
        if (status == STATUS_WAIT_SUB)
          _Subscript(fmt, & new_stack);
        else
          _Superscript(fmt, & new_stack);

        if (c == '{') {
          stack->eye = _Text_Formatter(& new_stack);

        } else {
          /* We emulate the syntax 'char_{s}' for the syntex 'char_s' */
          new_stack.short_text[0] = c;
          new_stack.short_text[1] = '}';
          new_stack.short_text[2] = '\0';
          new_stack.text = new_stack.short_text;
          new_stack.eye = 0;
          (void) _Text_Formatter(& new_stack);
        }

        _Restore(stack);
        if (stack->eye == -1) return stack->eye;
        status = STATUS_NORMAL;
        break;
      }
      break;
    }
  }
}

void BoxGFmt_Init(BoxGFmt *fmt) {
  fmt->restore = fmt->save = fmt->draw = fmt->newline =
    fmt->subscript = fmt->superscript = NULL;
  fmt->private_data = (void *) NULL;
}

BoxGFmt *BoxGFmt_Get(BoxGFmtStack *stack) {
  return stack->fmt;
}

void *BoxGFmt_Get_Private(BoxGFmt *fmt) {
  return fmt->private_data;
}

void BoxGFmt_Set_Private(BoxGFmt *fmt, void *private_data) {
  fmt->private_data = private_data;
}

void BoxGFmt_Draw_Text(BoxGFmt *fmt, const char *text) {
  BoxGFmtStack stack;
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
