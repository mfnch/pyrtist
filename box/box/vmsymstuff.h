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
  int conditional;
} VMSymLabel;

/** Return the symbol associated with a new procedure for the given VM.
 * This symbol will be used to emit all the calls to that procedure.
 */
BoxVMSymID BoxVMSym_New_Call(BoxVM *vm, BoxVMCallNum call_num);

/** Define the call number relative to the procedure whose symbol id is
 * 'sym_id'. You do not need to define the call number before calling
 * a procedure: the bytecode will be adjusted properly when the call number
 * will be known.
 */
void BoxVMSym_Def_Call(BoxVM *vm, BoxVMSymID sym_id);

/** Get the call number associated to the given symbold id. */
BoxVMCallNum BoxVMSym_Get_Call_Num(BoxVM *vm, BoxVMSymID sym_id);

/** Emit a new call to the procedure corresponding to the symbol 'sym_id'.
 * If the call number is not know by the time this function is called, a
 * "call 0" will be emitted. This instruction will then be adjusted when
 * the call number will be defined using 'BoxVMSym_Def_Call'.
 */
void BoxVMSym_Assemble_Call(BoxVM *vm, BoxVMSymID sym_id);

void VM_Sym_Alloc_Method_Register(BoxVM *vmp, UInt sym_num,
                                  BoxType type, BoxType method);

/** This function creates an undefined label. A label is a number which
 * refers to a position in the assembled code.
 */
BoxVMSymID BoxVMSym_New_Label(BoxVM *vmp);

/** Same as VM_Sym_New_Label, but sheet_id is the current active sheet and
 * position is the current position in that sheet.
 */
Task BoxVMSym_New_Label_Here(BoxVM *vmp, BoxVMSymID *label_sym_num);

/** Specify the position of a undefined label. */
Task BoxVMSym_Def_Label(BoxVM *vmp, UInt label_sym_num,
                        Int sheet_id, Int position);

/** Same as VM_Sym_Def_Label, but sheet_id is the current active sheet and
 * position is the current position in that sheet.
 */
Task BoxVMSym_Def_Label_Here(BoxVM *vmp, BoxVMSymID label_sym_id);

Task BoxVMSym_Jc(BoxVM *vm, BoxVMSymID sym_id);

Task BoxVMSym_Jmp(BoxVM *vm, BoxVMSymID sym_id);

/** Called to signal that a label is not needed anymore. */
Task BoxVMSym_Release_Label(BoxVM *vmp, UInt sym_num);

Task BoxVMSym_Assemble_Proc_Head(BoxVM *vm, BoxVMSymID *sym_id);

Task BoxVMSym_Def_Proc_Head(BoxVM *vmp, BoxVMSymID sym_id,
                            Int *num_var, Int *num_reg);

#endif

