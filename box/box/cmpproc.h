/****************************************************************************
 * Copyright (C) 2009 by Matteo Franchin                                    *
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

/** @file cmpproc.h
 * @brief CmpProc, generic object used for handling procedures.
 *
 * The object CmpProc is used to handle operations on procedures, such as
 * declaration, installation, referencing, assembly, etc.
 */

#ifndef _BOX_CMPPROC_H
#  define _BOX_CMPPROC_H

#  include <stdlib.h>
#  include <stdarg.h>

#  include "types.h"
#  include "registers.h"
#  include "vm_private.h"
#  include "vmsym.h"
#  include "container.h"
#  include "vmproc.h"
#  include "cmpptrs.h"
#  include "srcpos.h"

/** The CmpProc object. */
typedef struct BoxVMCode_struct BoxVMCode;
typedef BoxVMCode CmpProc;

/** Procedure style: determines if a procedure preamble and conclusion should
 * be inserted and if global (or local) variables should be used by default.
 * Example: by using CMPPROCKIND_SUB, the instructions "newX YY, ZZ" will
 * be inserted automatically at the beginning of the procedure, while the
 * instruction "ret" will be appended at the end.
 *
 * NOTE: the preamble is emitted only when the user calls CmpProc_Assemble
 *   (or equivalent) for the first time.
 */
typedef enum {
  CMPPROCSTYLE_PLAIN, /**< Plain procedure without automatic generation of
                           preamble and ending code. */
  CMPPROCSTYLE_MAIN,  /**< Main procedure, with automatic generation of
                           preamble and final ret instruction. Variables
                           are global (and subprocedures can access them). */
  CMPPROCSTYLE_SUB,   /**< Sub-procedure, with automatic generation of
                           preamble and ret instruction. Variables are local
                           to the procedure. */
  CMPPROCSTYLE_EXTERN /**< Externally defined procedure. Cannot be used as
                           a target for compilation. */
} CmpProcStyle;

/** Function called in order to generate the beginning of a procedure
 * (such as the register allocation instructions 'newi xx, yy', etc.)
 */
typedef void (*CmpProcBegin)(CmpProc *p);

/** Function called in order to generate tye ending of a procedure
 * (such as the 'ret' bytecode)
 */
typedef void (*CmpProcEnd)(CmpProc *p);

/** @brief The CmpProc object.
 */
struct BoxVMCode_struct {
  struct {
    unsigned int
               parent     :1,  /**< The procedure has a parent */
               child      :1,  /**< The procedure has a child */
               reg_alloc  :1,  /**< it has automatic register allocation */
               sym        :1,  /**< the procedure has an associated symbol */
               proc_id    :1,  /**< the procedure has a procedure number */
               proc_name  :1,  /**< it has a name */
               alter_name :1,  /**< has an alternative name */
               call_num   :1,  /**< it has a call number */
               wrote_beg  :1,  /**< CmpProc->beginning has been called */
               wrote_end  :1,  /**< CmpProc->ending has been called */
               installed  :1,  /**< The procedure was installed */
               head       :1;  /**< Head new-instructions have been emitted */
  } have;
  struct {
    unsigned int
                proc_id   :1;  /**< Permission to create proc_id */
  } perm;
  CmpProcStyle  style;       /**< Procedure style */
  BoxCmp        *cmp;        /**< Compiler corresponding to the procedure */
  CmpProcBegin  beginning;   /**< If not NULL, this is called before emitting
                                  any other instruction */
  CmpProcEnd    ending;      /**< If not NULL, this function is called at the
                                  end of the procedure */
  RegAlloc      reg_alloc;   /**< Register allocator: keeps track of registers
                                  which are being used and of those which are
                                  not used.*/
  BoxVMSymID    head_sym_id, /**< Symbol associated with the head block of
                                  new instructions */
                sym;         /**< Symbol associated with the procedure */
  BoxVMProcID   proc_id;     /**< Proc. number (needed to write ASM to it) */
  char          *proc_name,  /**< Procedure name */
                *alter_name; /**< Alternative name */
  BoxVMCallNum  call_num;    /**< Call number (needed to call it from ASM) */
  BoxVMRegNum   reg_parent,  /**< Register number for the parent */
                reg_child;   /**< Register number for the child */
};

/** Initialise a CmpProc object in the memory region pointed by p.
 * @param p A pointer to the space where the CmpProc object will be stored.
 */
void CmpProc_Init(CmpProc *p, BoxCmp *c, CmpProcStyle style);

/** Finalise a CmpProc object initialised with CmpProc_Init.
 */
void CmpProc_Finish(CmpProc *p);

/** Allocate space in the heap to hold a CmpProc object and initialise it with
 * CmpProc_Init.
 */
CmpProc *CmpProc_New(BoxCmp *c, CmpProcStyle style);

/** Destroy a CmpProc object created with CmpProc_New.
 */
void CmpProc_Destroy(CmpProc *p);

/** Initialise the code in the procedure, coherently with the procedure style.
 */
void CmpProc_Begin(CmpProc *p);

/** Finalise the code in the procedure, coherently with the procedure style.
 */
void CmpProc_End(CmpProc *p);

/** Set the prototype for the procedure. The prototype must be set for
 * procedures of type CMPPROCSTYLE_SUB in order to be able to use the methods
 * CmpProc_Get_Child_Reg and CmpProc_Get_Parent_Reg.
 * @see CmpProc_Get_Child_Reg, CmpProc_Get_Parent_Reg
 */
void CmpProc_Set_Prototype(CmpProc *p, int have_child, int have_parent);

/** Get the VM-register for the parent (a fatal error occurs if the procedure
 * hasn't got a parent).
 */
BoxVMRegNum CmpProc_Get_Parent_Reg(CmpProc *p);

/** Get the VM-register for the child (a fatal error occurs if the procedure
 * hasn't got a child).
 */
BoxVMRegNum CmpProc_Get_Child_Reg(CmpProc *p);

/** Provides a name for the procedure (necessary for CMPPROCSTYLE_EXTERN)
 * This is usually the C name of the procedure.
 */
void CmpProc_Set_Name(CmpProc *p, const char *proc_name);

/** Provides an alternative name for the procedure, this is usually the Box
 * name of the procedure (such as Int@Print) and is the name displayed when
 * disassembling the bytecode instruction call.
 * If it is not provided, then the procedure name is used (CmpProc_Set_Name)
 * or "|unknown|", if the name is not set either.
 */
void CmpProc_Set_Alter_Name(CmpProc *p, const char *alter_name);

/** Retrieve the alternative name for the procedure.
 */
char *CmpProc_Get_Alter_Name(CmpProc *p);

/** Get the procedure style (the parameter which was given during procedure
 * creation).
 */
CmpProcStyle CmpProc_Get_Style(CmpProc *p);

/** Get the register allocator for the procedure */
RegAlloc *CmpProc_Get_RegAlloc(CmpProc *p);

/** Sets the symbol associated to the procedure. */
void CmpProc_Set_Sym(CmpProc *p, BoxVMSymID sym_id);

/** Returns the symbol associated to the procedure (creating it, if necessary)
 */
BoxVMSymID CmpProc_Get_Sym(CmpProc *p);

/** Get the procedure ID, which is an integer number which the VM
 * allocates when it creates a BoxVMProc (which is just a bytecode container,
 * a place where bytecode can be written).
 * If such a number does not exist, then a procedure is created, which
 * is associated to this CmpProc object and the corresponding number
 * is returned.
 */
BoxVMProcID CmpProc_Get_ProcID(CmpProc *p);

/** Get the procedure call number. If the procedure doesn't have a call number
 * then, it is installed and the call number returned by such installation
 * operation is returned.
 */
BoxVMCallNum CmpProc_Get_Call_Num(CmpProc *p);

/** Returns the size of the VM code associated with the given procedure.
 * In case of errors returns a negative value:
 *  -1: if there has not been any attempt to write VM code on the procedure;
 *  -2: if this is a C procedure;
 */
size_t CmpProc_Get_Code_Size(CmpProc *p);

/** Install the procedure such that the code which calls it can be actually
 * executed.
 */
BoxVMCallNum CmpProc_Install(CmpProc *p);

/** Assemble the instruction calling VM_Assemble and put the code inside
 * the given CmpProc procedure p. A higher level function for assembling code
 * is provided by CmpProc_Assemble.
 * @see CmpProc_Raw_VA_Assemble, CmpProc_Assemble
 */
void CmpProc_Raw_Assemble(CmpProc *p, BoxOpcode instr, ...);

/** Non-variadic version of the function CmpProc_Raw_Assemble.
 * Equivalent to the latter, but gets a va_list rather than a variable-length
 * argument list.
 * @see CmpProc_Raw_Assemble
 */
void CmpProc_Raw_VA_Assemble(CmpProc *p, BoxOpcode instr, va_list ap);

/**
 * @brief Assemble a @c call instruction to call the procedure with the given
 *   call number.
 */
void BoxVMCode_Assemble_Call(BoxVMCode *code, BoxVMCallNum call_num);

/** High level routine to assemble bytecode for the Box virtual machine (VM).
 */
void CmpProc_Assemble(CmpProc *p, BoxGOp g_op, int num_args, ...);

/** Create a new jump-label. The label can be used to assemble jump
 * instructions with CmpProc_Assemble_Jump, CmpProc_Assemble_CJump.
 * The label has to be defined using CmpProc_Jump_Label_Define.
 */
BoxVMSymID CmpProc_Jump_Label_New(CmpProc *p);

/** Shorthand for:
 * jl = CmpProc_Jump_Label_New(p);
 * CmpProc_Jump_Label_Define(p, jl);
 * return jl;
 */
BoxVMSymID CmpProc_Jump_Label_Here(CmpProc *p);

/** Define an existing undefined label to point to the current position in
 * the procedure.
 */
void CmpProc_Jump_Label_Define(CmpProc *p, BoxVMSymID jl);

/** Release the given jump label. After this function has been called the jump
 * label cannot be used anymore.
 */
void CmpProc_Jump_Label_Release(CmpProc *p, BoxVMSymID jl);

/** Assemble a jump instruction to the given jump label. */
void CmpProc_Assemble_Jump(CmpProc *p, BoxVMSymID jl);

/** Assemble a conditional jump instruction to the given jump label. */
void CmpProc_Assemble_CJump(CmpProc *p, BoxVMSymID jl, BoxCont *cont);

/** State that the next code generated in the procedure will correspond
 * to the given position in the source file. Use a NULL pointer to specify
 * that such information is not known.
 */
void CmpProc_Associate_Source(CmpProc *p, BoxSrcPos *src_pos);

#endif /* _BOX_CMPPROC_H */
