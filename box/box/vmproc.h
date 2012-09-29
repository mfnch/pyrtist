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
 * @file vmproc.h
 * @brief The procedure manager for the Box VM.
 *
 * Here we define the procedure manager for the Box virtual machine. These
 * functions allow to define new procedures, to install them and to manipulate
 * them in many ways.
 */

#ifndef _BOX_VMPROC_H
#  define _BOX_VMPROC_H

#  include <stdlib.h>
#  include <stdio.h>

#  include <box/vm.h>
#  include <box/callable.h>

/**
 * Initialize the procedure table.
 *
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

/** Returns the call-number which will be assigned to the next installed
 * procedure.
 */
BoxVMProcID BoxVM_Proc_Get_ID(BoxVM *vm, BoxVMCallNum call_num);

size_t BoxVM_Proc_Get_Size(BoxVM *vm, BoxVMProcID id);

/**
 * @brief Allocate a new call number.
 *
 * Allocate a new call number in the given VM. This call number can be used to
 * generate VM code which calls a VM procedure without having to provide an
 * implementation, first. Obviously, the VM code which is generated in this way
 * cannot be used (executed) until the call number is implemented (using
 * BoxVM_Install_Proc_Code() or BoxVM_Install_Proc_Callable()).
 *
 * @param vm The virtual machine.
 * @return A new call number for @p vm.
 */
BOXEXPORT BoxVMCallNum
BoxVM_Allocate_CallNum(BoxVM *vm);

/**
 * @brief Deallocate the most recently allocated call number.
 *
 * Deallocate the last call number allocated with BoxVM_Allocate_CallNum().
 * The call number must be unused. @p num is required just for consistency
 * check.
 * @param vm The virtual machine.
 * @param The call to be deallocated, which should also be the last allocated
 *   one.
 * @return Whether the operation was successful.
 */
BOXEXPORT BoxBool
BoxVM_Deallocate_CallNum(BoxVM *vm, BoxVMCallNum num);

/**
 * @brief Install a procedure from VM code.
 */
BOXEXPORT BoxBool
BoxVM_Install_Proc_Code(BoxVM *vm, BoxVMCallNum call_num, BoxVMProcID id);

/**
 * Similar to BoxVM_Install_Proc_Code(), but installs the given C-function
 * @p c_proc as a new procedure. The call-number is returned in
 *   <tt>*call_num</tt>.
 * @param vm The virtual machine.
 * @param cn The call number used for installing the code. This is the number
 *   which can be used to call the procedure from bytecode.
 * @param c_proc The implementation of the procedure in C.
 * @return Whether the operation was successful.
 */
BOXEXPORT BoxBool
BoxVM_Install_Proc_CCode(BoxVM *vm, BoxVMCallNum cn, BoxVMCCode c_proc);

/**
 * Similar to BoxVM_Install_Proc_Code(), but installs the given C-function
 * @p c_proc as a new procedure. The call-number is returned in
 *   <tt>*call_num</tt>.
 * @param vm The virtual machine.
 * @param cn The call number used for installing the code. This is the number
 *   which can be used to call the procedure from bytecode.
 * @param c_proc The implementation of the procedure in C.
 * @return Whether the operation was successful.
 */
BOXEXPORT BoxBool
BoxVM_Install_Proc_Callable(BoxVM *vm, BoxVMCallNum cn, BoxCallable *cb);

/**
 */
BOXEXPORT BoxBool
BoxVM_Set_Proc_Names(BoxVM *vm, BoxVMCallNum cn,
                     const char *name, const char *desc);

/**
 * @brief Whether a given call number is allocated for the specified VM.
 *
 * @param vm The virtual machines the call number refers to.
 * @param cn The call number.
 * @return Whether @p cb is allocated.
 */
BOXEXPORT BoxBool 
BoxVM_Call_Is_Allocated(BoxVM *vm, BoxVMCallNum cn);







#if 0
/**
 * @brief Define the signature for a call number.
 *
 * Provide the procedure signature (input and output types) for a call number
 * @p cn of the given VM @p vm.
 *
 * @param vm The virtual machines the call number refers to.
 * @param cn The call number to define.
 * @param cb A callable containing the signature of the call number.
 * @return Whether the operation was successful.
 * @note The callable @p cb may be undefined. The actual implementation of
 *   @p cb is ignored. Use BoxVM_Implement_Call_From_Code() or
 *   BoxVM_Implement_Call_From_Callable() to provide the implementation.
 */
BOXEXPORT BoxBool
BoxVM_Define_Call(BoxVM *vm, BoxVMCallNum cn, BoxCallable *cb);

/**
 * @brief Whether a given call number has been defined in the specified VM.
 *
 * @param vm The virtual machines the call number refers to.
 * @param cn The call number.
 * @return Whether @p cb has been defined with BoxVM_Define_Call().
 */
BOXEXPORT BoxBool 
BoxVM_Call_Is_Defined(BoxVM *vm, BoxVMCallNum cn);

/**
 * @brief Provide a VM code implementation for a call number.
 *
 * @param vm The virtual machine the call number @p cn refers to.
 * @param cn The call number to be implemented.
 * @param code The #BoxVMCode object containing the implementation.
 * @return Whether the operation was completed successfully.
 */
BOXEXPORT BoxBool
BoxVM_Implement_Call_From_Code(BoxVM *vm, BoxVMCallNum cn, BoxVMCode *code);

/**
 * @brief Provide a generic #BoxCallable implementation for a call number.
 *
 * @param vm The virtual machine the call number @p cn refers to.
 * @param cn The call number to be implemented.
 * @param cb The callable containing the implementation.
 * @return Whether the operation was completed successfully.
 */
BOXEXPORT BoxBool
BoxVM_Implement_Call_From_Callable(BoxVM *vm, BoxVMCallNum cn,
                                   BoxCallable *cb);

/**
 */
BOXEXPORT BoxBool
BoxVM_Get_Code_Implem(BoxVM *vm, BoxVMCallNum cn, BoxCode **code);

/**
 */
BOXEXPORT BoxBool 
BoxVM_Get_Callable_Implem(BoxVM *vm, BoxVMCallNum cn, BoxCallable **code);

/**
 * @brief Get the callable corresponding to a given call number.
 *
 * @param vm VM the call number @p cb refers to.
 * @param cn The call number.
 * @return The callable corresponding to the given call number @p cb.
 */
BOXEXPORT BoxCallable *
BoxVM_Get_Callable(BoxVM *vm, BoxVMCallNum cn);
#endif











/** Return the state of the procedure with the given call number.
 * BOXVMPROCKIND_UNDEFINED is returned when no procedure with such call number
 * has been ever installed. When the procedure has been installed, return its
 * state, which depends on the routine used to do the installation.
 * Example: return BOXVMPROCKIND_C_CODE if BoxVM_Proc_Install_CCode was used.
 */
BoxVMProcKind BoxVM_Is_Installed(BoxVM *vm, BoxVMCallNum call_num);

/** Get the pointer to the bytecode of the procedure 'proc_num' and its
 * length, expressed as number of BoxVMWord elements.
 * The information is stored inside *ptr, and *length.
 * If one of these pointer is NULL, then the corresponding information
 * is not written.
 */
void BoxVM_Proc_Get_Ptr_And_Length(BoxVM *vmp, BoxVMWord **ptr,
                                   BoxUInt *length, BoxVMProcID proc_id);

/** Print as plain text the code contained inside the procedure 'proc_num'.
 * The stream out is the destination for the produced output.
 */
Task BoxVM_Proc_Disassemble(BoxVM *vmp, FILE *out, BoxVMProcID proc_num);

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

#endif /* _BOX_VMPROC_H */
