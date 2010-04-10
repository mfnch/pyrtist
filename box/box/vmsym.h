/****************************************************************************
 * Copyright (C) 2008, 2009 by Matteo Franchin                                    *
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

/** @file vmsym.h
 * @brief The implementation of the reference/definition list for the box VM.
 *
 * This file implements the mechanism to define and reference procedures,
 * labels and other symbols needed by a Box bytecode compiled program.
 * This mechanism is similar to the linking of the object files produced
 * by the compilation of several C-source files. In this sense whe could
 * say that this file implements the Box linker.
 */

#ifndef _BOX_VMSYM_H_TYPES
#  define _BOX_VMSYM_H_TYPES

#  include <box/types.h>
#  include <box/array.h>
#  include <box/hashtable.h>
#  include <box/list.h>

/* We cannot include "virtmach.h", since the structure VMProgram contains
 * VMSymTable. For this reason, it is "virtmach.h" which actually includes
 * "vmsym.h". We therefore declare the VMProgram structure and define a macro
 * which we will undef later.
 */
struct __vmprogram;
#  define BoxVM struct __vmprogram

/** A symbol ID is just an integer number. The VM knows to what symbol
 * this number refers to.
 */
typedef BoxUInt BoxVMSymID;

/** @brief The table of reference and definition for the Box VM.
 *
 * This structure is embedded in the main VM structure VMProgram.
 */
typedef struct {
  BoxHT  syms;
  BoxArr data,
         defs,
         refs,
         dylibs;
} BoxVMSymTable;

/** This is the prototype for a function which generates a piece of code
 * which makes a reference to a symbol.
 * Functions of this type are called when a reference is made to a symbol
 * ('resolving=0') or when the reference is being resolved ('resolving=1').
 * If the symbol has already been defined, then 'defined=1', otherwise
 * 'defined=0'. 'def' and 'def_size'are respectively the pointer
 * and the size of the definition data (which are allocated when the symbol
 * is created with BoxVMSym_New).
 */
typedef BoxTask (*BoxVMSymResolver)(BoxVM *vm, BoxVMSymID sym_num,
                                    BoxUInt sym_type, int defined,
                                    void *def, BoxUInt def_size,
                                    void *ref, BoxUInt ref_size);

/** @brief The details about a symbol.
 */
typedef struct {
  BoxVMSymID
          sym_num;   /**< The symbol ID number */
  BoxName name;      /**< The name of the symbol */
  int     defined;   /**< Has a definition been given for the symbol? */
  BoxUInt def_size,  /**< Size of the data containing the symbol definition */
          def_addr,  /**< Address of the data in the array 'data' */
          sym_type,  /**< Type of the symbol */
          first_ref; /**< Number of the first reference to the symbol */
} BoxVMSym;

/** @brief A reference...
 */
typedef struct {
  BoxVMSymID
          sym_num,  /**< ID-number of the referenced symbol */
          next;     /**< ID-number of the next reference to the same symbol */
  BoxUInt ref_size, /**< Size of the data containing the symbol reference */
          ref_addr; /**< Address of the data in the array 'data' */
  int     resolved; /**< Has the reference been resolved? */
  BoxVMSymResolver
          resolver; /**< Function to be called for resolution */
} BoxVMSymRef;

/* Data types used by the BoxVMSym_Code_* functions */
typedef BoxTask (*BoxVMSymCodeGen)(BoxVM *vm, BoxVMSymID sym_num,
                                   BoxUInt sym_type, int defined,
                                   void *def, BoxUInt def_size,
                                   void *ref, BoxUInt ref_size);

typedef struct {
  BoxUInt proc_num,
          pos,
          size;
  BoxVMSymCodeGen
          code_gen;
} BoxVMSymCodeRef;

#  undef BoxVM

#endif

#ifndef _BOX_VMSYM_H
#  ifndef _INSIDE_VIRTMACH_H
#    define _BOX_VMSYM_H

/** Initialize the symbol table of the program.
 * @param vmp is the VM-program.
 */
void BoxVMSymTable_Init(BoxVMSymTable *t);

/** Destroy the symbol table of the program.
 * @param vmp is the VM-program.
 */
void BoxVMSymTable_Finish(BoxVMSymTable *t);

/** Create a new symbol with name 'n' and type 'sym_type'.
 * '*sym_num' is set with the allocated symbol number.
 */
BoxVMSymID BoxVMSym_New(BoxVM *vmp, BoxUInt sym_type, BoxUInt def_size);

/** Associate a name to the symbol sym_num.
 */
void BoxVMSym_Set_Name(BoxVM *vm, BoxVMSymID sym_id, const char *name);

/** Get the name of the given symbol sym_id (NULL if the symbol has not
 * a name)
 */
const char *BoxVMSym_Get_Name(BoxVM *vm, BoxVMSymID sym_id);

/** Define a symbol which was previously created with BoxVMSym_New */
Task BoxVMSym_Define(BoxVM *vm, BoxVMSymID sym_num, void *def);

/** If the symbol is defined, return the pointer to the definition data,
 * otherwise, if the symbol is not defined, return NULL.
 */
void *BoxVMSym_Get_Definition(BoxVM *vm, BoxVMSymID sym_id);

/** Return whether the symbol 'sym_id' has been defined or not */
int BoxVMSym_Is_Defined(BoxVM *vm, BoxVMSymID sym_id);

typedef enum {
  BOXVMSYM_AUTO,
  BOXVMSYM_RESOLVED,
  BOXVMSYM_UNRESOLVED,
} BoxVMSymStatus;

/** Add a reference to the symbol 'sym_num'. The reference data is pointed
 * by 'ref' and has size 'ref_size'. 'r' is the function to be called for
 * resolving the symbol. If 'resolved' == BOXVMSYM_RESOLVED, the reference
 * is marked as resolved, if 'resolved' == BOXVMSYM_UNRESOLVED, it is marked
 * as unresolved. If 'resolved' == BOXVMSYM_AUTO, then it is marked as resolved
 * only if the symbol is defined.
 */
void BoxVMSym_Ref(BoxVM *vm, BoxVMSymID sym_num, BoxVMSymResolver r,
                  void *ref, BoxUInt ref_size, BoxVMSymStatus resolved);

#if 0
/** Set the symbol resolver */
Task BoxVMSym_Resolver_Set(BoxVM *vm, UInt sym_num, VMSymResolver r);
#endif

/** Set *all_resolved = 1 only if there are no unresolved references */
void BoxVMSym_Ref_Check(BoxVM *vm, int *all_resolved);

/** Print an error message for every unresolved reference */
void BoxVMSym_Ref_Report(BoxVM *vm);

/** Resolve the symbol 'sym_num'.
 * If sym_num=0, then try to resolve all the symbols.
 * If the symbol is not defined, it silently ignore it!
 */
Task BoxVMSym_Resolve(BoxVM *vm, BoxVMSymID sym_num);

/** Print the symbol table for the given symbol. The symbol table is a list
 * of all the references made to the symbol. If sym_num is zero the references
 * to all the symbols will be printed. out is the destination.
 */
void BoxVMSym_Table_Print(BoxVM *vm, FILE *out, BoxVMSymID sym_num);

/** Check that the type of the symbol 'sym_num' is 'sym_type'. */
Task BoxVMSym_Check_Type(BoxVM *vm, BoxVMSymID sym_num, UInt sym_type);

/** Open the given dynamic library to resolve symbols. If the file is not
 * found then this function appends an appropriate extension (.so for linux,
 *.dll for windows, etc.) and tries again.
 */
Task BoxVMSym_Resolve_CLib(BoxVM *vm, char *lib_file);

/** Resolution of functions defined in external dynamically loaded
 * C libraries.
 */
Task BoxVMSym_Resolve_CLibs(BoxVM *vm, BoxList *lib_paths, BoxList *libs);

/** This function calls the function given as argument 'BoxVMSym_Code_Ref'
 * to assemble a piece of VM-code which makes reference to the symbol
 * 'sym_num'. The reference will be resolved calling again 'code_gen'
 * once the symbol has been defined.
 * The function 'code_gen' assembles parametrically a piece of VM byte-code.
 */
Task BoxVMSym_Code_Ref(BoxVM *vm, BoxVMSymID sym_num,
                       BoxVMSymCodeGen code_gen,
                       void *ref, BoxUInt ref_size);

#  define BoxVMSym_Code_New BoxVMSym_New
#  define BoxVMSym_Code_Def BoxVMSym_Def
#  define BoxVMSym_Resolve_All(vmp) BoxVMSym_Resolve(vmp, 0)

#  endif
#endif
