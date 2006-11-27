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

#ifndef _VIRTMACH_H
#  include "virtmach.h"
#endif

#ifndef _VMSYM_H_TYPES
#  define _VMSYM_H_TYPES

#  include "types.h"
#  include "array.h"

/** @file vmsym.h
 * @brief The implementation of the reference/definition list for the box VM.
 *
 * This file implements the mechanism to define and reference procedures,
 * labels and other symbols needed by a Box bytecode compiled program.
 * This mechanism is similar to the linking of the object files produced
 * by the compilation of several C-source files. In this sense whe could
 * say that this file implements the Box linker.
 */

/** @brief The table of reference and definition for the Box VM.
 *
 * This structure is embedded in the main VM structure VMProgram.
 */
typedef struct {
  Array *defs;
  Array *refs;
} VMSymTable;

/** @brief Reference or definition of a symbol for the virtual machine of box.
 *
 * This structure contains the symbol to be added with the function
 * VM_Sym_Add.
 */
typedef struct {
  int is_definition; /**< 1 for a definition, 0 for a reference */
  char *name; /**< name of the reference or definition */
  enum {VMSYM_NONE, VMSYM_PROCEDURE} type; /**< Type of reference/definition */
  union {
    struct {
      int sheet; /**< sheet where the reference is */
      int position; /**< position of the reference in the sheet */
      int length; /**< length of the code which contains the reference */
    } ref; /**< All the things which describe a reference */
    int def_procedure; /**< The sheet containing the defined procedure */
  } value; /** What is needed to specify a reference/definition */
} VMSym;
#endif

#ifndef _VMSYM_H
#  ifndef _INSIDE_VIRTMACH_H
#    define _VMSYM_H

/** Initialize the symbol table of the program.
 * @param vmp is the VM-program.
 */
Task VM_Sym_Begin(VMProgram *vmp);

/** Destroy the symbol table of the program.
 * @param vmp is the VM-program.
 */
Task VM_Sym_End(VMProgram *vmp);

/** Create a symbol for a new reference or for the definition of a procedure.
 * @param s the VMSym structure which will be filled to represent
 *  the reference or the definition of the procedure;
 * @param name the name of the procedure;
 * @param sheet this parameter is zero for a reference to the procedure
 *  and is greater than zero for its definition. In this latter case
 *  it is the number of the sheet which contains the code of the procedure.
 */
void VM_Sym_Procedure(VMSym *s, char *name, int sheet);

/** Specify the code which makes the reference.
 * @param s the referenced symbol;
 * @param sheet sheet which contains the code which makes the reference.
 * @param position position of the code which makes the reference.
 * @param length size of the code which makes the reference.
 */
void VM_Sym_Reference(VMSym *s, int sheet, int position, int length);

/** Add a new reference/definition to the VM-program reference list.
 *  @param vmp is the VM-program.
 *  @param s is the reference or definition.
 *  @see VM_Reference_Def()
 */
Task VM_Sym_Add(VMProgram *vmp, VMSym *s);

/** Resolve all the references in the bytecode compiled program.
 * @param vmp is the VM-program.
 */
Task VM_Sym_Link(VMProgram *vmp);
#  endif
#endif
