/****************************************************************************
 * Copyright (C) 2008-2011 by Matteo Franchin                               *
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

#ifndef _BOX_LIBG_FORMATTER_H
#  define _BOX_LIBG_FORMATTER_H

#  include <box/types.h>

struct _fmt_stack;
typedef void (*BoxGFmtAction)(struct _fmt_stack *stack);

typedef struct {
  int       buffer_pos,
            buffer_size;
  char      *buffer;
  void      *private_data;
  BoxGFmtAction
            save,
            restore,
            draw,
            subscript,
            superscript,
            newline;
} BoxGFmt;

typedef struct _fmt_stack {
  int        level,
             eye;
  const char *text;
  char       short_text[3];
  Point      pos;
  BoxGFmt    *fmt;
} BoxGFmtStack;

void BoxGFmt_Init(BoxGFmt *fmt);
char *BoxGFmt_Get_Buffer(BoxGFmtStack *stack);
void BoxGFmt_Clear_Buffer(BoxGFmtStack *stack);
void BoxGFmt_Draw_Text(BoxGFmt *fmt, const char *text);


BoxGFmt *BoxGFmt_Get(BoxGFmtStack *stack);
void *BoxGFmt_Get_Private(BoxGFmt *fmt);
void BoxGFmt_Set_Private(BoxGFmt *fmt, void *private_data);

#endif

