/***************************************************************************
 *   Copyright (C) 2006 by Matteo Franchin                                 *
 *   fnch@libero.it                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _VIRTMACH_H
#define _VIRTMACH_H

#include <stdio.h>
#include "types.h"
#include "defaults.h"
#include "array.h"

/* Associo un numero a ciascun tipo, per poterlo identificare */
typedef enum {
  TYPE_NONE  = -1,
  TYPE_CHAR  =  0,
  TYPE_INTG  =  1,
  TYPE_REAL  =  2,
  TYPE_POINT =  3,
  TYPE_OBJ   =  4,
  TYPE_FIRST_USER_DEF = 5,
  TYPE_VOID  =  5,
  TYPE_OPEN  =  6,
  TYPE_CLOSE =  7,
  TYPE_PAUSE =  8,
  TYPE_DESTROY= 9
} TypeID;

/* Enumero gli header di istruzione della macchina virtuale.
 * ATTENZIONE: L'ordine nella seguente enumerazione deve rispettare l'ordine
 *  nella tabella dei descrittori di istruzione ( vm_instr_desc_table[] )
 * NUMERO ISTRUZIONI: 77
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
  ASM_MALLOC_I, ASM_MFREE_O, ASM_MCOPY_OO,
  ASM_LEA_C, ASM_LEA_I, ASM_LEA_R, ASM_LEA_P, ASM_LEA_OO,
  ASM_ILLEGAL
} AsmCode;

/* Enumerazione delle categorie di argomento, utilizzata da Asm_Assemble
 * per assemblare le istruzioni.
 * NOTA: Questa enumerazione deve essere coerente con l'ordine dell'array
 *  vm_gets[].
 */
typedef enum {CAT_NONE = -1, CAT_GREG = 0, CAT_LREG, CAT_PTR, CAT_IMM} AsmArg;

/* Data type used to write/read binary codes for the instructions */
typedef unsigned char VMByte;
typedef unsigned long VMByteX4;
typedef signed char VMSByte;

/* Definisco il tipo Obj, che e' semplicemente il tipo puntatore */
typedef void *Obj;

/* This is an union of all possible types */
typedef union {
  Char c;
  Intg i;
  Real r;
  Point p;
  Obj o;
} Generic;

/* Associo un numero a ciascun tipo, per poterlo identificare */
typedef enum {
  SIZEOF_CHAR  = sizeof(Char),
  SIZEOF_INTG  = sizeof(Intg),
  SIZEOF_REAL  = sizeof(Real),
  SIZEOF_POINT = sizeof(Point),
  SIZEOF_OBJ   = sizeof(Obj), SIZEOF_PTR = SIZEOF_OBJ
} SizeOfType;

/* Numero massimo degli argomenti di un'istruzione */
#define VM_MAX_NUMARGS 2

struct __vmprogram;
struct __vmstatus;

/* Enumerazione dei tipi di moduli */
typedef enum {
  MODULE_IS_VM_CODE,
  MODULE_IS_C_FUNC,
  MODULE_UNDEFINED
} VMModuleType;

/* Tipo che descrive l'indirizzo del (codice-VM / funzione in C)
 * associato al modulo.
 */
typedef union {
  struct {
    Intg dim;
    void *code;
  } vm;
  Task (*c_func)(struct __vmprogram *);
} VMModulePtr;

typedef struct {
  VMModuleType type;  /* Tipo di modulo */
  const char *name; /* Nome del modulo */
  VMModulePtr ptr;    /* Puntatore al modulo */
  Intg length;      /* Dimensione del modulo (in numero di VMByteX4) */
} VMModule;

/* This type is used in the table 'vm_instr_desc_table', which collects
 * and describes all the instruction of the VM.
 */
typedef struct {
  const char *name;             /* Nome dell'istruzione */
  UInt numargs;                 /* Numero di argomenti dell'istruzione */
  TypeID t_id;                  /* Numero che identifica il tipo
                                   degli argomenti (interi, reali, ...) */
  void (*get_args)(struct __vmstatus *); /* Per trattare gli argomenti */
  void (*execute)(struct __vmprogram *); /* Per eseguire l'istruzione */
  /* Per disassemblare gli argomenti */
  void (*disasm)(struct __vmprogram *, char **);
} VMInstrDesc;

/* This structure contains all the data which define the status for the VM.
 * Status is allocated by 'VM_Module_Execute' inside its stack.
 */
struct __vmstatus {
  /* Flags della VM */
  struct {
    unsigned int error    : 1; /* L'istruzione ha provocato un errore! */
    unsigned int exit     : 1; /* Bisogna uscire dall'esecuzione! */
    unsigned int is_long  : 1; /* L'istruzione e' in formato lungo? */
  } flags;

  Intg line;          /* Numero di linea (fissato dall'istruzione line) */
  VMModule *m;        /* Modulo correntemente in esecuzione */

  /* Variabili che riguardano l'istruzione in esecuzione: */
  VMByteX4 *i_pos;    /* Puntatore all'istruzione */
  VMByteX4 i_eye;     /* Occhio di lettura (gli ultimi 4 byte letti) */
  UInt i_type, i_len; /* Tipo e dimensione dell'istruzione */
  UInt arg_type;      /* Tipo degli argomenti dell'istruzione */
  VMInstrDesc *idesc; /* Descrittore dell'istruzione corrente */
  void *arg1, *arg2;  /* Puntatori agli argomenti del'istruzione */

  /* Array di puntatori alle zone registri globali e locali */
  void *global[NUM_TYPES];
  void *local[NUM_TYPES];
  /* Numero di registro globale minimo e massimo */
  Intg gmin[NUM_TYPES], gmax[NUM_TYPES];
  /* Numero di registro locale minimo e massimo */
  Intg lmin[NUM_TYPES], lmax[NUM_TYPES];
  /* Stato di allocazione dei registri per il modulo in esecuzione */
  Intg alc[NUM_TYPES];
};

typedef struct __vmstatus VMStatus;

/* Tipo che serve a gestire la scrittura di codice per la VM */
typedef struct {
  struct {
    unsigned int error : 1;
    unsigned int inhibit : 1;
  } status;
  Array *program;
} AsmOut;

/* This structure define all what is needed for the functions defined inside
 * the file 'virtmach.c'
 */
struct __vmprogram {
  Array *vm_modules_list; /* List of installed modules */
  int vm_globals;
  void *vm_global[NUM_TYPES];
  Intg vm_gmin[NUM_TYPES], vm_gmax[NUM_TYPES];
  Obj *box_vm_current, *box_vm_arg1, *box_vm_arg2;
  struct {unsigned int hexcode : 1;} vm_dflags;
  /* Array in cui verranno disassemblati gli argomenti (scritti con sprintf) */
  char iarg_str[VM_MAX_NUMARGS][64];
  struct {unsigned int forcelong : 1;} vm_aflags;
  AsmOut *vm_cur_output;
  AsmOut *tmp_code;
  VMStatus *vmcur;
};

typedef struct __vmprogram VMProgram;

extern VMInstrDesc vm_instr_desc_table[];

extern const UInt size_of_type[NUM_TYPES];

/* These functions are intended to be used only inside 'vmexec.h' */
void VM__GLP_GLPI(VMStatus *vmcur);
void VM__GLP_Imm(VMStatus *vmcur);
void VM__GLPI(VMStatus *vmcur);
void VM__Imm(VMStatus *vmcur);
void VM__D_GLPI_GLPI(VMProgram *vmp, char **iarg);
void VM__D_CALL(VMProgram *vmp, char **iarg);
void VM__D_GLPI_Imm(VMProgram *vmp, char **iarg);

/* These are the functions to use to control the VM */
Task VM_Init(VMProgram **new_vmp);
void VM_Destroy(VMProgram *vmp);

Task VM_Module_Install(VMProgram *vmp, Intg *new_module,
 VMModuleType t, const char *name, VMModulePtr p);
Task VM_Module_Define(VMProgram *vmp, Intg module_num,
 VMModuleType t, VMModulePtr p);
Task VM_Module_Undefined(VMProgram *vmp, Intg *new_module, const char *name);
Intg VM_Module_Next(VMProgram *vmp);
Task VM_Module_Check(VMProgram *vmp, int report_errs);
Task VM_Module_Globals(VMProgram *vmp, Intg num_var[], Intg num_reg[]);
Task VM_Module_Global_Set(VMProgram *vmp, Intg type, Intg reg, void *value);

Task VM_Module_Execute(VMProgram *vmp, Intg mnum);

void VM_DSettings(VMProgram *vmp, int hexcode);
Task VM_Module_Disassemble(VMProgram *vmp, Intg module_num, FILE *stream);
Task VM_Module_Disassemble_All(VMProgram *vmp, FILE *stream);
Task VM_Disassemble(VMProgram *vmp, FILE *output, void *prog, UInt dim);

void VM_ASettings(VMProgram *vmp, int forcelong, int error, int inhibit);
AsmOut *VM_Asm_Out_New(Intg dim);
void VM_Asm_Out_Set(VMProgram *vmp, AsmOut *out);
Task VM_Asm_Prepare(VMProgram *vmp, Intg *num_var, Intg *num_reg);
Task VM_Asm_Install(VMProgram *vmp, Intg module, AsmOut *program);
void VM_Assemble(VMProgram *vmp, AsmCode instr, ...);

/* Numero minimo di VMByteX4 che riesce a contenere tutti i tipi possibili
 * di argomenti (Intg, Real, Point, Obj)
 */
#define MAX_SIZE_IN_IWORDS \
 ((sizeof(Point) + sizeof(VMByteX4) - 1) / sizeof(VMByteX4))

#define BOX_VM_CURRENT(vmp, Type) *((Type *) *(vmp)->box_vm_current)
#define BOX_VM_ARG1(vmp, Type) *((Type *) *(vmp)->box_vm_arg1)
#define BOX_VM_ARG2(vmp, Type) *((Type *) *(vmp)->box_vm_arg2)
#define BOX_VM_ARGPTR1(vmp, Type) ((Type *) *(vmp)->box_vm_arg1)
#define BOX_VM_ARGPTR2(vmp, Type) ((Type *) *(vmp)->box_vm_arg2)

#endif
