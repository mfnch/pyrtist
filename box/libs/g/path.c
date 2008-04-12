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
 *   but WITHOUT ANY WARRANTY {} without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Lesser General Public License for more details.                    *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with Box.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

#include "types.h"
#include "path.h"

void gpath_init(GPath **p) {}

void gpath_destroy(GPath *p) {}

void gpath_appent(GPath *p, Point *point, int join) {}

void gpath_move_to(GPath *p, Point *point) {}

void gpath_line_to(GPath *p, Point *point) {}

void gpath_arc_to(GPath *p, Point *p1, Point *p2) {}

void gpath_length(GPath *p) {}

void gpath_num_pieces(GPath *p) {}

void gpath_get_first_point_at_length(GPath *p, Real length) {}

void gpath_get_last_point_at_length(GPath *p, Real length) {}

void gpath_get_first_point_of_piece(GPath *p, Real piece) {}

void gpath_get_last_point_of_piece(GPath *p, Real piece) {}

void gpath_get_piece_from_length(GPath *p, Real length) {}

void gpath_get_length_from_piece(GPath *p, Real piece) {}

void gpath_subpath(GPath *p, GPath *subpath, Real first_piece, Real last_piece) {}

void gpath_append_reversed(GPath *in, GPath *out, int join) {}

void gpath_append(GPath *in, GPath *out, int join) {}
