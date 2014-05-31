/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin                                 *
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

#  include <stdint.h>

#  include <box/types.h>
#  include <box/defaults.h>
#  include <box/array.h>
#  include <box/occupation.h>

#  define NUM_REGISTER_TYPES 5

/** Register number (an alias for integer) */
typedef BoxInt BoxVMRegNum;

/** @brief The state of allocation of the registers in a single function.
 */
typedef struct {
  BoxOcc reg_occ[NUM_TYPES]; /**< Registers occupation */
  uint32_t lvar[NUM_TYPES];  /**< Local variables */
} RegFrame;

/** @brief This structure keeps the full state of the register allocator.
 */
typedef struct {
  RegFrame reg_frame;       /**< Register frame */
  uint32_t gvar[NUM_TYPES]; /**< Global variables */
} RegAlloc;

void Reg_Init(RegAlloc *ra);
void Reg_Finish(RegAlloc *ra);
BoxInt Reg_Occupy(RegAlloc *ra, BoxTypeId t);
void Reg_Release(RegAlloc *ra, BoxInt t, BoxUInt regnum);
BoxInt Var_Occupy(RegAlloc *ra, BoxTypeId type, BoxInt level);
BoxInt GVar_Occupy(RegAlloc *ra, BoxTypeId type);

/**
 * @brief Get the number of local variables and temps for each type.
 * @param ra The register allocator.
 * @param num_temps A pointer to an array of NUM_REGISTER_TYPES values.
 * @param num_vars A pointer to an array of NUM_REGISTER_TYPES values.
 */
void
RegAlloc_Get_Local_Nums(RegAlloc *ra, uint32_t *num_regs, uint32_t *num_vars);

/**
 * @brief Get the number of global variables and temps for each type.
 * Same usage as RegAlloc_Get_Local_Nums().
 */
void
RegAlloc_Get_Global_Nums(RegAlloc *ra, uint32_t *num_temps, uint32_t *num_vars);

#endif /* _BOX_REGISTERS_H */
