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
  BOXVMSYMTYPE_PROC=4
} BoxVMSymType;

typedef struct {
  BoxInt sheet_id, position;
  int conditional;
} VMSymLabel;


/* Temporary, very ugly stuff... */

BoxBool
BoxVMSym_Proc_Is_Implemented(BoxVM *vm, BoxVMSymID sym_id,
                             const char **c_name);

BoxBool
BoxVMSym_Define_Proc(BoxVM *vm, BoxVMSymID sym_id, BoxCCallOld fn_ptr);

BoxBool
BoxVMSym_Reference_Proc(BoxVM *vm, BoxCallable *cb);

#endif
