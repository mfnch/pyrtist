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

#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "gpath.h"
#include "buffer.h"
#include "g.h"

GPath *gpath_init(void) {
  GPath *gp = (GPath *) malloc(sizeof(GPath));
  if (gp == (GPath *) NULL) return ((GPath *) NULL);
  if (!buff_create(& gp->pieces, sizeof(GPathPiece), 10)) {
    free(gp);
    return (GPath *) NULL;
  }
  gp->have.position = 0;
  return gp;
}

void gpath_destroy(GPath *gp) {
  buff_free(& gp->pieces);
  free(gp);
}

void gpath_clear(GPath *gp) {
  buff_clear(& gp->pieces);
  gp->have.position = 0;
}

void gpath_break(GPath *gp) {
  gp->have.position = 0;
}

Point *gpathpiece_last_point(GPathPiece *piece) {
  switch(piece->kind) {
  case GPATHKIND_LINE: return & (piece->p[1]);
  case GPATHKIND_ARC: return & (piece->p[2]);
  default: g_error("gpathpiece_last_point: shouldn't happen: damaged path?");
  }
  return (Point *) NULL;
}

void gpathpiece_mirror(GPathPiece *piece) {
  Point p = piece->p[0];
  switch(piece->kind) {
  case GPATHKIND_LINE:
    piece->p[0] = piece->p[1]; piece->p[1] = p; break;
  case GPATHKIND_ARC:
    piece->p[0] = piece->p[2]; piece->p[2] = p; break;
  default: g_error("gpathpiece_mirror: shouldn't happen: damaged path?");
  }
}

void gpath_close(GPath *gp) {
  if (!gp->have.position)
    return;

  else {
    GPathPiece *piece = buff_firstitemptr(& gp->pieces, GPathPiece);
    gpath_line_to(gp, & (piece->p[0]));
  }
}

void gpath_append(GPath *gp, Point *point, int join) {
  if (join && gp->have.position) {
    GPathPiece piece;
    piece.kind = GPATHKIND_LINE;
    piece.p[0] = gp->position;
    piece.p[1] = *point;
    gp->position = *point;
    (void) buff_push(& gp->pieces, & piece);

  } else {
    gp->position = *point;
    gp->have.position = 1;
  }
}

void gpath_move_to(GPath *gp, Point *point) {
  return gpath_append(gp, point, 0);
}

void gpath_line_to(GPath *gp, Point *point) {
  return gpath_append(gp, point, 1);
}

void gpath_arc_to(GPath *gp, Point *p1, Point *p2) {
  GPathPiece piece;
  if (!gp->have.position) {
    gpath_move_to(gp, p1);
    gpath_line_to(gp, p2);
    return;
  }

  piece.kind = GPATHKIND_ARC;
  piece.p[0] = gp->position;
  piece.p[1] = *p1;
  piece.p[2] = *p2;
  gp->position = *p2;
  (void) buff_push(& gp->pieces, & piece);
}

int gpath_iter(GPath *gp, GPathIterator iter, void *data) {
  Int i, n = buff_numitems(& gp->pieces);
  GPathPiece *piece = buff_firstitemptr(& gp->pieces, GPathPiece);
  for(i=1; i<=n; i++) {
    int retval = iter(i, piece, data);
    if (retval != 0) return retval;
    ++piece;
  }
  return 0;
}

static int gpath_print_iterator(Int i, GPathPiece *p, void *data) {
  FILE *out = (FILE *) data;
  switch(p->kind) {
  case GPATHKIND_LINE:
    fprintf(out, "piece n. "SInt", kind=line, ("SReal", "SReal
            ") -> ("SReal", "SReal")\n", i, p->p[0].x, p->p[0].y,
            p->p[1].x, p->p[1].y);
    break;
  case GPATHKIND_ARC:
    fprintf(out, "piece n. "SInt", kind=arc, ("SReal", "SReal
            ") -> ("SReal", "SReal") -> ("SReal", "SReal")\n",
            i, p->p[0].x, p->p[0].y, p->p[1].x, p->p[1].y,
            p->p[2].x, p->p[2].y);
    break;
  default:
    fprintf(out, "piece n. "SInt", kind=unknown: damaged?\n", i);
    break;
  }
  return 0;
}

void gpath_print(GPath *gp, FILE *out) {
  (void) gpath_iter(gp, gpath_print_iterator, out);
}

static int gpath_print_points_iterator(Int i, GPathPiece *p, void *data) {
  FILE *out = (FILE *) data;
  Int j, n = 0;
  switch(p->kind) {
  case GPATHKIND_LINE: n = 2; break;
  case GPATHKIND_ARC: n = 3; break;
  default:
    fprintf(out, "piece n. "SInt", kind=unknown: damaged?\n", i);
    break;
  }
  for(j=0; j<n; j++)
    fprintf(out, SReal" "SReal"\n", p->p[j].x, p->p[j].y);
    fprintf(out, "\n");
  return 0;
}

void gpath_print_points(GPath *gp, FILE *out) {
  (void) gpath_iter(gp, gpath_print_points_iterator, out);
}

Real gpath_length(GPath *gp) {return 0.0;}

Int gpath_num_pieces(GPath *gp) {
  return buff_numitems(& gp->pieces);
}

void gpath_get_first_point_at_length(GPath *p, Real length) {}

void gpath_get_last_point_at_length(GPath *p, Real length) {}

void gpath_get_first_point_of_piece(GPath *p, Real piece) {}

void gpath_get_last_point_of_piece(GPath *p, Real piece) {}

void gpath_get_piece_from_length(GPath *p, Real length) {}

void gpath_get_length_from_piece(GPath *p, Real piece) {}

void gpath_subgpath(GPath *p, GPath *subpath, Real first_piece, Real last_piece) {}

void gpath_append_gpath(GPath *dest, GPath *src, int options) {
  Int n = buff_numitems(& src->pieces);
  if (n > 0) {
    if ((options & GPATH_INVERT) == 0) {
      GPathPiece *piece = buff_firstitemptr(& src->pieces, GPathPiece);
      if ((options & GPATH_JOIN) != 0)
        gpath_line_to(dest, & (piece->p[0]));

      (void) buff_mpush(& dest->pieces, piece, n);
      piece = buff_lastitemptr(& src->pieces, GPathPiece);
      dest->position = *gpathpiece_last_point(piece);

    } else {
      GPathPiece *piece = buff_lastitemptr(& src->pieces, GPathPiece);
      Int i;
      if ((options & GPATH_JOIN) != 0)
        gpath_line_to(dest, gpathpiece_last_point(piece));

      for(i=0; i<n; i++) {
        GPathPiece mirror_piece = *piece;
        gpathpiece_mirror(& mirror_piece);
        (void) buff_push(& dest->pieces, & mirror_piece);
        --piece;
      }

      piece = buff_firstitemptr(& src->pieces, GPathPiece);
      dest->position = piece->p[0];
    }
  }
  if ((options & GPATH_CLOSE) != 0) gpath_close(dest);
}
