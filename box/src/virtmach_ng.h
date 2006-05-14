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

/* Data type used to write/read binary codes for the instructions */
typedef unsigned char VMByte;
typedef unsigned long VMByteX4;
typedef signed char VMSByte;

/* Definisco il tipo Obj, che e' semplicemente il tipo puntatore */
typedef void *Obj;

/* Numero massimo degli argomenti di un'istruzione */
#define VM_MAX_NUMARGS 2

struct __vmprogram {};

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
  void (*get_args)(VMStatus *); /* Per trattare gli argomenti */
  void (*execute)(VMProgram *); /* Per eseguire l'istruzione */
  void (*disasm)(VMProgram *, char **);  /* Per disassemblare gli argomenti */
} VMInstrDesc;

/* This structure contains all the data which define the status for the VM.
 * Status is allocated by 'VM_Module_Execute' inside its stack.
 */
typedef struct {
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
} VMStatus;

/* This structure define all what is needed for the functions defined inside
 * the file 'virtmach.c'
 */
struct __vmprogram {
  Array *vm_modules_list = NULL; /* List of installed modules */
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
} VMProgram;

typedef struct __vmprogram VMProgram;

Task VMProg_Init(VMProgram *new_vmp);
void VMProg_Destroy(VMProgram *vmp);

Task VM_Module_Install(VMProgram *vmp, Intg *new_module
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

/* Numero minimo di ByteX4 che riesce a contenere tutti i tipi possibili
 * di argomenti (Intg, Real, Point, Obj)
 */
#define MAX_SIZE_IN_IWORDS \
 ((sizeof(Point) + sizeof(ByteX4) - 1) / sizeof(ByteX4))

#define BOX_VM_CURRENT(vmp, Type) *((Type *) *(vmp)->box_vm_current)
#define BOX_VM_ARG1(vmp, Type) *((Type *) *(vmp)->box_vm_arg1)
#define BOX_VM_ARG2(vmp, Type) *((Type *) *(vmp)->box_vm_arg2)
#define BOX_VM_ARGPTR1(vmp, Type) ((Type *) *(vmp)->box_vm_arg1)
#define BOX_VM_ARGPTR2(vmp, Type) ((Type *) *(vmp)->box_vm_arg2)

#endif
