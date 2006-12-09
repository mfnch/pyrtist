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

/** This is the structure used to store the bytecode representation
 * of a procedure
 */
typedef struct {
  /** Structure with the settings to control the assembler */
  struct {
    unsigned int error : 1;
    unsigned int inhibit : 1;
  } status;
  Array *code; /**< Array which contains effectively the code */
} VMProc;

/** This structure describes an installed procedure
 * (a procedure, whose call-number has been defined.
 * The call number is the one used to call the procedure
 * in the call instruction).
 */
typedef struct {
  enum {
    VMPROC_IS_VM_CODE,
    VMPROC_IS_C_CODE,
  } type; /**< Kind of procedure */
  char *name; /**< Symbol-name of the procedure */
  char *desc; /**< Description of the procedure */
  union {
    Task (*c)(void *); /**< Pointer to the C function (can't use VMCCode!) */
    /** Structure containing the details about the VM code,
     * which implements the procedure.
     */
    struct {
      unsigned int size; /**< Number of VMByteX4 in the code */
      void *ptr; /**< Pointer to the code */
    } vm;
  } code;
} VMProcInstalled;

/** @brief The structure which keeps installed and uninstalled procedure.
 *
 * This structure is embedded in the main VM structure VMProgram.
 */
typedef struct {
  unsigned int target_proc_num; /**< Number of the target procedure */
  VMProc *target_proc;  /**< The target procedure */
  Array *installed; /**< Array of the installed procedures */
  Collection *uninstalled; /**< Collection of the uninstalled procedures */
} VMProcTable;

#endif

#ifndef _VMPROC_H
#  ifndef _INSIDE_VIRTMACH_H
#    define _VMPROC_H

/** Initialize the procedure table.
 * @param vmp is the VM-program.
 */
Task VM_Proc_Init(VMProgram *vmp);

/** Destroy the table of installed and uninstalled procedures.
 * @param vmp is the VM-program.
 */
void VM_Sym_Destroy(VMProgram *vmp);

/** Creates a new procedure. A procedure is a place where to put
 * the assembly code (bytecode) produced by VM_Assemble and is identified
 * by an integer assigned by this function. This integer is passed back
 * through *proc_num.
 */
Task VM_Proc_Code_New(VMProgram *vmp, unsigned int *proc_num);

/** Destroys the procedure whose number is 'proc_num'.
 */
Task VM_Proc_Code_Destroy(VMProgram *vmp, unsigned int proc_num);

/** Set 'proc_num' to be the target procedure: the place where
 * VM_Assemble put the assembled code
 */
Task VM_Proc_Target_Set(VMProgram *vmp, unsigned int sheet_id);

/** Get the ID of the target procedure */
unsigned int VM_Proc_Target_Get(VMProgram *vmp);

/** Remove all the code assembled inside the procedure 'proc_num'.
 * WARNING: Labels and their references are not removed!
 */
Task VM_Proc_Empty(VMProgram *vmp, unsigned int proc_num);


/** Install the procedure 'proc_num' and assign to it the number 'call_num'.
 * After this function has been executed, the VM will recognize the instruction
 * 'call call_num' as a call to the code contained inside the sheet 'proc_num'.
 * The procedure 'proc_num' will be empty, but not destroyed (it will still be
 * inside the collection of uninstalled sheets).
 */
Task VM_Proc_Install_Code(VMProgram *vmp, unsigned int *call_num,
                          unsigned int proc_num, const char *name,
                          const char *desc);

/** The prototype of a C-function to be used as a procedure.
 * @see VM_Proc_Install_CCode
 */
typedef Task (*VMCCode)(VMProgram *);

/** Similar to VM_Proc_Install_Code, but install the given C-function
 * 'c_proc' as a new procedure. The call-number is returned in '*call_num'.
 * @see VM_Proc_Install_Code
 */
Task VM_Proc_Install_CCode(VMProgram *vmp, unsigned int *call_num,
                           VMCCode c_proc, const char *name,
                           const char *desc);

/** Get the pointer to the bytecode of the procedure 'proc_num' and its
 * length, expressed as number of VMByteX4 elements.
 * The information is stored inside *ptr, and *length.
 * If one of these pointer is NULL, then the corresponding information
 * is not written.
 */
Task VM_Proc_Ptr_And_Length(VMProgram *vmp, VMByteX4 **ptr,
                            unsigned int *length, int proc_num);

/** Print as plain text the code contained inside the procedure 'proc_num'.
 * The stream out is the destination for the produced output.
 */
Task VM_Proc_Disassemble(VMProgram *vmp, FILE *out, unsigned int proc_num);

#  endif
#endif
