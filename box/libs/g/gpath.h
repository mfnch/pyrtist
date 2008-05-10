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

#  include <stdio.h>

#  include "types.h"
#  include "buffer.h"

typedef enum {
  GPATHKIND_LINE,
  GPATHKIND_ARC
} GPathKind;

typedef struct {
  int kind;
  Point p[3];
} GPathPiece;

typedef struct {
  struct {
    int position : 1;
  } have;
  Real tolerance;
  Point position;
  buff pieces;
} GPath;

enum {
  GPATH_JOIN=1,
  GPATH_INVERT=2,
  GPATH_CLOSE=4
};

/** Used in conjunction with gpath_iter() */
typedef int GPathIterator(Int piece_index, GPathPiece *piece, void *data);

/* For now a GPath is not thought to be modified:
 * we don't provide functions to change the pieces which have already
 * been added to the GPath.
 */

/** Return the pointer to a new allocated empty GPath object. */
GPath *gpath_init(void);

/** Destroy the given GPath. */
void gpath_destroy(GPath *gp);

/** Clear the given GPath. */
void gpath_clear(GPath *gp);

/** Break the path = forgets the current position */
void gpath_break(GPath *gp);

/** Return the last point in the piece */
Point *gpathpiece_last(GPathPiece *piece);

void gpath_append(GPath *gp, Point *point, int join);

void gpath_move_to(GPath *gp, Point *point);

void gpath_line_to(GPath *gp, Point *point);

void gpath_arc_to(GPath *gp, Point *p1, Point *p2);

/** Calls the function iter() for each piece of the GPath,
 * passing the piece index, a pointer to the piece data structure
 * and the user provided pointer 'data'. The index starts from 1
 * at the first piece.
 * The iteration stops if the function iter() returns a value != 0.
 * If this is the case then this value is returned by 'gpath_iter'.
 * See the documentation of 'GPathIterator' for more details.
 */
int gpath_iter(GPath *gp, GPathIterator iter, void *data);

/** Prints the content of the provided GPath to the 'out' stream. */
void gpath_print(GPath *gp, FILE *out);

/** Prints the points of the provided GPath to the 'out' stream. */
void gpath_print_points(GPath *gp, FILE *out);

/** This function returns the length of the path. */
Real gpath_length(GPath *gp);

/** This function returns the number of pieces in the path. */
Int gpath_num_pieces(GPath *gp);

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

void gpath_append_gpath(GPath *dest, GPath *src, int options);

#endif
