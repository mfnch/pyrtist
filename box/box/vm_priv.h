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
 * @file vm_priv.h
 * @brief Box virtual machine private definitions.
 */

#ifndef _BOX_VM_PRIVATE_H
#  define _BOX_VM_PRIVATE_H

#  include <stdio.h>
#  include <stdarg.h>
#  include <stdlib.h>

#  include <box/types.h>
#  include <box/defaults.h>
#  include <box/array.h>
#  include <box/occupation.h>
#  include <box/hashtable.h>
#  include <box/vm.h>
#  include <box/vmsym.h>
#  include <box/vmdasm.h>
#  include <box/vmproc_priv.h>

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

/**
 * Content of a BoxVMInstrDesc object.
 */
struct BoxVMInstrDesc_struct {
  const char         *name;         /**< Instruction name */
  BoxUInt            numargs;       /**< Number of arguments */
  BoxTypeId          t_id;          /**< Type of the arguments (all have the
                                         same type) */
  BoxVMOpArgsGetter  get_args;      /**< Per trattare gli argomenti */
  BoxVMOpExecutor    execute;       /**< Per eseguire l'istruzione */
  BoxVMOpDisasm      disasm;        /**< Per disassemblare gli argomenti */
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
            *box_vm_arg1;

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

/** This is the type of the C functions which can be called by the VM. */
typedef BoxTask (*BoxVMFunc)(BoxVMX *);

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
BOXEXPORT void BoxVMX_Set_Fail_Msg(BoxVMX *vm, const char *msg);

BOXEXPORT BoxTask BoxVM_Module_Execute(BoxVMX *vmx, BoxVMCallNum call_num);

/** Similar to BoxVM_Module_Execute, but takes also pointers to child
 * and parent. The register gro1 and gro2 are modified after this call: in
 * particular, '*parent' is stored in gro1 and '*child' in gro2.
 * This guarantee that the reference counting protocol is respected.
 */
BOXEXPORT BoxTask
  BoxVM_Module_Execute_With_Args(BoxVMX *vmx, BoxVMCallNum cn,
                                 BoxPtr *parent, BoxPtr *child);

/**
 * Clear the backtrace of the program.
 */
void BoxVM_Backtrace_Clear(BoxVM *vm);

/**
 * Print on 'stream' a human redable representation of the backtrace
 * of the program.
 */
void BoxVM_Backtrace_Print(BoxVM *vm, FILE *stream);

/* Read the first 4 bytes (BoxVMWord), extract the format bit and put the "rest"
 * in the i_eye (which should be defined as 'register BoxVMWord i_eye;')
 * is_long tells if the instruction is encoded with the packed format (4 bytes)
 * or with the long format (> 4 bytes). To read / write an instruction
 * represented with the packed format one should use the macros
 * ASM_SHORT_GET_* / ASM_SHORT_PUT_*. To read / write an instruction written
 * with the long format one should use instead the macros MY_ASM_LONG_GET_* /
 * MY_ASM_LONG_PUT_*
 */
#define BOXVM_READ_OP_FORMAT(i_pos, i_eye, is_long) \
  do {i_eye = *(i_pos++); is_long = (i_eye & 0x1); i_eye >>= 1;} while(0)

/* SHORT INSTRUCTION: we assemble the istruction header in the following way:
 * (note: 1 is represented with bit 0 = 1 and all other bits = 0)
 *  bit 0: true if the instruction is long
 *  bit 1-4: type of arguments
 *  bit 5-7: length of instruction
 *  bit 8-15: type of instruction
 *  (bit 16-23: left empty for argument 1)
 *  (bit 24-31: left empty for argument 2)
 */
#define BOXVM_WRITE_SHORTOP_HEADER(i_pos, i_eye, i_type, is_long, i_len, \
                                   arg_type) \
  do {i_eye = (((i_type) & 0xff) << 3 | ((i_len)  & 0x7)) << 4 \
              | ((arg_type) & 0xf);                            \
      i_eye = i_eye << 1 | ((is_long) & 0x1);} while(0)

#define BOXVM_WRITE_SHORTOP_1ARG(i_pos, i_eye, arg) \
  do {*((i_pos)++) = ((arg) & 0xff) << 16 | (i_eye);} while(0)

#define BOXVM_WRITE_SHORTOP_2ARGS(i_pos, i_eye, arg1, arg2) \
  do {*(i_pos++) = i_eye = \
      (((arg2) & 0xff)<<8 | ((arg1) & 0xff))<<16 | i_eye;} while(0)

#define BOXVM_READ_SHORTOP_HEADER(i_pos, i_eye, i_type, i_len, arg_type) \
  do {arg_type = i_eye & 0xf; i_len = (i_eye >>= 4) & 0x7; \
      i_type = (i_eye >>= 3) & 0xff; } while(0)

#define BOXVM_READ_SHORTOP_1ARG(i_pos, i_eye, arg) \
  do {arg = (signed char) ((i_eye >>= 8) & 0xff);} while(0)

#define BOXVM_READ_SHORTOP_2ARGS(i_pos, i_eye, arg1, arg2) \
  do {arg1 = (signed char) ((i_eye >>= 8) & 0xff); \
      arg2 = (signed char) ((i_eye >>= 8) & 0xff);} while(0)

/* LONG INSTRUCTION: we assemble the istruction header in the following way:
 *  FIRST FOUR BYTES:
 *    bit 0: true if the instruction is long
 *    bit 1-4: type of arguments
 *    bit 5-31: length of instruction
 *  SECOND FOUR BYTES:
 *    bit 0-31: type of instruction
 *  (THIRD FOUR BYTES: argument 1)
 *  (FOURTH FOUR BYTES: argument 2)
 */
#define BOXVM_WRITE_LONGOP_HEADER(i_pos, i_eye, i_type, is_long, i_len, \
                                  arg_type)                             \
  do {i_eye = ((i_len) & 0x07ff)<<4 | ((arg_type) & 0xf);               \
      i_eye = i_eye<<1 | ((is_long) & 0x1);                             \
      *(i_pos++) = i_eye; *(i_pos++) = i_type;} while(0)

#define BOXVM_READ_LONGOP_HEADER(i_pos, i_eye, i_type, i_len, arg_type) \
  do {(arg_type) = (i_eye) & 0xf; i_len = ((i_eye) >>= 4);              \
      (i_type) = *((i_pos)++);} while(0)

#define BOXVM_READ_LONGOP_1ARG(i_pos, i_eye, arg) \
  do {arg = i_eye = *((i_pos)++);} while(0)

#define BOXVM_READ_LONGOP_2ARGS(i_pos, i_eye, arg1, arg2) \
  do {arg1 = *(i_pos++); arg2 = i_eye = *(i_pos++);} while(0)

#define BOXVM_READ_OP_HEADER(op_ptr, op_word, op_type, op_size,         \
                             op_arg_type, op_is_long)                   \
  do {                                                                  \
    BOXVM_READ_OP_FORMAT((op_ptr), (op_word), (op_is_long));            \
    if ((op_is_long))                                                   \
      BOXVM_READ_LONGOP_HEADER((op_ptr), (op_word), (op_type),          \
                               (op_size), (op_arg_type));               \
    else                                                                \
      BOXVM_READ_SHORTOP_HEADER((op_ptr), (op_word), (op_type),         \
                                (op_size), (op_arg_type));              \
  } while(0)



#if 0
#define BOXVM_READ_SHORTOP_HEADER(i_pos, i_eye, i_type, i_len, arg_type) \
  do {arg_type = i_eye & 0xf; i_len = (i_eye >>= 4) & 0x7; \
      i_type = (i_eye >>= 3) & 0xff; } while(0)
#endif








/** Get the parent of the current combination (this is something with type
 * ``BoxPtr *``.
 */
#  define BoxVMX_Get_Parent(vmx) ((vmx)->vm->box_vm_current)

/** Get the child of the current combination (this is something with type
 * ``BoxPtr *`` 
 */
#  define BoxVMX_Get_Child(vmx) ((vmx)->vm->box_vm_arg1)

/** Shorthand for BoxPtr_Get_Target(BoxVM_Get_Parent(vm)). */
#  define BoxVMX_Get_Parent_Target(vmx) \
  (BoxPtr_Get_Target(BoxVMX_Get_Parent(vmx)))

/** Shorthand for BoxPtr_Get_Target(BoxVM_Get_Child(vm)). */
#  define BoxVMX_Get_Child_Target(vmx) \
  (BoxPtr_Get_Target(BoxVMX_Get_Child(vmx)))


/* XXX TODO: the ones below are obsolete macros, which should be removed. */
#  define BOX_VM_THIS_PTR(vmx, Type) ((Type *) (vmx)->vm->box_vm_current->ptr)
#  define BOX_VM_THIS(vmx, Type) (*BOX_VM_THIS_PTR(vmx, Type))
#  define BOX_VM_ARG1_PTR(vmx, Type) ((Type *) (vmx)->vm->box_vm_arg1->ptr)
#  define BOX_VM_ARG1(vmx, Type) (*BOX_VM_ARG1_PTR(vmx, Type))
#  define BOX_VM_ARG_PTR BOX_VM_ARG1_PTR
#  define BOX_VM_ARG BOX_VM_ARG1
#  define BOX_VM_SUB_PARENT_PTR(vmp, parent_t) \
   SUBTYPE_PARENT_PTR(BOX_VM_THIS_PTR(vmp, Subtype), parent_t)
#  define BOX_VM_SUB_PARENT(vmp, parent_t) \
   (*BOX_VM_SUB_PARENT_PTR(vmp, parent_t))
#  define BOX_VM_SUB_CHILD_PTR(vmp, child_t) \
   SUBTYPE_CHILD_PTR(BOX_VM_THIS_PTR(vmp, Subtype), child_t)
#  define BOX_VM_SUB_CHILD(vmp, child_t) \
   (*BOX_VM_SUB_CHILD_PTR(vmp, child_t))
#  define BOX_VM_SUB2_PARENT(vmp, parent_t) \
  (*SUBTYPE_PARENT_PTR(BOX_VM_SUB_PARENT_PTR(vmp, Subtype), parent_t))

#endif /* _BOX_VM_PRIVATE_H */
