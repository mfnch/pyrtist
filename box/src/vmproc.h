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

/** @file vmproc.h
 * @brief The procedure manager for the Box VM.
 *
 * Here we define the procedure manager for the Box virtual machine.
 * These functions allow to define new procedures, to install them
 * and to manipulate them in many ways.
 */

#ifndef _VMPROC_H_TYPES
#  define _VMPROC_H_TYPES

/** @brief The structure which keeps installed and uninstalled procedure.
 *
 * This structure is embedded in the main VM structure VMProgram.
 */
typedef struct {
  Array *installed;
  Collection *uninstalled;
} VMProcTable;

/** This is the structure used to store the bytecode representation
 * of a procedure
 */
typedef struct {
  /** Structure with some flags, to control the assembler */
  struct {
    unsigned int error : 1;
    unsigned int inhibit : 1;
  } status;
  Array *code; /**< Array which contains effectively the code */
} VMProc;

#endif

#ifndef _VMPROC_H
#  ifndef _INSIDE_VIRTMACH_H
#    define _VMPROC_H

/** Initialize the procedure table.
 * @param vmp is the VM-program.
 */
Task VM_Proc_Init(VMProgram *vmp);

/** Destroy the symbol table of the program.
 * @param vmp is the VM-program.
 */
void VM_Sym_Destroy(VMProgram *vmp);

#    if 0
/** Create a symbol for a new reference or for the definition of a procedure.
 * @param s the VMSym structure which will be filled to represent
 *  the reference or the definition of the procedure;
 * @param name the name of the procedure;
 * @param sheet this parameter is zero for a reference to the procedure
 *  and is greater than zero for its definition. In this latter case
 *  it is the number of the sheet which contains the code of the procedure.
 */
void VM_Sym_Procedure(VMSym *s, Name *name, int sheet);

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
#    endif

#  endif
#endif
