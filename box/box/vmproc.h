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

/** @file vmproc.h
 * @brief The procedure manager for the Box VM.
 *
 * Here we define the procedure manager for the Box virtual machine.
 * These functions allow to define new procedures, to install them
 * and to manipulate them in many ways.
 */

#ifndef _VMPROC_H_TYPES
#  define _VMPROC_H_TYPES

/** When a procedure is created, an ID (an integer number) is assigned to it.
 * BoxVMProcID is the type of such a thing (an alias for UInt).
 */
typedef BoxUInt BoxVMProcID;
typedef BoxVMProcID BoxVMProcNum; /* Alias for BoxVMProcID */

/** When a procedure is installed a "call number" X is associated to it,
 * so that it can be called with "call X". BoxVMCallNum is the type for such
 * a number.
 */
typedef BoxUInt BoxVMCallNum;

/** This is the structure used to store the bytecode representation
 * of a procedure
 */
typedef struct {
  /** Structure with the settings to control the assembler */
  struct {
    unsigned int error : 1;
    unsigned int inhibit : 1;
  } status;
  BoxArr code; /**< Array which contains effectively the code */
} BoxVMProc;

/** This structure describes an installed procedure
 * (a procedure, whose call-number has been defined.
 * The call number is the one used to call the procedure
 * in the call instruction).
 */
typedef struct {
  enum {
    BOXVMPROC_IS_VM_CODE,
    BOXVMPROC_IS_C_CODE,
  } type; /**< Kind of procedure */
  char *name; /**< Symbol-name of the procedure */
  char *desc; /**< Description of the procedure */
  union {
    BoxTask (*c)(void *); /**< Pointer to the C function (can't use VMCCode!) */
    /** Structure containing the details about the VM code,
     * which implements the procedure.
     */
    BoxUInt proc_num; /**> Number of the procedure which contains the code */
  } code;
} BoxVMProcInstalled;

/** @brief The table of installed and uninstalled procedures.
 *
 * This structure is embedded in the main VM structure BoxVM.
 */
typedef struct {
  BoxUInt
    target_proc_num,       /**< Number of the target procedure */
    tmp_proc;              /**< Procedure used as temporary buffer */
  BoxVMProc *target_proc;  /**< The target procedure */
  BoxArr installed;        /**< Array of the installed procedures */
  BoxOcc uninstalled;      /**< Array of the uninstalled procedures */
} BoxVMProcTable;

#ifdef BOX_ABBREV
typedef BoxVMProc VMProc;
typedef BoxVMProcInstalled VMProcInstalled;
typedef BoxVMProcTable VMProcTable;
#  define VMPROC_IS_VM_CODE BOXVMPROC_IS_VM_CODE
#  define VMPROC_IS_C_CODE BOXVMPROC_IS_C_CODE
#endif

#endif

#ifndef _VMPROC_H
#  ifndef _INSIDE_VIRTMACH_H
#    define _VMPROC_H

/** Initialize the procedure table.
 * @param vmp is the VM-program.
 */
Task VM_Proc_Init(BoxVM *vmp);

/** Destroy the table of installed and uninstalled procedures.
 * @param vmp is the VM-program.
 */
void VM_Proc_Destroy(BoxVM *vmp);

/** Creates a new procedure. A procedure is a place where to put
 * the assembly code (bytecode) produced by VM_Assemble and is identified
 * by an integer assigned by this function. This integer is passed back
 * through *proc_num.
 */
Task VM_Proc_Code_New(BoxVM *vmp, BoxUInt *proc_num);

/** Destroys the procedure whose number is 'proc_num'.
 */
Task VM_Proc_Code_Destroy(BoxVM *vmp, BoxUInt proc_num);

/** Set 'proc_num' to be the target procedure: the place where
 * VM_Assemble puts the assembled code
 */
BoxVMProcID BoxVM_Proc_Target_Set(BoxVM *vm, BoxVMProcID proc_num);

/** Get the ID of the target procedure */
BoxVMProcID BoxVM_Proc_Target_Get(BoxVM *vm);

/** Remove all the code assembled inside the procedure 'proc_num'.
 * WARNING: Labels and their references are not removed!
 */
void VM_Proc_Empty(BoxVM *vmp, BoxUInt proc_num);

/** Returns the call-number which will be assigned to the next installed
 * procedure.
 */
UInt VM_Proc_Install_Number(BoxVM *vmp);

/** Install the procedure 'proc_num' and assign to it the number 'call_num'.
 * After this function has been executed, the VM will recognize the instruction
 * 'call call_num' as a call to the code contained inside 'proc_num'.
 * The procedure 'proc_num' is untouched it still exists and can be modified
 * (this is necessary for symbol resolution: a procedure can be installed
 * even if it references are still undefined).
 */
void VM_Proc_Install_Code(BoxVM *vmp, BoxUInt *call_num,
                          BoxUInt proc_num, const char *name,
                          const char *desc);

/** The prototype of a C-function to be used as a procedure.
 * @see VM_Proc_Install_CCode
 */
typedef Task (*BoxVMCCode)(BoxVM *);

#ifdef BOX_ABBREV
typedef BoxVMCCode VMCCode;
#endif

/** Similar to VM_Proc_Install_Code, but install the given C-function
 * 'c_proc' as a new procedure. The call-number is returned in '*call_num'.
 * @see VM_Proc_Install_Code
 */
void VM_Proc_Install_CCode(BoxVM *vmp, BoxUInt *call_num,
                           BoxVMCCode c_proc, const char *name,
                           const char *desc);

/** Get the pointer to the bytecode of the procedure 'proc_num' and its
 * length, expressed as number of VMByteX4 elements.
 * The information is stored inside *ptr, and *length.
 * If one of these pointer is NULL, then the corresponding information
 * is not written.
 */
void VM_Proc_Ptr_And_Length(BoxVM *vmp, VMByteX4 **ptr,
                            BoxUInt *length, int proc_num);

/** Print as plain text the code contained inside the procedure 'proc_num'.
 * The stream out is the destination for the produced output.
 */
Task VM_Proc_Disassemble(BoxVM *vmp, FILE *out, BoxUInt proc_num);

/** This function prints information and assembly source code
 * of the procedure, whose number is 'call_num'.
 * It is similar to VM_Proc_Disassemble, but gives some more details.
 * @see VM_Proc_Disassemble
 */
Task VM_Proc_Disassemble_One(BoxVM *vmp, FILE *out, BoxUInt call_num);

/** This function prints the assembly source code
 * of all the installed modules.
 */
Task VM_Proc_Disassemble_All(BoxVM *vmp, FILE *out);

#if 0
void BoxVMSheet_Init(BoxVMSheet *vmsh, BoxVM *vm);
void BoxVMSheet_Finish(BoxVMSheet *vmsh);
BoxVMSheet *BoxVMSheet_New(BoxVM *vm);
void BoxVMSheet_Destroy(BoxVMSheet *vmsh);

void BoxVMSheet_Asm(...);

#endif

#  endif
#endif
