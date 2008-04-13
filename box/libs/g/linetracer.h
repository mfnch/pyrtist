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
  Real width1, width2;
  Point point;
  LineStyle style;
  void *arrow;
  Real arrow_scale;
} LinePiece;

typedef struct {
  /* Lunghezze di giunzione interne ed esterne della linea corrente */
  double ti, te;
  /* Lunghezze di giunzione interne ed esterne della prossima linea */
  double ni, ne;
} LineJoinStyle;

/* Questa struttura tiene conto di cio' che si sta tracciando */
typedef struct {
  Point pnt1, pnt2; /* Vertici dell'asse della linea */
  Real sp1, sp2;    /* Spessori in corrispondenza dei due vertici */
  Point v;          /* Vettori dell'asse della linea */
  Point u;          /* Vettore v normalizzato */
  Point o;          /* Versore ortogonale al vettore dell'asse */
  Point vb[2];      /* Vettori dei due lati della linea */
  Point p[2];       /* I due punti laterali della linea */
  Point cong[2];    /* Punti di congiuntura fra i lati delle linee */
  Point vci;        /* Vettore di congiuntura interno */
  Point vertex[4];  /* Vertici della linea precedente */
  Real mod, mod2;   /* Lunghezza della linea */
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

void lt_join_style(LineTracer *lt, Real *userjs);

#endif
