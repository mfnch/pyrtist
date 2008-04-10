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

#ifndef _PATH_H
#  define _PATH_H

#  include "types.h"

enum {
  PATHKIND_LINE,
  PATHKIND_ARC
} PathKind;

typedef struct {
  int kind;
  Point p[3];
} PathPiece;

typedef struct {
  Real tolerance;

} Path;

/* For now a path is not thought to be modified:
 * we don't provide functions to change the pieces which have already
 * been added to the path.
 */

void path_init(Path **p);

void path_appent(Path *p, Point *point, int join);

void path_move_to(Path *p, Point *point);

void path_line_to(Path *p, Point *point);

void path_arc_to(Path *p, Point *p1, Point *p2);

/** This function returns the length of the path. */
void path_length(Path *p);

/** This function returns the number of pieces in the path. */
void path_num_pieces(Path *p);

/** This function returns a point in the path, such that the sub-path
 * which connects it to the starting point has the provided 'length'.
 */
void path_get_first_point_at_length(Path *p, Real length);

void path_get_last_point_at_length(Path *p, Real length);

/** This function returns the first point of the provided 'piece'.
 * If 'piece' is not an integer, then a proper interpolation is made.
 * Therefore:
 *  - 0.0 is the first point of the first piece of the path;
 *  - 0.5 is the point in the middle of the first piece of the path;
 *  - 1.0 is the first point of the second piece of the path;
 *  ...
 * NOTE: this function returns the same output returned by the function
 *  path_get_last_point_of_piece if the path is not broken (=closed).
 */
void path_get_first_point_of_piece(Path *p, Real piece);

/** This function returns the last point of the provided 'piece'.
 * If 'piece' is not an integer, then a proper interpolation is made.
 * Therefore:
 *  - 1.0 is the last point of the first piece of the path;
 *  - 1.5 is the point in the middle of the second piece of the path;
 *  - 2.0 is the last point of the second piece of the path;
 *  ...
 * NOTE: this function returns the same output returned by the function
 *  path_get_first_point_of_piece if the path is not broken (=closed).
 */
void path_get_last_point_of_piece(Path *p, Real piece);

void path_get_piece_from_length(Path *p, Real length);

void path_get_length_from_piece(Path *p, Real piece);

void path_subpath(Path *p, Path *subpath, Real first_piece, Real last_piece);

void path_append_reversed(Path *in, Path *out, int join);

void path_append(Path *in, Path *out, int join);

#endif
