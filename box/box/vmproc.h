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

#  include <stdlib.h>

#  include <box/srcpos.h>

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
  struct {
    unsigned int   error   :1;
    unsigned int   inhibit :1;
  }              status;    /**< Structure with the settings to control the assembler */
  BoxSrcPosTable pos_table; /**< Info about the corresponding source files */
  BoxArr         code;      /**< Array which contains effectively the code */
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
    BoxVMProcID proc_id; /**< Number of the procedure which contains
                              the code */
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
typedef BoxVMProcInstalled VMProcInstalled;
typedef BoxVMProcTable VMProcTable;
#endif

#endif

#ifndef _VMPROC_H
#  ifndef _INSIDE_VIRTMACH_H
#    define _VMPROC_H

/** Initialize the procedure table.
 * @param vmp is the VM-program.
 */
void BoxVM_Proc_Init(BoxVM *vmp);

/** Destroy the table of installed and uninstalled procedures.
 * @param vmp is the VM-program.
 */
void BoxVM_Proc_Finish(BoxVM *vmp);

/** Creates a new procedure. A procedure is a place where to put
 * the assembly code (bytecode) produced by VM_Assemble and is identified
 * by an integer assigned by this function, which is returned.
 */
BoxVMProcID BoxVM_Proc_Code_New(BoxVM *vm);

/** Destroys the procedure whose number is 'proc_num'.
 */
void BoxVM_Proc_Code_Destroy(BoxVM *vm, BoxVMProcID proc_id);

/** Return a string that may serve to the user to identify the procedure */
char *BoxVM_Proc_Get_Description(BoxVM *vm, BoxVMCallNum call_num);

/** Set 'proc_num' to be the target procedure: the place where
 * VM_Assemble puts the assembled code
 * NOTE: return 0, if a target procedure has not been set, yet.
 *   If proc_num == 0, unset the target.
 */
BoxVMProcID BoxVM_Proc_Target_Set(BoxVM *vm, BoxVMProcID proc_num);

/** Get the ID of the target procedure */
BoxVMProcID BoxVM_Proc_Target_Get(BoxVM *vm);

/** Remove all the code assembled inside the procedure 'proc_id'.
 * WARNING: Labels and their references are not removed!
 */
void BoxVM_Proc_Empty(BoxVM *vm, BoxVMProcID proc_id);

/** Returns the call-number which will be assigned to the next installed
 * procedure.
 */
BoxVMCallNum BoxVM_Proc_Next_Call_Num(BoxVM *vmp);

size_t BoxVM_Proc_Get_Size(BoxVM *vm, BoxVMProcID id);

/** Install the procedure 'proc_id' and assign to it a call number 'n' which
 * is returned. After this function has been executed, the VM can recognize
 * the instruction 'call n' as a call to the code contained inside 'proc_id'.
 * The procedure 'proc_id' is untouched it still exists and can be modified
 * (this is necessary for symbol resolution: a procedure can be installed
 * even if it references are still undefined).
 */
BoxVMCallNum BoxVM_Proc_Install_Code(BoxVM *vm, BoxVMProcID proc_id,
                                     const char *name, const char *desc);

/** The prototype of a C-function to be used as a procedure.
 * @see VM_Proc_Install_CCode
 */
typedef Task (*BoxVMCCode)(BoxVM *);

#ifdef BOX_ABBREV
typedef BoxVMCCode VMCCode;
#endif

/** Similar to VM_Proc_Install_Code, but install the given C-function
 * 'c_proc' as a new procedure. The call-number is returned in '*call_num'.
 * @see BoxVM_Proc_Install_Code
 */
BoxVMCallNum BoxVM_Proc_Install_CCode(BoxVM *vmp, BoxVMCCode c_proc,
                                      const char *name, const char *desc);

/** Get the pointer to the bytecode of the procedure 'proc_num' and its
 * length, expressed as number of VMByteX4 elements.
 * The information is stored inside *ptr, and *length.
 * If one of these pointer is NULL, then the corresponding information
 * is not written.
 */
void BoxVM_Proc_Get_Ptr_And_Length(BoxVM *vmp, VMByteX4 **ptr,
                                   BoxUInt *length, BoxVMProcID proc_id);

/** Print as plain text the code contained inside the procedure 'proc_num'.
 * The stream out is the destination for the produced output.
 */
Task BoxVM_Proc_Disassemble(BoxVM *vmp, FILE *out, BoxUInt proc_num);

/** This function prints information and assembly source code
 * of the procedure, whose number is 'call_num'.
 * It is similar to VM_Proc_Disassemble, but gives some more details.
 * @see VM_Proc_Disassemble
 */
Task BoxVM_Proc_Disassemble_One(BoxVM *vmp, FILE *out, BoxVMCallNum call_num);

/** This function prints the assembly source code
 * of all the installed modules.
 */
Task BoxVM_Proc_Disassemble_All(BoxVM *vmp, FILE *out);

/** Associate the given position in the source file 'sp' with the current
 * position in the procedure so that it can be later retrieved with
 * BoxVM_Proc_Get_Source_Of.
 * @see BoxVM_Proc_Get_Source_Of
 */
void BoxVM_Proc_Associate_Source(BoxVM *vm, BoxVMProcID id, BoxSrcPos *sp);

/** Retrieve the position in the source corresponding to the given position
 * 'op' in the procedure 'id'.
 */
BoxSrcPos *BoxVM_Proc_Get_Source_Of(BoxVM *vm, BoxVMProcID id, BoxOutPos op);

#  endif
#endif
