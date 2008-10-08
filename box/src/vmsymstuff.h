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

/* $Id$ */

/** @file vmsymstuff.h
 * @brief Wrappings of 'vmsym.c' for defining and referencing symbols.
 *
 * Some words.
 */

#ifndef _VMSYMSTUFF_H
#  define _VMSYMSTUFF_H

#  define VM_SYM_CALL 1
#  define VM_SYM_COND_JMP 2
#  define VM_SYM_PROC_HEAD 123
#  define VM_SYM_LABEL 3

typedef struct {
  Int sheet_id, position;
} VMSymLabel;

Task VM_Sym_New_Call(VMProgram *vmp, UInt *sym_num);

Task VM_Sym_Def_Call(VMProgram *vmp, UInt sym_num, UInt proc_num);

Task VM_Sym_Call(VMProgram *vmp, UInt sym_num);

Task VM_Sym_Alloc_Method_Register(VMProgram *vmp, UInt sym_num,
                                  Type type, Type method);

Task VM_Sym_New_Cond_Jmp(VMProgram *vmp, UInt *sym_num);

Task VM_Sym_Def_Cond_Jmp(VMProgram *vmp, UInt sym_num,
                         Int sheet_id, Int position);

Task VM_Sym_Cond_Jmp(VMProgram *vmp, UInt sym_num);

Task VM_Sym_Proc_Head(VMProgram *vmp, UInt *sym_num);

Task VM_Sym_Def_Proc_Head(VMProgram *vmp, UInt sym_num,
 Int *num_var, Int *num_reg);

#endif
