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
#  include "virtmach.h"

/* These types for now are just duplicates of the types defined in virtmach.h
 * This has to be seen as a first attempt of create an interface for code
 * generation which is unbound to the target machine, even if for now we
 * support as a target only the box vm.
 */
typedef enum {
  CONT_GREG = CAT_GREG,
  CONT_LREG = CAT_LREG,
  CONT_PTR = CAT_PTR,
  CONT_IMM = CAT_IMM
} ContCateg;

typedef enum {
  CONT_CHAR  = TYPE_CHAR,
  CONT_INT   = TYPE_INT,
  CONT_REAL  = TYPE_REAL,
  CONT_POINT = TYPE_POINT,
  CONT_OBJ   = TYPE_OBJ
} ContType;

typedef struct {
  ContCateg  categ;
  ContType   type;
  Int        reg;
  Int        ptr_reg;
  void       *extra;
  struct {
    int ptr_is_greg : 1;
  }          flags;
} Cont;

/*
#define CONT_NEW_LREG(type, num) (& ((Cont) {CONT_LREG, (type), & (Int) (num)}))
#define CONT_LREG(type, num) (& ((Cont) {CONT_LREG, (type), & (Int) (num)}))
*/

#endif
