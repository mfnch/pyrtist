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

#ifndef _I_STYLE_H
#  define _I_STYLE_H

typedef struct {
  struct {
    unsigned int fill : 1;
  } have;

  struct {
    unsigned int on_pause : 1;
    unsigned int on_end : 1;
    enum {FILL_STYLE_PLAIN=1, FILL_STYLE_EOFILL,
          FILL_STYLE_CLIP, FILL_STYLE_EOCLIP} fill;
  } fill;
} Style;

#endif
