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

/* registers.h - agosto 2004
 *
 * Questo file contiene le dichiarazioni delle procedure definite in register.c
 */

#define REG_OCC_TYP_SIZE {10, 10, 10, 10, 10}
#define VAR_OCC_TYP_SIZE {10, 10, 10, 10, 10}

Task Reg_Init(UInt typical_num_reg[NUM_TYPES]);
Intg Reg_Occupy(Intg t);
Task Reg_Release(Intg t, UInt regnum);
Intg Reg_Num(Intg t);
Task Var_Init(UInt typical_num_var[NUM_TYPES]);
Intg Var_Occupy(Intg type, Intg level);
Task Var_Release(Intg type, UInt varnum);
Intg Var_Num(Intg type);
void RegVar_Get_Nums(Intg *num_var, Intg *num_reg);
