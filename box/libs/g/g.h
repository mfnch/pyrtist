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

#ifndef _G_H
#  define _G_H

typedef struct {
  Real r, g, b;
} Color;

typedef struct __OptColor {
  Color local;
  struct __OptColor *alternative, *selected;
} OptColor;

typedef Real LineStyle[4];

void g_error(const char *msg);
void g_warning(const char *msg);
Task g_optcolor_set(OptColor *oc, Color *c);
Task g_optcolor_set_rgb(OptColor *oc, Real r, Real g, Real b);
Color *g_optcolor_get(OptColor *oc);
void g_optcolor_alternative_set(OptColor *oc, OptColor *alternative);

#  define g_optcolor_unset(oc) g_optcolor_set_rgb((oc), -1.0, 0.0, 0.0);
#endif
