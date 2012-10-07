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

/**
 * @file registers.h
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

#ifndef _BOX_REGISTERS_H
#  define _BOX_REGISTERS_H

#  include <box/types.h>
#  include <box/defaults.h>
#  include <box/array.h>
#  include <box/occupation.h>

/** Register number (an alias for integer) */
typedef BoxInt BoxVMRegNum;

typedef struct {
  BoxInt    chain; /**< Chain of released registers */
  BoxInt    max;   /**< Total number of registers to allocate for variables */
  BoxArr regs;  /**< Registers corresponding to variables */
} VarFrame;

/** @brief The state of allocation of the registers in a single function.
 */
typedef struct {
  BoxOcc reg_occ[NUM_TYPES]; /**< Registers occupation */
  VarFrame lvar[NUM_TYPES];  /**< Local variables */
} RegFrame;

/** @brief This structure keeps the full state of the register allocator.
 */
typedef struct {
  BoxArr reg_frame;         /**< Array of register frames */
  VarFrame gvar[NUM_TYPES]; /**< Global variables */
} RegAlloc;

void Reg_Init(RegAlloc *ra);
void Reg_Finish(RegAlloc *ra);
void Reg_Frame_Push(RegAlloc *ra);
void Reg_Frame_Pop(RegAlloc *ra);
BoxInt Reg_Frame_Get(RegAlloc *ra);
BoxInt Reg_Occupy(RegAlloc *ra, BoxTypeId t);
void Reg_Release(RegAlloc *ra, BoxInt t, BoxUInt regnum);
BoxInt Reg_Num(RegAlloc *ra, BoxInt t);
BoxInt Var_Occupy(RegAlloc *ra, BoxTypeId type, BoxInt level);
void Var_Release(RegAlloc *ra, BoxInt type, BoxUInt varnum);
BoxInt Var_Num(RegAlloc *ra, BoxInt type);
BoxInt GVar_Occupy(RegAlloc *ra, BoxTypeId type);
void GVar_Release(RegAlloc *ra, BoxInt type, BoxUInt varnum);
BoxInt GReg_Num(RegAlloc *ra, BoxInt type);
BoxInt GVar_Num(RegAlloc *ra, BoxInt type);
void Reg_Get_Local_Nums(RegAlloc *ra, BoxInt *num_regs, BoxInt *num_vars);
void Reg_Get_Global_Nums(RegAlloc *ra, BoxInt *num_regs, BoxInt *num_vars);

#endif /* _BOX_REGISTERS_H */
