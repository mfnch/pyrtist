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

void path_init(Path **p) {}

void path_appent(Path *p, Point *point, int join) {}

void path_move_to(Path *p, Point *point) {}

void path_line_to(Path *p, Point *point) {}

void path_arc_to(Path *p, Point *p1, Point *p2) {}

void path_length(Path *p) {}

void path_num_pieces(Path *p) {}

void path_get_first_point_at_length(Path *p, Real length) {}

void path_get_last_point_at_length(Path *p, Real length) {}

void path_get_first_point_of_piece(Path *p, Real piece) {}

void path_get_last_point_of_piece(Path *p, Real piece) {}

void path_get_piece_from_length(Path *p, Real length) {}

void path_get_length_from_piece(Path *p, Real piece) {}

void path_subpath(Path *p, Path *subpath, Real first_piece, Real last_piece) {}

void path_append_reversed(Path *in, Path *out, int join) {}

void path_append(Path *in, Path *out, int join) {}
