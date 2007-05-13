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

/** @file registers.h
 * @brief The register allocation system.
 *
 * Some words.
 */

#ifndef _REGISTERS_H
#  define _REGISTERS_H

#  include "types.h"
#  include "array.h"
#  include "collection.h"

/** This structure describe the state of allocation of the register
 * in a single function.
 */
typedef struct {
  Collection *reg_occ[NUM_TYPES];
  Int var_max[NUM_TYPES];
  Array *var_occ[NUM_TYPES];
} RegFrame;

/** This structure keeps the full state of the register allocator.
 */
typedef struct {
  Array *reg_frame;
} RegAlloc;

Task Reg_Init(void);
void Reg_Destroy(void);
Int Reg_Occupy(Int t);
Task Reg_Release(Int t, UInt regnum);
Int Reg_Num(Int t);
Task Var_Init(void);
Int Var_Occupy(Int type, Int level);
Task Var_Release(Int type, UInt varnum);
Int Var_Num(Int type);
void RegVar_Get_Nums(Int *num_var, Int *num_reg);
#endif
