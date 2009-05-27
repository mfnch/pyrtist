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

#ifndef _CMPPROC_H
#  define _CMPPROC_H

#  include <stdarg.h>

#  include "types.h"
#  include "registers.h"
#  include "virtmach.h"
#  include "vmsym.h"
#  include "vmproc.h"
#  include "cmpptrs.h"

/** The CmpProc object. */
typedef struct _cmp_proc CmpProc;

/** Procedure style: determines if a procedure preamble and closure should
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
  CMPPROCSTYLE_SUB    /**< Sub-procedure, with automatic generation of
                           preamble and ret instruction. Variables are local
                           to the procedure. */
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
struct _cmp_proc {
  struct {
    unsigned int
                reg_alloc :1,  /**< it has automatic register allocation */
                sym       :1,  /**< the procedure has an associated symbol */
                proc_id   :1,  /**< the procedure has a procedure number */
                proc_name :1,  /**< it has a name */
                call_num  :1,  /**< it has a call number */
                type      :1,  /**< it has a type */
                wrote_beg :1,  /**< CmpProc->beginning has been called */
                wrote_end :1,  /**< CmpProc->ending has been called */
                head      :1;  /**< Head instructions new have been emitted */
  } have;
  CmpProcStyle  style;       /**< Procedure style */
  BoxCmp        *cmp;        /**< Compiler corresponding to the procedure */
  CmpProcBegin  beginning;   /**< If not NULL, this is called before emitting
                                  any other instruction */
  CmpProcEnd    ending;      /**< If not NULL, this function is called at the
                                  end of the procedure */
  RegAlloc      reg_alloc;   /**< Register allocator: keeps track of registers
                                  which are being used and of those which are
                                  not used.*/
  BoxVMSymID    head_sym_id; /**< Symbol associated with the head block of
                                  new instructions */
  BoxVMSymID    sym;         /**< Symbol associated with the procedure */
  BoxVMProcNum  proc_id;     /**< Proc. number (needed to write ASM to it) */
  char          *proc_name;  /**< Procedure name */
  BoxVMCallNum  call_num;    /**< Call number (needed to call it from ASM) */
  Type          type;        /**< Type of the procedure */
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

/** Get the register allocator for the procedure */
RegAlloc *CmpProc_Get_RegAlloc(CmpProc *p);

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

/** Get the procedure description, which is just a help string to make
 * the bytecode more readable (may disappear in the future).
 * For now the procedure description is:
 *  - the type of the procedure, if this is known;
 *  - the name of the procedure, if this is known;
 *  - "|unknown|"
 */
char *CmpProc_Get_Proc_Desc(CmpProc *p);

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
Int CmpProc_Code_Size(CmpProc *p);

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

/** Assemble a 'call' instruction to call the procedure whose symbol ID is
 * 'sym_id' (using BoxVMSym_Assemble_Call).
 */
void CmpProc_Assemble_Call(CmpProc *p, BoxVMSymID sym_id);

/** High level routine to assemble bytecode for the Box virtual machine (VM).
 */
void CmpProc_Assemble(CmpProc *p, BoxGOp g_op, int num_args, ...);

#endif
