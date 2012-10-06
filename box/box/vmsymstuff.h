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

/**
 * @file vmsymstuff.h
 * @brief Wrappings of 'vmsym.c' for defining and referencing symbols.
 *
 * Some words.
 */

#ifndef _VMSYMSTUFF_H
#  define _VMSYMSTUFF_H

typedef enum {
  BOXVMSYMTYPE_CALL=1,
  BOXVMSYMTYPE_COND_JMP=2,
  BOXVMSYMTYPE_PROC_HEAD=123,
  BOXVMSYMTYPE_LABEL=3,
  BOXVMSYMTYPE_CALLABLE=4,
  BOXVMSYMTYPE_PROC=5
} BoxVMSymType;

typedef struct {
  Int sheet_id, position;
  int conditional;
} VMSymLabel;


void VM_Sym_Alloc_Method_Register(BoxVM *vmp, UInt sym_num,
                                  BoxTypeId type, BoxTypeId method);

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


/* Temporary, very ugly stuff... */

BoxBool
BoxVMSym_Define_Proc(BoxVM *vm, BoxVMSymID sym_id, BoxCCallOld fn_ptr);

BoxBool
BoxVMSym_Reference_Proc(BoxVM *vm, BoxVMCallNum cn, const char *name);

#endif

