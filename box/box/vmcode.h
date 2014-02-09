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

/**
 * @file vmcode.h
 * @brief BoxVMCode, generic object used for handling procedures.
 *
 * The object BoxVMCode is used to handle operations on procedures, such as
 * declaration, installation, referencing, assembly, etc.
 */

#ifndef _BOX_BOXVMCODE_H
#  define _BOX_BOXVMCODE_H

#  include <stdlib.h>
#  include <stdarg.h>

#  include <box/types.h>
#  include <box/registers.h>
#  include <box/vm_priv.h>
#  include <box/vmsym.h>
#  include <box/container.h>
#  include <box/vmproc.h>
#  include <box/compiler.h>
#  include <box/srcpos.h>
#  include <box/callable.h>
#  include <box/lir.h>


/**
 * @brief The BoxVMCode object.
 */
typedef struct BoxVMCode_struct BoxVMCode;

/**
 * Procedure style: determines if a procedure preamble and conclusion should
 * be inserted and if global (or local) variables should be used by default.
 * Example: by using #BOXVMCODETYPE_SUB, the instructions "newX YY, ZZ" will
 * be inserted automatically at the beginning of the procedure, while the
 * instruction @c ret will be appended at the end.
 *
 * @note the preamble is emitted only when the user calls BoxVMCode_Assemble()
 *   (or equivalent) for the first time.
 */
typedef enum {
  BOXVMCODESTYLE_PLAIN, /**< Plain procedure without automatic generation of
                           preamble and ending code. */
  BOXVMCODESTYLE_MAIN,  /**< Main procedure, with automatic generation of
                           preamble and final ret instruction. Variables
                           are global (and subprocedures can access them). */
  BOXVMCODESTYLE_SUB,   /**< Sub-procedure, with automatic generation of
                           preamble and ret instruction. Variables are local
                           to the procedure. */
  BOXVMCODESTYLE_EXTERN /**< Externally defined procedure. Cannot be used as
                           a target for compilation. */
} BoxVMCodeStyle;

/**
 * Function called in order to generate the beginning of a procedure
 * (such as the register allocation instructions 'newi xx, yy', etc.)
 */
typedef void (*BoxVMCodeBegin)(BoxVMCode *p);

/**
 * Function called in order to generate tye ending of a procedure
 * (such as the @c ret bytecode)
 */
typedef void (*BoxVMCodeEnd)(BoxVMCode *p);

/**
 * @brief Implementation of the #BoxVMCode object.
 */
struct BoxVMCode_struct {
  struct {
    unsigned int
               parent     :1,  /**< The procedure has a parent */
               child      :1,  /**< The procedure has a child */
               reg_alloc  :1,  /**< it has automatic register allocation */
               proc_id    :1,  /**< the procedure has a procedure number */
               proc_name  :1,  /**< it has a name */
               alter_name :1,  /**< has an alternative name */
               call_num   :1,  /**< it has a call number */
               wrote_beg  :1,  /**< BoxVMCode->beginning has been called */
               wrote_end  :1,  /**< BoxVMCode->ending has been called */
               installed  :1,  /**< The procedure was installed */
               head       :1,  /**< Head new-instructions have been emitted */
               callable   :1;
  } have;
  struct {
    unsigned int
                proc_id   :1;  /**< Permission to create proc_id */
  } perm;
  BoxVMCodeStyle  style;       /**< Procedure style */
  BoxCmp        *cmp;        /**< Compiler corresponding to the procedure */
  BoxLIRNodeProc
                *proc;
  BoxVMCodeBegin
                beginning;   /**< If not NULL, this is called before emitting
                                  any other instruction */
  BoxVMCodeEnd  ending;      /**< If not NULL, this function is called at the
                                  end of the procedure */
  RegAlloc      reg_alloc;   /**< Register allocator: keeps track of registers
                                  which are being used and of those which are
                                  not used.*/
  BoxVMSymID    head_sym_id, /**< Symbol associated with the head block of
                                  new instructions */
                sym;         /**< Symbol associated with the procedure */
  BoxVMProcID   proc_id;     /**< Proc. number (needed to write ASM to it) */
  BoxVMProcID   real_proc_id; /* Hack */
  char          *proc_name,  /**< Procedure name */
                *alter_name; /**< Alternative name */
  BoxVMCallNum  call_num;    /**< Call number (needed to call it from ASM) */
  BoxVMRegNum   reg_parent,  /**< Register number for the parent */
                reg_child;   /**< Register number for the child */

  BoxCallable   *callable;
};

/**
 * @brief Initialise the object.
 * @param p A pointer to the space where the #BoxVMCode object will be stored.
 */
BOXEXPORT void
BoxVMCode_Init(BoxVMCode *p, BoxCmp *c, BoxVMCodeStyle style);

/**
 * @brief Finalise a #BoxVMCode object initialised with BoxVMCode_Init().
 */
BOXEXPORT void
BoxVMCode_Finish(BoxVMCode *p);

/**
 * @brief Allocate space in the heap to hold a #BoxVMCode object and initialise
 *   it with BoxVMCode_Init().
 */
BOXEXPORT BoxVMCode *
BoxVMCode_Create(BoxCmp *c, BoxVMCodeStyle style);

/**
 * @brief Destroy a #BoxVMCode object created with BoxVMCode_Create().
 */
BOXEXPORT void
BoxVMCode_Destroy(BoxVMCode *p);

/**
 * @breif Initialise the code in the procedure, coherently with the procedure
 *   style.
 */
BOXEXPORT void
BoxVMCode_Begin(BoxVMCode *p);

/**
 * @brief Finalise the code in the procedure, coherently with the procedure
 *   style.
 */
BOXEXPORT void
BoxVMCode_End(BoxVMCode *p);

/**
 * Set the prototype for the procedure. The prototype must be set for
 * procedures of type BOXVMCODESTYLE_SUB in order to be able to use the methods
 * BoxVMCode_Get_Child_Reg and BoxVMCode_Get_Parent_Reg.
 * @see BoxVMCode_Get_Child_Reg, BoxVMCode_Get_Parent_Reg
 */
void BoxVMCode_Set_Prototype(BoxVMCode *p, int have_child, int have_parent);

/** Get the VM-register for the parent (a fatal error occurs if the procedure
 * hasn't got a parent).
 */
BoxVMRegNum BoxVMCode_Get_Parent_Reg(BoxVMCode *p);

/** Get the VM-register for the child (a fatal error occurs if the procedure
 * hasn't got a child).
 */
BoxVMRegNum BoxVMCode_Get_Child_Reg(BoxVMCode *p);

/**
 * @brief Provides an alternative name for the procedure.
 *
 * This is usually the Box name of the procedure (such as <tt>Int@@Print</tt>)
 * and is the name displayed when disassembling the bytecode instruction call.
 * If it is not provided, then the procedure name is used (see
 * BoxVMCode_Set_Name()) or "|unknown|", if the name is not set either.
 */
void BoxVMCode_Set_Alter_Name(BoxVMCode *p, const char *alter_name);

/**
 * @brief Retrieve the alternative name for the procedure.
 */
char *BoxVMCode_Get_Alter_Name(BoxVMCode *p);

/**
 * @brief Get the procedure style (the parameter which was given during
 *   procedure creation).
 */
BOXEXPORT BoxVMCodeStyle
BoxVMCode_Get_Style(BoxVMCode *p);

/** Get the register allocator for the procedure */
RegAlloc *BoxVMCode_Get_RegAlloc(BoxVMCode *p);

/** Get the procedure ID, which is an integer number which the VM
 * allocates when it creates a BoxVMProc (which is just a bytecode container,
 * a place where bytecode can be written).
 * If such a number does not exist, then a procedure is created, which
 * is associated to this BoxVMCode object and the corresponding number
 * is returned.
 */
BoxVMProcID BoxVMCode_Get_ProcID(BoxVMCode *p);

/**
 * @brief Get the procedure call number.
 *
 * If the procedure doesn't have a call number then, it is installed and the
 * call number returned by such installation operation is returned.
 */
BOXEXPORT BoxVMCallNum
BoxVMCode_Get_Call_Num(BoxVMCode *p);

/**
 * @brief Install the procedure such that the code which calls it can be
 *   actually executed.
 */
BOXEXPORT BoxVMCallNum
BoxVMCode_Install(BoxVMCode *p);

/**
 * @brief Assemble a @c call instruction to call the procedure with the given
 *   call number.
 */
BOXEXPORT void
BoxVMCode_Assemble_Call(BoxVMCode *code, BoxVMCallNum call_num);

/** High level routine to assemble bytecode for the Box virtual machine (VM).
 */
void BoxVMCode_Assemble(BoxVMCode *p, BoxGOp g_op, int num_args, ...);

/** Create a new jump-label. The label can be used to assemble jump
 * instructions with BoxVMCode_Assemble_Jump, BoxVMCode_Assemble_CJump.
 * The label has to be defined using BoxVMCode_Jump_Label_Define.
 */
BoxVMSymID BoxVMCode_Jump_Label_New(BoxVMCode *p);

/**
 * @brief Define the label with the current position in the code.
 *
 * This is a shorthand for:
 * @code
 * jl = BoxVMCode_Jump_Label_New(p);
 * BoxVMCode_Jump_Label_Define(p, jl);
 * return jl;
 * @endcode
 */
BOXEXPORT BoxVMSymID
BoxVMCode_Jump_Label_Here(BoxVMCode *p);

/**
 * @brief Define an existing undefined label to point to the current position
 *   in the procedure.
 */
BOXEXPORT void
BoxVMCode_Jump_Label_Define(BoxVMCode *p, BoxVMSymID jl);

/**
 * @brief Release the given jump label. After this function has been called
 *   the jump label cannot be used anymore.
 */
BOXEXPORT void
BoxVMCode_Jump_Label_Release(BoxVMCode *p, BoxVMSymID jl);

/**
 * @brief Assemble a jump instruction to the given jump label.
 */
BOXEXPORT void
BoxVMCode_Assemble_Jump(BoxVMCode *p, BoxVMSymID jl);

/**
 * @brief Assemble a conditional jump instruction to the given jump label.
 */
BOXEXPORT void
BoxVMCode_Assemble_CJump(BoxVMCode *p, BoxVMSymID jl, BoxCont *cont);

/**
 * @brief State that the next code generated in the procedure will correspond
 *   to the given position in the source file.
 *
 * Use a NULL pointer to specify that such information is not known.
 */
BOXEXPORT void
BoxVMCode_Associate_Source(BoxVMCode *p, BoxSrcPos *src_pos);

#endif /* _BOX_BOXVMCODE_H */
