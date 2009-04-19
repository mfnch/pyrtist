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

/**
 * @file virtmach.h
 * @brief The virtual machine of Box.
 */

#ifndef _VIRTMACH_H
#  define _VIRTMACH_H

#  include <stdio.h>
#  include <stdarg.h>
/*#  include <stdint.h>
#  include <inttypes.h>*/

#  include <box/types.h>
#  include <box/defaults.h>
#  include <box/array.h>
#  include <box/occupation.h>
#  include <box/hashtable.h>

#  define _INSIDE_VIRTMACH_H
#  include <box/vmproc.h>
#  include <box/vmsym.h>
#  undef _INSIDE_VIRTMACH_H

/* Data type used to write/read binary codes for the instructions */
typedef unsigned char BoxVMByte;
typedef char BoxVMSByte;
typedef unsigned long BoxVMByteX4;
#  define BoxVMByteX4_Fmt "%8.8lx"

#  ifdef BOX_ABBREV
typedef BoxVMByteX4 VMByteX4;
#    define VMByteX4_Fmt BoxVMByteX4_Fmt
#  endif

/** To each type a number is associated. */
typedef BoxInt Type;

/** Here is a list of builtin types. */
typedef enum {
  TYPE_NONE           = -1,
  TYPE_FAST_FIRST     =  0,
  TYPE_CHAR           =  0,
  TYPE_INTG           =  1,
  TYPE_INT            =  1,
  TYPE_REAL           =  2,
  TYPE_POINT          =  3,
  TYPE_OBJ            =  4,
  TYPE_FAST_LAST      =  4,
  TYPE_VOID           =  5,
  TYPE_OPEN           =  6,
  TYPE_CLOSE          =  7,
  TYPE_PAUSE          =  8,
  TYPE_DESTROY        =  9,
  TYPE_ITER           = 10,
  TYPE_PTR            = 11,
  TYPE_IF             = 12,
  TYPE_FOR            = 13
} TypeID;

/* Enumero gli header di istruzione della macchina virtuale.
 * ATTENZIONE: L'ordine nella seguente enumerazione deve rispettare l'ordine
 *  nella tabella dei descrittori di istruzione ( vm_instr_desc_table[] )
 * NUMERO ISTRUZIONI: 78
 */
typedef enum {
  ASM_LINE_Iimm=1, ASM_CALL_I, ASM_CALL_Iimm,
  ASM_NEWC_II, ASM_NEWI_II, ASM_NEWR_II, ASM_NEWP_II, ASM_NEWO_II,
  ASM_MOV_Cimm, ASM_MOV_Iimm, ASM_MOV_Rimm, ASM_MOV_Pimm,
  ASM_MOV_CC, ASM_MOV_II, ASM_MOV_RR, ASM_MOV_PP, ASM_MOV_OO,
  ASM_BNOT_I, ASM_BAND_II, ASM_BXOR_II, ASM_BOR_II,
  ASM_SHL_II, ASM_SHR_II,
  ASM_INC_I, ASM_INC_R, ASM_DEC_I, ASM_DEC_R,
  ASM_POW_II, ASM_POW_RR,
  ASM_ADD_II, ASM_ADD_RR, ASM_ADD_PP, ASM_SUB_II, ASM_SUB_RR, ASM_SUB_PP,
  ASM_MUL_II, ASM_MUL_RR, ASM_DIV_II, ASM_DIV_RR, ASM_REM_II,
  ASM_NEG_I, ASM_NEG_R, ASM_NEG_P, ASM_PMULR_P, ASM_PDIVR_P,
  ASM_EQ_II, ASM_EQ_RR, ASM_EQ_PP, ASM_NE_II, ASM_NE_RR, ASM_NE_PP,
  ASM_LT_II, ASM_LT_RR, ASM_LE_II, ASM_LE_RR,
  ASM_GT_II, ASM_GT_RR, ASM_GE_II, ASM_GE_RR,
  ASM_LNOT_I, ASM_LAND_II, ASM_LOR_II,
  ASM_REAL_C, ASM_REAL_I, ASM_INTG_R, ASM_POINT_II, ASM_POINT_RR,
  ASM_PROJX_P, ASM_PROJY_P, ASM_PPTRX_P, ASM_PPTRY_P,
  ASM_RET,
  ASM_MALLOC_I, ASM_MLN_O, ASM_MUNLN_O, ASM_MCOPY_OO,
  ASM_LEA_C, ASM_LEA_I, ASM_LEA_R, ASM_LEA_P, ASM_LEA_OO,
  ASM_PUSH_O, ASM_POP_O,
  ASM_JMP_I, ASM_JC_I,
  ASM_ADD_O,
  ASM_ARINIT, ASM_ARSIZE, ASM_ARADDR, ASM_ARGET, ASM_ARNEXT, ASM_ARDEST,
  ASM_ILLEGAL
} AsmCode;

typedef AsmCode BoxOpcode;

/* Enumerazione delle categorie di argomento, utilizzata da Asm_Assemble
 * per assemblare le istruzioni.
 * NOTA: Questa enumerazione deve essere coerente con l'ordine dell'array
 *  vm_gets[].
 */
typedef enum {CAT_NONE = -1, CAT_GREG = 0, CAT_LREG, CAT_PTR, CAT_IMM} AsmArg;

/* Associo un numero a ciascun tipo, per poterlo identificare */
typedef enum {
  SIZEOF_CHAR  = sizeof(BoxChar),
  SIZEOF_INTG  = sizeof(BoxInt),
  SIZEOF_REAL  = sizeof(BoxReal),
  SIZEOF_POINT = sizeof(BoxPoint),
  SIZEOF_OBJ   = sizeof(BoxObj), SIZEOF_PTR = SIZEOF_OBJ
} SizeOfType;

/* Numero massimo degli argomenti di un'istruzione */
#  define VM_MAX_NUMARGS 2

/* The struct __vmprogram and __vmstatus contain pointers to functions whose
 * arguments are the structures themselves. This requires a trick.
 * First we declare the structures and we use a macro to make the function
 * declarations more natural. Later in this file we undef the macros
 * and typedef the structures (to VMProgram and VMStatus).
 * Silly, but - at least - this trick should not affect areas outside
 * this header file.
 */
struct __vmprogram;
struct __vmstatus;

#  define BoxVM struct __vmprogram
#  define VMStatus struct __vmstatus

/* This type is used in the table 'vm_instr_desc_table', which collects
 * and describes all the instruction of the VM.
 */
typedef struct {
  const char *name;             /* Nome dell'istruzione */
  BoxUInt numargs;              /* Numero di argomenti dell'istruzione */
  TypeID t_id;                  /* Numero che identifica il tipo
                                   degli argomenti (interi, reali, ...) */
  void (*get_args)(VMStatus *); /* Per trattare gli argomenti */
  void (*execute)(BoxVM *); /* Per eseguire l'istruzione */
  /* Per disassemblare gli argomenti */
  void (*disasm)(BoxVM *, char **);
} VMInstrDesc;

/** This structure contains all the data which define the status for the VM.
 * Status is allocated by 'VM_Module_Execute' inside its stack.
 */
struct __vmstatus {
  /* Flags della VM */
  struct {
    unsigned int error    : 1; /* L'istruzione ha provocato un errore! */
    unsigned int exit     : 1; /* Bisogna uscire dall'esecuzione! */
    unsigned int is_long  : 1; /* L'istruzione e' in formato lungo? */
  } flags;

  BoxInt line;           /**< Number of line, as set by the 'line' instruction */
  BoxVMProcInstalled *p; /**< Procedure which is currently been executed */

  /* Variabili che riguardano l'istruzione in esecuzione: */
  BoxUInt dasm_pos;       /* Position in num. of read bytes for the disassembler */
  BoxVMByteX4 *i_pos;     /* Puntatore all'istruzione */
  BoxVMByteX4 i_eye;      /* Occhio di lettura (gli ultimi 4 byte letti) */
  BoxUInt i_type,         /* Tipo e dimensione dell'istruzione */
          i_len,
          arg_type;       /* Tipo degli argomenti dell'istruzione */
  VMInstrDesc *idesc;     /* Descrittore dell'istruzione corrente */
  void *arg1, *arg2;      /* Puntatori agli argomenti del'istruzione */
  void *global[NUM_TYPES]; /* Array di puntatori alle zone registri globali e locali */
  void *local[NUM_TYPES];
  BoxInt gmin[NUM_TYPES],  /* Numero di registro globale minimo e massimo */
         gmax[NUM_TYPES],
         lmin[NUM_TYPES],  /* Numero di registro locale minimo e massimo */
         lmax[NUM_TYPES];
  /* Stato di allocazione dei registri per il modulo in esecuzione */
  BoxInt alc[NUM_TYPES];
};

/* Here we undef the VMStatus macro and typedef __vmstatus to VMStatus. */
#undef VMStatus
typedef struct __vmstatus VMStatus;

/** @brief The full status of the virtual machine of Box.
 */
struct __vmprogram {
  BoxVMSymTable  sym_table;    /**< Table of referenced and defined symbols */
  BoxVMProcTable proc_table;   /**< Table of installed and uninstalled procs */
  BoxHT          method_table; /**< Hashtable containing destructors, etc. */
  BoxArr         stack,        /**< The stack for the VM object */
                 data_segment; /**< The segment of data (strings, etc.)
                                    which is accessible through the global
                                    register gr0 */

  /** Flags which control the behaviour of the VM. */
  struct {
    unsigned int
              forcelong : 1,   /** Force long form assembly. */
              hexcode : 1,     /** Show Hex values in disassembled code */
              identdata : 1;   /** Add also identity info for data inserted
                                   into the data segment */
  } attr;

  int vm_globals;
  void *vm_global[NUM_TYPES];
  BoxInt vm_gmin[NUM_TYPES], vm_gmax[NUM_TYPES];
  Obj *box_vm_current, *box_vm_arg1, *box_vm_arg2;
  /** Array used with sprintf, when arguments are disassembled. */
  char iarg_str[VM_MAX_NUMARGS][64];

  VMStatus *vmcur;
};

#  undef BoxVM
typedef struct __vmprogram BoxVM;
typedef BoxVM VMProgram;

extern VMInstrDesc vm_instr_desc_table[];

extern const BoxUInt size_of_type[NUM_TYPES];

/* These functions are intended to be used only inside 'vmexec.h' */
void VM__GLP_GLPI(VMStatus *vmcur);
void VM__GLP_Imm(VMStatus *vmcur);
void VM__GLPI(VMStatus *vmcur);
void VM__Imm(VMStatus *vmcur);
void VM__D_GLPI_GLPI(BoxVM *vmp, char **iarg);
void VM__D_CALL(BoxVM *vmp, char **iarg);
void VM__D_JMP(BoxVM *vmp, char **iarg);
void VM__D_GLPI_Imm(BoxVM *vmp, char **iarg);

/** This is the type of the C functions which can be called by the VM. */
typedef BoxTask (*VMFunc)(BoxVM *);



/** Initialise a BoxVM object for which space has been already allocated
 * somehow. You'll need to use BoxVM_Finish to destroy the object.
 * @see BoxVM_Finish, BoxVM_New
 */
BoxTask BoxVM_Init(BoxVM *vm);

/** Destroy a BoxVM object initialised with BoxVM_Init
 * @see BoxVM_Init
 */
void BoxVM_Finish(BoxVM *vm);

/** Allocate space for a BoxVM object and initialise it with BoxVM_Init.
 * You'll need to call BoxVM_Destroy to destroy the object.
 * @see BoxVM_Destroy, BoxVM_Init
 */
BoxVM *BoxVM_New(void);

/** Destroy a BoxVM object created with BoxVM_New
 * @see BoxVM_New
 */
void BoxVM_Destroy(BoxVM *vm);

/** Specifies the number of global registers and variables used by the BoxVM.
 */
BoxTask BoxVM_Alloc_Global_Regs(BoxVM *vm, BoxInt num_var[], BoxInt num_reg[]);

BoxTask VM_Module_Global_Set(BoxVM *vmp, BoxInt type, BoxInt reg, void *value);

BoxTask VM_Code_Prepare(BoxVM *vmp, BoxInt *num_var, BoxInt *num_reg);
BoxTask VM_Module_Execute(BoxVM *vmp, unsigned int call_num);

/** The attributes corresponding to different behaviours of the Box virtual
 * machine.
 * @see BoxVM_Set_Attr
 */
typedef enum {
  BOXVM_ATTR_ASM_LONG_FMT=1,  /**< Use long format when assembling code */
  BOXVM_ATTR_DASM_WITH_HEX=2, /**< Show also hex values when disassembling */
  BOXVM_ATTR_ADD_DATA_IDENT=4 /**< Add identity info (debug) to data blocks */
} BoxVMAttr;

/** Set or unset the attributes which control the behaviour of the Box
 * virtual machine.
 * @param vmp an instance of the Box virtual machine
 * @param mask specify what values to set/unset
 * @param value specify to set/unset the values specified in mask
 * @see BoxVMAttr
 */
void BoxVM_Set_Attr(BoxVM *vm, BoxVMAttr mask, BoxVMAttr value);

/** Sets the force-long flag and return what was its previous value. */
int BoxVM_Set_Force_Long(BoxVM *vm, int force_long);

BoxTask VM_Disassemble(BoxVM *vmp, FILE *output, void *prog, BoxUInt dim);


void VM_ASettings(BoxVM *vmp, int forcelong, int error, int inhibit);

void VM_VA_Assemble(BoxVM *vm, AsmCode instr, va_list ap);

void VM_Assemble(BoxVM *vmp, AsmCode instr, ...);

/** Add the block of data pointed by 'data' with size 'size'
 * to the data segment for the VM instance 'vm'.
 */
BoxUInt BoxVM_Data_Add(BoxVM *vm, const void *data, BoxUInt size,
                       BoxInt type);

/** Produce a human readable representation of the data segment of 'vm'
 * and send it to the output stream 'stream'.
 */
void BoxVM_Data_Display(BoxVM *vm, FILE *stream);

/** Similar to VM_Assemble, but use the long bytecode format. */
#define VM_Assemble_Long(vmp, instr, ...) { \
  int is_long = BoxVM_Set_Force_Long(vmp, 1); \
  VM_Assemble(vmp, instr, __VA_ARGS__); \
  (void) BoxVM_Set_Force_Long(vmp, is_long);}

/* Numero minimo di BoxVMByteX4 che riesce a contenere tutti i tipi possibili
 * di argomenti (Int, Real, Point, Obj)
 */
#  define MAX_SIZE_IN_IWORDS \
   ((sizeof(Point) + sizeof(BoxVMByteX4) - 1) / sizeof(BoxVMByteX4))

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

/* These are obsolete macros */
#  define BOX_VM_CURRENT BOX_VM_THIS
#  define BOX_VM_CURRENTPTR BOX_VM_THIS_PTR
#  define BOX_VM_ARGPTR1 BOX_VM_ARG1_PTR
#  define BOX_VM_ARGPTR2 BOX_VM_ARG2_PTR

#endif
