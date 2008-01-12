/***************************************************************************
 *   Copyright (C) 2008 by Matteo Franchin                                 *
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

/** @file container.h
 * @brief Generate the assembly code for expression manipulation.
 *
 * Some words.
 */

#ifndef _CONTAINER_H
#  define _CONTAINER_H

#  include "types.h"

typedef enum {
  CONT_IMM=0,
  CONT_LOC,
  CONT_GLOB,
  CONT_PTR
} ContCateg;

typedef enum {
  CONT_INT=1,
  CONT_REAL,
  CONT_POINT,
  CONT_OBJ
} ContType;

typedef struct {
  ContCateg categ;
  ContType type;
  union {
    Int i;
    Real r;
    Point p;
  } data;
} Cont;


/*
type, categ, data

type = (INT, REAL, POINT, OBJ)
categ = (CAT_IMM, CAT_LREG, CAT_GREG, CAT_PTR)
data = union(INT, REAL, POINT, (INT, INT))
*/
#endif
