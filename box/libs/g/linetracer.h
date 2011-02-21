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

#ifndef _LINETRACER_H
#  define _LINETRACER_H

#  include "types.h"
#  include "g.h"
#  include "gpath.h"

typedef struct {
  BoxReal ti, te, /**< Internal, external joining lengths for this segment */
          ni, ne; /**< Internal, external joining lengths for next segment */
} LineJoinStyle;

typedef struct {
  BoxReal       width1, width2;
  BoxPoint      point;
  LineJoinStyle style;
  void          *arrow;
  BoxReal       arrow_scale;
} LinePiece;

/* Questa struttura tiene conto di cio' che si sta tracciando */
typedef struct {
  BoxPoint pnt1, pnt2; /* Vertici dell'asse della linea */
  BoxReal sp1, sp2;    /* Spessori in corrispondenza dei due vertici */
  BoxPoint v,          /* Vettori dell'asse della linea */
           u,          /* Vettore v normalizzato */
           o,          /* Versore ortogonale al vettore dell'asse */
           vb[2],      /* Vettori dei due lati della linea */
           p[2],       /* I due punti laterali della linea */
           cong[2],    /* Punti di congiuntura fra i lati delle linee */
           vci,        /* Vettore di congiuntura interno */
           vertex[4];  /* Vertici della linea precedente */
  BoxReal mod, mod2;   /* Lunghezza della linea */
} LineDesc;

typedef struct {
  LineJoinStyle join_style;
  LineDesc firstline, line1, line2, *thsl, *nxtl;
  int is_closed, segment;
  Real cutting;
  GPath *border[2];
  buff pieces;
} LineTracer;

enum {
  LT_CLOSE=1
};

LineTracer *lt_new(void);

void lt_destroy(LineTracer *lt);

void lt_add_piece(LineTracer *lt, LinePiece *lp);

Int lt_num_pieces(LineTracer *lt);

void lt_clear(LineTracer *lt);

int lt_draw(LineTracer *lt, int closed);

void lt_join_style_from_array(LineJoinStyle *ljs,
                              Real ti, Real te, Real ni, Real ne);

void lt_join_style_set(LineTracer *lt, LineJoinStyle *ljs);

#endif

