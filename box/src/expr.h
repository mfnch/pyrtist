/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin                                 *
 *   fnch@libero.it                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* $Id$ */

/** @file expr.h
 * @brief The functions to handle declarations and instantiations of boxes.
 *
 * Some words.
 */

#ifndef _EXPR_H
#  define _EXPR_H

#  include "virtmach.h"

typedef struct {
  struct {
    unsigned int imm     : 1; /* l'espressione e' immediata? */
    unsigned int value   : 1; /* Possiede un valore determinato? */
    unsigned int typed   : 1; /* Possiede tipo? */
    unsigned int ignore  : 1; /* Va ignorata o passata alla box? */
    unsigned int target  : 1; /* si puo' assegnare un valore all'espressione?*/
    unsigned int gaddr   : 1; /* addr e' un registro globale (o locale)? */
    unsigned int allocd  : 1; /* l'oggetto e' stato allocato? (va liberato?) */
    unsigned int release : 1; /* il registro va rilasciato automaticamente? */
  } is;

  Intg    addr;
  Intg    type;     /* The type of the expression */
  Intg    resolved; /* = Tym_Type_Resolve_All(type) */
  AsmArg  categ;
  union {
    Intg  i;
    Real  r;
    Point p;
    Intg  reg;
    Name  nm;
  } value;

} Expr;

/** For backward compatibility */
#define Expression Expr

/** Create a new type-expression */
void Expr_New_Type(Expr *e, Int type);

/** Create a new Void expression */
void Expr_New_Void(Expr *e);

/** Print the content of the given expression */
void Expr_Print(Expr *e, FILE *out);

#endif
