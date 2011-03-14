/****************************************************************************
 * Copyright (C) 2006, 2008 by Matteo Franchin                              *
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

/** @file builtins.h
 * @brief Register built-in types and functions(calling other bltin modules).
 *
 * This file registers the fundamental types of Box, calling also the init
 * functions for other modules, such as bltinstr and bltinio.
 */

#ifndef _BUILTINS_H
#  define _BUILTINS_H

#  include "types.h"
#  include "cmpptrs.h"

/** Builtin types */
typedef struct {
  BoxType string,
          length,
          struc_real_real,
          species_int,
          species_real,
          species_point,
          alias_if,
          alias_else,
          alias_elif,
          alias_for,
          print,
          exit,
          file;
  BoxVMSymID
          subtype_init,
          subtype_finish;
} BltinStuff;

void Bltin_Init(BoxCmp *c);

void Bltin_Finish(BoxCmp *c);

/** Add a C procedure 'c_fn' (of type BoxVMFunc) with name 'proc_name' to
 * the VM and returns its symbol ID, so that it can be called.
 * NOTE: the function is not registered, meaning that Box programs do not
 *  "see" the new function. The function can only be called by the compiler
 *  generated code.
 */
BoxVMSymID Bltin_Proc_Add(BoxCmp *c, const char *proc_name,
                          Task (*c_fn)(BoxVM *));

/** Add and register a new C procedure 'c_fn' (of type BoxVMFunc) for the
 * given 'parent' and 'child' types. After this function has returned,
 * Box programs will be able to find Child@Parent and will be able to call
 * it!
 */
BoxVMSymID Bltin_Proc_Def(BoxCmp *c, BoxType parent, BoxType child,
                          Task (*c_fn)(BoxVM *));

/** Define a new intrinsic type with the given name and size. */
BoxType Bltin_New_Type(BoxCmp *c, const char *type_name, size_t type_size);

#endif
