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
 * This file provides the functions needed to associate registers
 * to expressions, guaranting that occupied registers will never be used
 * for other new expressions, unless they have been first released
 * with the appropriate function.
 * The system keeps track of registers indipendently for different
 * pieces of code. For this purpose the concept of frame is used.
 * Registers in different frames are completely independent. This means
 * that there is a different occupation list for each different frame.
 * Frames can be created with the function Reg_Frame_Push and destroyed
 * with Reg_Frame_Pop. Registers with different types are also independent.
 * This means that there is a different occupation list for each different
 * type of register. There are NUM_TYPES kinds of registers.
 * NOTE: In all the functions defined here, when the given type exceed
 * the maximum value, the default type TYPE_OBJ is considered.
 */

#ifndef _REGISTERS_H
#  define _REGISTERS_H

#  include "types.h"
#  include "array.h"
#  include "collection.h"

typedef struct {
  Int max; /**< Total number of registers to allocate for variables */
  Array *occ; /**< Variables occcupations */
} VarFrame;

/** @brief The state of allocation of the registers in a single function.
 */
typedef struct {
  Collection *reg_occ[NUM_TYPES]; /**< Registers occupation */
  VarFrame lvar[NUM_TYPES]; /**< Local variables */
} RegFrame;

/** @brief This structure keeps the full state of the register allocator.
 */
typedef struct {
  Array *reg_frame; /**< Array of register frames */
  VarFrame gvar[NUM_TYPES]; /**< Global variables */
} RegAlloc;

Task Reg_Init(void);
void Reg_Destroy(void);
void Reg_Frame_Push(void);
Task Reg_Frame_Pop(void);
Int Reg_Frame_Get(void);
Int Reg_Occupy(Int t);
Task Reg_Release(Int t, UInt regnum);
Int Reg_Num(Int t);
Task Var_Init(void);
Int Var_Occupy(Int type, Int level);
Task Var_Release(Int type, UInt varnum);
Int Var_Num(Int type);
Int GVar_Occupy(Int type);
Task GVar_Release(Int type, UInt varnum);
Int GVar_Num(Int type);
void RegLVar_Get_Nums(Int *num_reg, Int *num_lvar);
#endif
