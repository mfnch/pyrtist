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
 * @brief Implementation of the #BoxVMCode object.
 */
struct BoxVMCode_struct {
  struct {
    unsigned int
               parent     :1,  /**< The procedure has a parent */
               parent_reg :1,  /**< The procedure uses the parent register. */
               child      :1,  /**< The procedure has a child */
               child_reg  :1,  /**< The procedure uses the child register. */
               reg_alloc  :1,  /**< it has automatic register allocation */
               proc_name  :1,  /**< it has a name */
               alter_name :1,  /**< has an alternative name */
               call_num   :1,  /**< it has a call number */
               installed  :1,  /**< The procedure was installed */
               callable   :1;
  } have;
  BoxVMCodeStyle
               style;       /**< Procedure style */
  BoxCmp       *cmp;        /**< Compiler corresponding to the procedure */
  RegAlloc     reg_alloc;   /**< Register allocator: keeps track of registers
                                 which are being used and of those which are
                                 not used.*/
  char         *proc_name,  /**< Procedure name */
               *alter_name; /**< Alternative name */
  BoxVMCallNum call_num;    /**< Call number (needed to call it from ASM) */
  BoxVMRegNum  reg_parent,  /**< Register number for the parent */
               reg_child;   /**< Register number for the child */
  BoxCallable  *callable;
};

BOX_BEGIN_DECLS

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

/**
 * @brief Install the procedure such that the code which calls it can be
 *   actually executed.
 */
BOXEXPORT BoxVMCallNum
BoxVMCode_Install(BoxVMCode *p);

BOX_END_DECLS

#endif /* _BOX_BOXVMCODE_H */
