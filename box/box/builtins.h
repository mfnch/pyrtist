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

/**
 * @file builtins.h
 * @brief Register built-in types and functions(calling other bltin modules).
 *
 * This file registers the fundamental types of Box, calling also the init
 * functions for other modules, such as bltinstr and bltinio.
 */

#ifndef _BUILTINS_H
#  define _BUILTINS_H

#  include <box/types.h>
#  include <box/compiler.h>

/** Builtin types */
typedef struct {
  BoxVMCallNum
          subtype_init,
          subtype_finish;
} BltinStuff;

BOX_BEGIN_DECLS

void Bltin_Init(BoxCmp *c);

void Bltin_Finish(BoxCmp *c);

/** Add a C procedure 'c_fn' (of type BoxVMFunc) with name 'proc_name' to
 * the VM and returns its symbol ID, so that it can be called.
 * NOTE: the function is not registered, meaning that Box programs do not
 *  "see" the new function. The function can only be called by the compiler
 *  generated code.
 */
BoxVMCallNum Bltin_Proc_Add(BoxCmp *c, const char *proc_name,
                            BoxTask (*c_fn)(BoxVMX *));

/** Add and register a new C procedure 'c_fn' (of type BoxVMFunc) for the
 * combination 'comb' between the types 'left' and 'right'. After this
 * function has returned, Box programs will be able to find the combination
 * and use it!
 */
void Bltin_Comb_Def(BoxType *child, BoxCombType comb,
                    BoxType *parent, BoxTask (*c_fn)(BoxVMX *));

void Bltin_Comb_Def_With_Ids(BoxTypeId child, BoxCombType comb_type,
                             BoxTypeId parent, BoxTask (*c_fn)(BoxVMX *));


void Bltin_Proc_Def(BoxType *parent, BoxType *child,
                    BoxTask (*c_fn)(BoxVMX *));

void Bltin_Proc_Def_With_Id(BoxType *parent, BoxTypeId child_id,
                            BoxTask (*c_fn)(BoxVMX *));

/** Similar to 'Bltin_Comb_Def' but assumes 'comb == BOXCOMBTYPE_AT'.
 * @see Bltin_Comb_Def
 */
void Bltin_Proc_Def_With_Ids(BoxTypeId parent, BoxTypeId child,
                             BoxTask (*c_fn)(BoxVMX *));

/** Define a new intrinsic type with the given name and size. */
BoxType *Bltin_Create_Type(BoxCmp *c, const char *type_name,
                           size_t type_size, size_t alignment);

/** Convenient function to define a new intrinsic type from a given C type. */
#define BLTIN_CREATE_TYPE(c, type_name, type) \
  Bltin_Create_Type((c), (type_name), sizeof(type), __alignof__(type))

BOX_END_DECLS

#endif
