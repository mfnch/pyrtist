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

#ifndef _FORMATTER_H
#  define _FORMATTER_H

#  include "types.h"

struct _stack;
typedef void (*FmtAction)(struct _stack *stack);

typedef struct {
  int buffer_pos, buffer_size;
  char *buffer;

  FmtAction save, restore, draw, subscript, superscript, newline;
} Fmt;

typedef struct _stack {
  int level;
  int eye;
  const char *text;
  Point pos;
  Fmt *fmt;
} Stack;

void Fmt_Text(Fmt *fmt, const char *text);

#endif
