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

#ifndef _GPATH_H
#  define _GPATH_H

#  include "types.h"

enum {
  GPATHKIND_LINE,
  GPATHKIND_ARC
} GPathKind;

typedef struct {
  int kind;
  Point p[3];
} GPathPiece;

typedef struct {
  Real tolerance;

} GPath;

/* For now a GPath is not thought to be modified:
 * we don't provide functions to change the pieces which have already
 * been added to the GPath.
 */

GPath *gpath_init(void);

void gpath_destroy(GPath *p);

void gpath_append(GPath *p, Point *point, int join);

void gpath_move_to(GPath *p, Point *point);

void gpath_line_to(GPath *p, Point *point);

void gpath_arc_to(GPath *p, Point *p1, Point *p2);

/** This function returns the length of the path. */
void gpath_length(GPath *p);

/** This function returns the number of pieces in the path. */
void gpath_num_pieces(GPath *p);

/** This function returns a point in the path, such that the sub-path
 * which connects it to the starting point has the provided 'length'.
 */
void gpath_get_first_point_at_length(GPath *p, Real length);

void gpath_get_last_point_at_length(GPath *p, Real length);

/** This function returns the first point of the provided 'piece'.
 * If 'piece' is not an integer, then a proper interpolation is made.
 * Therefore:
 *  - 0.0 is the first point of the first piece of the path;
 *  - 0.5 is the point in the middle of the first piece of the path;
 *  - 1.0 is the first point of the second piece of the path;
 *  ...
 * NOTE: this function returns the same output returned by the function
 *  gpath_get_last_point_of_piece if the path is not broken (=closed).
 */
void gpath_get_first_point_of_piece(GPath *p, Real piece);

/** This function returns the last point of the provided 'piece'.
 * If 'piece' is not an integer, then a proper interpolation is made.
 * Therefore:
 *  - 1.0 is the last point of the first piece of the path;
 *  - 1.5 is the point in the middle of the second piece of the path;
 *  - 2.0 is the last point of the second piece of the path;
 *  ...
 * NOTE: this function returns the same output returned by the function
 *  gpath_get_first_point_of_piece if the path is not broken (=closed).
 */
void gpath_get_last_point_of_piece(GPath *p, Real piece);

void gpath_get_piece_from_length(GPath *p, Real length);

void gpath_get_length_from_piece(GPath *p, Real piece);

void gpath_subgpath(GPath *p, GPath *subpath, Real first_piece, Real last_piece);

void gpath_append_reversed(GPath *in, GPath *out, int join);

void gpath_append_gpath(GPath *in, GPath *out, int join);

#endif
