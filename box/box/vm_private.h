/****************************************************************************
 * Copyright (C) 2008-2012 by Matteo Franchin                               *
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
 * @file vm_private.h
 * @brief Box virtual machine private definitions.
 */

#ifndef _BOX_VM_PRIVATE_H
#  define _BOX_VM_PRIVATE_H

#  include <stdio.h>
#  include <stdarg.h>
#  include <stdlib.h>

/*#  include <stdint.h>
#  include <inttypes.h>*/

#  include <box/types.h>
#  include <box/defaults.h>
#  include <box/array.h>
#  include <box/occupation.h>
#  include <box/hashtable.h>
#  include <box/vm.h>
#  include <box/vmproc.h>
#  include <box/vmsym.h>

/** To each type a number is associated. */
typedef BoxType Type;

/** Here is a list of builtin types. */
typedef enum {
  TYPE_NONE           = -1,
  TYPE_CHAR           =  0,
  TYPE_INT            =  1,
  TYPE_REAL           =  2,
  TYPE_POINT          =  3,
  TYPE_OBJ            =  4,
} TypeID;


typedef BoxOp BoxOpcode;

/** Structure used in BoxOpInfo to list the input and output registers
 * for each VM operation.
 */
typedef struct {
  char kind, /**< 'a' for explicit argument, 'r' for implicit local register */
       type, /**< 'c':Char, 'i':Int, 'r':Real, 'p':Point, 'o':Obj */
       num,  /**< Number of argument or register (can be 0, 1, 2) */
       io;   /**< 'o' for output, 'i' for input, 'b' for input/output */
} BoxOpReg;

/** Enumeration of all the possible types of signatures for the ops
 * (instructions of the Box VM). Different signatures mean different
 * number and/or type of arguments.
 */
typedef enum {
  BOXOPSIGNATURE_NONE,
  BOXOPSIGNATURE_ANY,
  BOXOPSIGNATURE_IMM,
  BOXOPSIGNATURE_ANY_ANY,
  BOXOPSIGNATURE_ANY_IMM
} BoxOpSignature;

/** Possible methods for disassembling a Box VM operation.
 * Each item in the enumeration corresponds to a different method to be used
 * for disassembling the operation.
 */
typedef enum {
  BOXOPDASM_ANY_ANY,
  BOXOPDASM_ANY_IMM,
  BOXOPDASM_JMP,
  BOXOPDASM_CALL
} BoxOpDAsm;

/** Prototype of function which implements a VM instruction. */
typedef void (*BoxVMOpExecutor)(BoxVMX *vmx);

/** Prototype of function which disassembles a VM instruction. */
typedef void (*BoxVMOpDisasm)(BoxVM *vm, char **out);

/** Prototype of function to retrieve the arguments of a VM instruction. */
typedef void (*BoxVMOpArgsGetter)(BoxVMX *vmx);

/** Typedef of struc __BoxOpInfo */
typedef struct BoxOpInfo_struct BoxOpInfo;

/** Structure containing detailed information about one VM operation */
struct BoxOpInfo_struct {
  BoxOp      opcode;       /**< Opcode for the operation */
  BoxGOp     g_opcode;     /**< Generic opcode */
  BoxOpInfo  *next;        /**< Next operation with the same generic opcode */
  const char *name;        /**< Literal name of the opcode (a string) */
  BoxOpSignature
             signature;    /**< Operation kind (depends on the arguments) */
  BoxOpDAsm  dasm;         /**< How to disassemble the operation */
  char       arg_type,     /**< Type of the arguments */
             num_args,     /**< Number of arguments */
             num_inputs,   /**< Num. of input registers (explicit+implicit) */
             num_outputs,  /**< Num. of output registers(explicit+implicit) */
             num_regs;     /**< Num. of distinct registers involved by the
                                operation (this is not just in + out) */
  BoxOpReg   *regs;        /**< Pointer to the list of input/output regs */
  BoxVMOpExecutor
             executor;    /**< Pointer to the function which implements
                               the operation */
};

/** Table containing info about all the VM operations */
typedef struct {
  BoxOpInfo info[BOX_NUM_OPS]; /**< Table of BoxOpInfo strucs. One for each
                                    VM operation */
  BoxOpReg  *regs;             /**< Buffer used by the BoxOpTable.info */
} BoxOpTable;

typedef enum {
  BOXOPCAT_NONE = -1,
  BOXOPCAT_GREG = 0,
  BOXOPCAT_LREG,
  BOXOPCAT_PTR,
  BOXOPCAT_IMM
} BoxOpCat;

/* Numero massimo degli argomenti di un'istruzione */
#  define VM_MAX_NUMARGS 2

/** Item used in a backtrace to identify where the exception caused the
 * particular function to exit.
 */
typedef struct {
  BoxVMCallNum call_num; /**< Call number of the function */
  size_t       vm_pos;   /**< Position in the VM code */
} BoxVMTrace;

/* This type is used in the table 'vm_instr_desc_table', which collects
 * and describes all the instruction of the VM.
 */
typedef struct {
  const char         *name;         /**< Nome dell'istruzione */
  BoxUInt            numargs;       /**< Numero di argomenti dell'istruzione */
  TypeID             t_id;          /**< Numero che identifica il tipo
                                         degli argomenti (interi, reali, ...) */
  BoxVMOpArgsGetter  get_args;      /**< Per trattare gli argomenti */
  BoxVMOpExecutor    execute;       /**< Per eseguire l'istruzione */
  BoxVMOpDisasm      disasm;        /**< Per disassemblare gli argomenti */

} BoxVMInstrDesc;

typedef struct {
  void   *ptr; /**< Pointer to the region allocated for the registers */
  BoxInt min,  /**< Min register number */
         max;  /**< Max register number */
} BoxVMRegs;

/** This structure contains all the data which define the status for the VM.
 * Status is allocated by 'VM_Module_Execute' inside its stack.
 */
struct BoxVMX_struct {
  BoxVM       *vm;        /**< VM to execute. */

  BoxVMProcInstalled *p;  /**< Procedure which is currently been executed */

  struct {
    unsigned int
              error   :1, /**< Error detected */
              exit    :1, /**< Exit current execution frame */
              is_long :1; /**< Instruction is in long format */

  } flags;                /**< Execution flags */

  BoxVMWord   *i_pos,     /**< Pointer to the current instruction */
              i_eye;      /**< Execution "eye" (last four bytes processed) */
  BoxUInt     i_type,     /**< Type of instruction */
              i_len,      /**< Size of instruction */
              arg_type;   /**< Type of arguments of instruction */

  const BoxVMInstrDesc
              *idesc;     /**< Descriptor for current instruction */
  void        *arg1,
              *arg2;      /**< Pointer to instruction arguments */

  BoxVMRegs   local[NUM_TYPES], /**< Local register allocation status */
              *global;          /**< Global register allocation status */

  BoxInt      alc[NUM_TYPES]; /**< Allocation status of local registers
                                   (whether a 'new num_regs, num_vars'
                                   instruction has been used) */
};

/** @brief The full status of the virtual machine of Box. */
struct BoxVM_struct {
  BoxVMX  *vmcur;          /**< The current execution frame */

  struct {
    unsigned int
          forcelong :1,    /**< Force long form assembly. */
          hexcode   :1,    /**< Show Hex values in disassembled code */
          identdata :1;    /**< Add also identity info for data inserted
                                  into the data segment */
  }       attr;            /** Flags controlling the behaviour of the VM */

  struct {
    unsigned int
          globals   :1,    /**< Global regs have been allocated */
          op_table  :1;    /**< The operation table has been built */

  }       has;             /**< State of the VM */

  BoxArr  stack,           /**< The stack for the VM object */
          data_segment;    /**< The segment of data (strings, etc.) which is
                                accessible through the register gro0 */

  BoxVMRegs global[NUM_TYPES];  /**< The values of the global registers */

  BoxPtr    *box_vm_current,
            *box_vm_arg1,
            *box_vm_arg2;

  const BoxVMInstrDesc
            *exec_table;    /**< Table collecting info about the instructions
                                 which are useful for execution. */

  BoxVMProcTable
            proc_table;     /**< Table of installed and uninstalled procs */

  BoxVMSymTable
            sym_table;      /**< Table of referenced and defined symbols */

  BoxOpTable
            op_table;       /**< Table describing the instructions and their
                                 properties */

  BoxArr    backtrace;      /**< Information about error location */
  char      *fail_msg;      /**< Failure message */

  BoxUInt   dasm_pos;       /**< Position in num. of read bytes for the
                                 disassembler */

  BoxHT     id_from_desc;   /**< Hashtable containing object descriptors */
  BoxArr    desc_from_id;   /**< Table of descriptors from allocation IDs */
};

extern const size_t size_of_type[NUM_TYPES];

/** Maximum num of arguments (implicit + explicit) that an operation
 * can have
 */
#define BOXOP_MAX_NUM_ARGS 4

/** Build a table containing info on the Box VM operations for each of them.
 * The table is addressable using a BoxGOp as index.
 * This is quite an internal function.
 */
void BoxOpTable_Build(BoxOpTable *ot);

/** Destroy a BoxOpTable object created with BoxOpTable_Build */
void BoxOpTable_Destroy(BoxOpTable *ot);

/** Print the given BoxOpTable to the given stream. */
void BoxOpTable_Print(FILE *out, BoxOpTable *ot);

/** Get information about the specified generic VM operation. */
BoxOpInfo *BoxVM_Get_Op_Info(BoxVM *vm, BoxGOp g_op);

/** Print all the signatures for a the given BoxOpInfo object. */
void BoxOpInfo_Print(FILE *out, BoxOpInfo *oi);

/** (Internal) Get the execution table for the Box VM instructions. */
const BoxVMInstrDesc *BoxVM_Get_Exec_Table(void);

/* These functions are intended to be used only inside 'vmexec.h' */
void VM__GLP_GLPI(BoxVMX *vmx);
void VM__GLP_Imm(BoxVMX *vmx);
void VM__GLPI(BoxVMX *vmx);
void VM__Imm(BoxVMX *vmx);
void VM__D_GLPI_GLPI(BoxVM *vmp, char **iarg);
void VM__D_CALL(BoxVM *vmp, char **iarg);
void VM__D_JMP(BoxVM *vmp, char **iarg);
void VM__D_GLPI_Imm(BoxVM *vmp, char **iarg);

/** This is the type of the C functions which can be called by the VM. */
typedef BoxTask (*BoxVMFunc)(BoxVM *);

/** Initialise a BoxVM object for which space has been already allocated
 * somehow. You'll need to use BoxVM_Finish to destroy the object.
 * @see BoxVM_Finish, BoxVM_Create
 */
BoxTask BoxVM_Init(BoxVM *vm);

/** Destroy a BoxVM object initialised with BoxVM_Init
 * @see BoxVM_Init
 */
void BoxVM_Finish(BoxVM *vm);

/** Provide a failure message for a raised exception. */
BOXEXPORT void BoxVM_Set_Fail_Msg(BoxVM *vm, const char *msg);

BOXEXPORT BoxTask BoxVM_Module_Execute(BoxVMX *vmx, BoxVMCallNum call_num);

/** Similar to BoxVM_Module_Execute, but takes also pointers to child
 * and parent. The register gro1 and gro2 are modified after this call: in
 * particular, '*parent' is stored in gro1 and '*child' in gro2.
 * This guarantee that the reference counting protocol is respected.
 */
BOXEXPORT BoxTask
  BoxVM_Module_Execute_With_Args(BoxVMX *vmx, BoxVMCallNum cn,
                                 BoxPtr *parent, BoxPtr *child);


/** Clear the backtrace of the program. */
void BoxVM_Backtrace_Clear(BoxVM *vm);

/** Print on 'stream' a human redable representation of the backtrace
 * of the program.
 */
void BoxVM_Backtrace_Print(BoxVM *vm, FILE *stream);

/** Get the parent of the current combination (this is something with type
 * ``BoxPtr *``.
 */
#  define BoxVM_Get_Parent(vm) ((vm)->box_vm_current)

/** Get the child of the current combination (this is something with type
 * ``BoxPtr *`` 
 */
#  define BoxVM_Get_Child(vm) ((vm)->box_vm_arg1)

/** Shorthand for BoxPtr_Get_Target(BoxVM_Get_Parent(vm)). */
#  define BoxVM_Get_Parent_Target(vm) (BoxPtr_Get_Target(BoxVM_Get_Parent(vm)))

/** Shorthand for BoxPtr_Get_Target(BoxVM_Get_Child(vm)). */
#  define BoxVM_Get_Child_Target(vm) (BoxPtr_Get_Target(BoxVM_Get_Child(vm)))

#  define BOX_VM_THIS_PTR(vmp, Type) ((Type *) (vmp)->box_vm_current->ptr)
#  define BOX_VM_THIS(vmp, Type) (*BOX_VM_THIS_PTR(vmp, Type))
#  define BOX_VM_ARG1_PTR(vmp, Type) ((Type *) (vmp)->box_vm_arg1->ptr)
#  define BOX_VM_ARG1(vmp, Type) (*BOX_VM_ARG1_PTR(vmp, Type))
#  define BOX_VM_ARG_PTR BOX_VM_ARG1_PTR
#  define BOX_VM_ARG BOX_VM_ARG1
#  define BOX_VM_ARG2_PTR(vmp, Type) ((Type *) *(vmp)->box_vm_arg2)
#  define BOX_VM_ARG2(vmp, Type) (*BOX_VM_ARG2_PTR(vmp, Type))

#  define BOX_VM_THIS_OBJ(vmp) ((vmp)->box_vm_current)
#  define BOX_VM_ARG1_OBJ(vmp) ((vmp)->box_vm_arg1)

/* Subtype-related macros */
#  define BOX_VM_SUB_PARENT_PTR(vmp, parent_t) \
   SUBTYPE_PARENT_PTR(BOX_VM_THIS_PTR(vmp, Subtype), parent_t)
#  define BOX_VM_SUB_PARENT(vmp, parent_t) \
   (*BOX_VM_SUB_PARENT_PTR(vmp, parent_t))
#  define BOX_VM_SUB_CHILD_PTR(vmp, child_t) \
   SUBTYPE_CHILD_PTR(BOX_VM_THIS_PTR(vmp, Subtype), child_t)
#  define BOX_VM_SUB_CHILD(vmp, child_t) \
   (*BOX_VM_SUB_CHILD_PTR(vmp, child_t))
#  define BOX_VM_SUB2_PARENT_PTR(vmp, parent_t) \
   SUBTYPE_PARENT_PTR(BOX_VM_SUB_PARENT_PTR(vmp, Subtype), parent_t)
#  define BOX_VM_SUB2_PARENT(vmp, parent_t) \
   (*BOX_VM_SUB2_PARENT_PTR(vmp, parent_t))
#  define BOX_VM_SUB2_CHILD_PTR(vmp, child_t) \
   SUBTYPE_CHILD_PTR(BOX_VM_SUB_PARENT_PTR(vmp, Subtype), child_t)
#  define BOX_VM_SUB2_CHILD(vmp, child_t) \
   (*BOX_VM_SUB2_CHILD_PTR(vmp, child_t))

#endif /* _BOX_VM_PRIVATE_H */
