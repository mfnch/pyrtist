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

/** @file vmsym.h
 * @brief The implementation of the reference/definition list for the box VM.
 *
 * This file implements the mechanism to define and reference procedures,
 * labels and other symbols needed by a Box bytecode compiled program.
 * This mechanism is similar to the linking of the object files produced
 * by the compilation of several C-source files. In this sense whe could
 * say that this file implements the Box linker.
 */

#ifndef _VMSYM_H_TYPES
#  define _VMSYM_H_TYPES

#  include "types.h"
#  include "array.h"
#  include "hashtable.h"
#  include "vmcommon.h"

/** @brief The table of reference and definition for the Box VM.
 *
 * This structure is embedded in the main VM structure VMProgram.
 */
typedef struct {
  Hashtable *syms;
  Array *data;
  Array *defs;
  Array *refs;
} VMSymTable;

/** This is the prototype for a function which generates a piece of code
 * which makes a reference to a symbol.
 * Functions of this type are called when a reference is made to a symbol
 * ('resolving=0') or when the reference is being resolved ('resolving=1').
 * If the symbol has already been defined, then 'defined=1', otherwise
 * 'defined=0'. 'def' and 'def_size'are respectively the pointer
 * and the size of the definition data (which are allocated when the symbol
 * is created with VM_sym_New).
 */
typedef Task (*VMSymResolver)(VMProgram *vmp, UInt sym_num, UInt sym_type,
 int defined, void *def, UInt def_size, void *ref, UInt ref_size);

/** @brief The details about a symbol.
 */
typedef struct {
  Name name; /**< The name of the symbol */
  int defined; /**< Has a definition been given for the symbol? */
  UInt def_size; /**< Size of the data containing the symbol definition */
  UInt def_addr; /**< Address of the data in the array 'data' */
  UInt sym_type; /**< Type of the symbol */
  UInt first_ref; /**< Number of the first reference to the symbol */
  VMSymResolver resolver; /**< Procedure to invoke for resolution */
} VMSym;

/** @brief A reference...
 */
typedef struct {
  UInt sym_num; /**< ID-number of the referenced symbol */
  UInt next; /**< ID-number of the next reference to the same symbol */
  UInt ref_size; /**< Size of the data containing the symbol reference */
  UInt ref_addr;  /**< Address of the data in the array 'data' */
  int resolved; /**< Has the reference been resolved? */
} VMSymRef;

/* Data types used by the VM_Sym_Code_* functions */
typedef Task (*VMSymCodeGen)(VMProgram *vmp, UInt sym_num, UInt sym_type,
 int defined, void *def, UInt def_size, void *ref, UInt ref_size);

typedef struct {
  UInt proc_num;
  UInt pos;
  UInt size;
  VMSymCodeGen code_gen;
} VMSymCodeRef;

#endif

#ifndef _VMSYM_H
#  ifndef _INSIDE_VIRTMACH_H
#    define _VMSYM_H

/** Initialize the symbol table of the program.
 * @param vmp is the VM-program.
 */
Task VM_Sym_Init(VMProgram *vmp);

/** Destroy the symbol table of the program.
 * @param vmp is the VM-program.
 */
void VM_Sym_Destroy(VMProgram *vmp);

/** Create a new symbol with name 'n' and type 'sym_type'.
 * '*sym_num' is set with the allocated symbol number.
 */
Task VM_Sym_New(VMProgram *vmp, UInt *sym_num, UInt sym_type, UInt def_size);

/** Associate a name to the symbol sym_num.
 */
Task VM_Sym_Name_Set(VMProgram *vmp, UInt sym_num, Name *n);

/** Rename a symbol, given its old name.
 */
Task VM_Sym_Rename_From_Old(VMProgram *vmp, Name *old_name, Name *new_name);

/** Rename a symbol, given its ID number.
 */
Task VM_Sym_Rename(VMProgram *vmp, UInt sym_num, Name *new_name);

/** Define a symbol which was previously created with VM_sym_New */
Task VM_Sym_Def(VMProgram *vmp, UInt sym_num, void *def);

/** Add a reference to the symbol 'sym_num' */
Task VM_Sym_Ref(VMProgram *vmp, UInt sym_num, void *ref, UInt ref_size);

/** Set the symbol resolver */
Task VM_Sym_Resolver_Set(VMProgram *vmp, UInt sym_num, VMSymResolver r);

/** Resolve the symbol 'sym_num'.
 * If sym_num=0, then try to resolve all the symbols.
 */
Task VM_Sym_Resolve(VMProgram *vmp, UInt sym_num);

/** Print the symbol table for the given symbol. The symbol table is a list
 * of all the references made to the symbol. If sym_num is zero the references
 * to all the symbols will be printed. out is the destination.
 */
void VM_Sym_Table_Print(VMProgram *vmp, FILE *out, UInt sym_num);

const char *VM_Sym_Name_Get(VMProgram *vmp, UInt sym_num);

/** Check that the type of the symbol 'sym_num' is 'sym_type'. */
Task VM_Sym_Check_Type(VMProgram *vmp, UInt sym_num, UInt sym_type);

/** This function calls the function given as argument 'VM_Sym_Code_Ref'
 * to assemble a piece of VM-code which makes reference to the symbol
 * 'sym_num'. The reference will be resolved calling again 'code_gen'
 * once the symbol has been defined.
 * The function 'code_gen' assembles parametrically a piece of VM byte-code.
 */
Task VM_Sym_Code_Ref(VMProgram *vmp, UInt sym_num, VMSymCodeGen code_gen);

#  define VM_Sym_Code_New VM_Sym_New
#  define VM_Sym_Code_Def VM_Sym_Def
#  define VM_Sym_Resolve_All(vmp) VM_Sym_Resolve(vmp, 0)
#  endif
#endif
