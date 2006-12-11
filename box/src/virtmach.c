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

/* $Id$ */

/* This files implements the functionality of the Virtual Machine (VM)
 * of Box: defines functions to assemble, disassemble, execute basic
 * instructions.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "str.h"
#include "messages.h"
#include "array.h"
#include "collection.h"
#include "virtmach.h"
#include "vmsym.h"
#include "vmproc.h"

/* Read the first 4 bytes (VMByteX4), extract the format bit and put the "rest"
 * in the i_eye (which should be defined as 'register VMByteX4 i_eye;')
 * is_long tells if the instruction is encoded with the packed format (4 bytes)
 * or with the long format (> 4 bytes). To read / write an instruction
 * represented with the packed format one should use the macros
 * ASM_SHORT_GET_* / ASM_SHORT_PUT_*. To read / write an instruction written
 * with the long format one should use instead the macros ASM_LONG_GET_* /
 * ASM_LONG_PUT_*
 */
#define ASM_GET_FORMAT(i_pos, i_eye, is_long) \
{ i_eye = *(i_pos++); is_long = (i_eye & 0x1); i_eye >>= 1; }

/* SHORT INSTRUCTION: we assemble the istruction header in the following way:
 * (note: 1 is represented with bit 0 = 1 and all other bits = 0)
 *  bit 0: true if the instruction is long
 *  bit 1-4: type of arguments
 *  bit 5-7: length of instruction
 *  bit 8-15: type of instruction
 *  (bit 16-23: left empty for argument 1)
 *  (bit 24-31: left empty for argument 2)
 */
#define ASM_SHORT_PUT_HEADER(i_pos, i_eye, i_type, is_long, i_len, arg_type) \
{ i_eye = (((i_type) & 0xff)<<3 | ((i_len) & 0x7))<<4 | ((arg_type) & 0xf); \
  i_eye = i_eye<<1 | ((is_long) & 0x1); }

#define ASM_SHORT_PUT_1ARG(i_pos, i_eye, arg) \
{ *(i_pos++) = ((arg) & 0xff) << 16 | i_eye; }

#define ASM_SHORT_PUT_2ARGS(i_pos, i_eye, arg1, arg2) \
{ *(i_pos++) = i_eye = (((arg2) & 0xff)<<8 | ((arg1) & 0xff))<<16 | i_eye; }

#define ASM_SHORT_GET_HEADER(i_pos, i_eye, i_type, i_len, arg_type) \
{ arg_type = i_eye & 0xf; i_len = (i_eye >>= 4) & 0x7; \
  i_type = (i_eye >>= 3) & 0xff; }

#define ASM_SHORT_GET_1ARG(i_pos, i_eye, arg) \
{ arg = (signed char) ((i_eye >>= 8) & 0xff); }

#define ASM_SHORT_GET_2ARGS(i_pos, i_eye, arg1, arg2) \
{ arg1 = (signed char) ((i_eye >>= 8) & 0xff); \
  arg2 = (signed char) ((i_eye >>= 8) & 0xff); }

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
#define ASM_LONG_PUT_HEADER(i_pos, i_eye, i_type, is_long, i_len, arg_type) \
{ i_eye = ((i_len) & 0x07ff)<<4 | ((arg_type) & 0xf); \
  i_eye = i_eye<<1 | ((is_long) & 0x1); \
  *(i_pos++) = i_eye; *(i_pos++) = i_type; }

#define ASM_LONG_GET_HEADER(i_pos, i_eye, i_type, i_len, arg_type) \
{ arg_type = i_eye & 0xf; i_len = (i_eye >>= 4); i_type = *(i_pos++); }

#define ASM_LONG_GET_1ARG(i_pos, i_eye, arg) \
{ arg = i_eye = *(i_pos++); }

#define ASM_LONG_GET_2ARGS(i_pos, i_eye, arg1, arg2) \
{ arg1 = *(i_pos++); arg2 = i_eye = *(i_pos++); }

/*****************************************************************************
 *  Le seguenti funzioni servono a ricavare gli indirizzi in memoria dei     *
 *  registri globali (indicati con 'G'), dei registri locali ('L'),          *
 *  dei puntatori ('P') o degli immediati interi ('I') che compaiono come    *
 *  argomenti delle istruzioni. Ad esempio: l'istruzione "mov ri2, i[ro0+4]" *
 *  dice alla macchina virtuale di mettere nel registro intero locale num. 2 *
 *  (ri2) il valore intero presente alla locazione (ro0 + 4). A tale scopo   *
 *  la VM calcola gli indirizzi in memoria dei 2 argomenti (usando VM__Get_L *
 *  per il primo argomento, ri2, e usando VM_Get_P per il secondo, i[ro0+4]).*
 *  Una volta calcolati gli indirizzi la VM chiamera' VM__Exec_Mov_II per    *
 *  eseguire materialmente l'operazione.                                     *
 *****************************************************************************/

/* This array lets us to obtain the size of a type by type index.
 * (Useful in what follows)
 */
const UInt size_of_type[NUM_TYPES] = {
  sizeof(Char),
  sizeof(Intg),
  sizeof(Real),
  sizeof(Point),
  sizeof(Obj)
};

/* Restituisce il puntatore al registro globale n-esimo. */
static void *VM__Get_G(VMStatus *vmcur, Intg n) {
  register TypeID t = vmcur->idesc->t_id;

  assert( (t >= 0) && (t < NUM_TYPES) );

#ifdef VM_SAFE_EXEC1
  if ( (n < vmcur->gmin[t]) || (n > vmcur->gmax[t]) ) {
  MSG_ERROR("Riferimento a registro globale non allocato!");
  vmcur->flags.error = vmcur->flags.exit = 1;
  return NULL;
  }
#endif
  return (
   (void *) (vmcur->global[t]) + (Intg) (n * size_of_type[t])
         );
}

/* Restituisce il puntatore al registro locale n-esimo. */
static void *VM__Get_L(VMStatus *vmcur, Intg n) {
  register TypeID t = vmcur->idesc->t_id;

  assert( (t >= 0) && (t < NUM_TYPES) );

#ifdef VM_SAFE_EXEC1
  if ( (n < vmcur->lmin[t]) || (n > vmcur->lmax[t]) ) {
  MSG_ERROR("Riferimento a registro locale non allocato!");
  vmcur->flags.error = vmcur->flags.exit = 1;
  return NULL;
  }
#endif
  return (
   (void *) (vmcur->local[t]) + (Intg) (n * size_of_type[t])
         );
}

/* Restituisce il puntatore alla locazione di memoria (ro0 + n). */
static void *VM__Get_P(VMStatus *vmcur, Intg n) {
  return *((void **) vmcur->local[TYPE_OBJ]) + n;
}

/* Restituisce il puntatore ad un intero/reale di valore n. */
static void *VM__Get_I(VMStatus *vmcur, Intg n) {
  register TypeID t = vmcur->idesc->t_id;
  static Intg i = 0;
  static union { Char c; Intg i; Real r; } v[2], *value;

  assert( (t >= TYPE_CHAR) && (t <= TYPE_REAL) );

  value = & v[i]; i ^= 1;
  switch (t) {
    case TYPE_CHAR:
      value->c = (Char) n; return (void *) (& value->c);
    case TYPE_INTG:
      value->i = (Intg) n; return (void *) (& value->i);
    case TYPE_REAL:
      value->r = (Real) n; return (void *) (& value->r);
      default: /* This shouldn't happen! */
        return (void *) (& value->i); break;
  }
}

/* Array di puntatori alle 4 funzioni sopra: */
static void *(*vm_gets[4])(VMStatus *, Intg) = {
  VM__Get_G, VM__Get_L, VM__Get_P, VM__Get_I
};

/*******************************************************************************
 * Functions used to execute the instructions                                  *
 *******************************************************************************/

/* Questa funzione trova e imposta gli indirizzi corrispondenti
 *  ai 2 argomenti dell'istruzione. In modo da poter procedere all'esecuzione.
 * NOTA: Questa funzione gestisce solo le istruzioni di tipo GLP-GLPI,
 *  cioe' le istruzioni in cui: il primo argomento e' 'G'lobale, 'L'ocale,
 *  'P'untatore, mentre il secondo puo' essere dei tipi appena enumerati oppure
 *  puo' essere un 'I'mmediato intero.
 */
void VM__GLP_GLPI(VMStatus *vmcur) {
  signed long narg1, narg2;
  register UInt atype = vmcur->arg_type;

  if ( vmcur->flags.is_long ) {
    ASM_LONG_GET_2ARGS( vmcur->i_pos, vmcur->i_eye, narg1, narg2);
  } else {
    ASM_SHORT_GET_2ARGS( vmcur->i_pos, vmcur->i_eye, narg1, narg2);
  }

  vmcur->arg1 = vm_gets[atype & 0x3](vmcur, narg1);
  vmcur->arg2 = vm_gets[(atype >> 2) & 0x3](vmcur, narg2);
#ifdef DEBUG_EXEC
  printf("Cathegories: arg1 = %lu - arg2 = %lu\n",
         atype & 0x3, (atype >> 2) & 0x3);
#endif
}

/* Questa funzione e' analoga alla precedente, ma gestisce
 *  istruzioni come: "mov ri1, 123456", "mov rf2, 3.14", "mov rp5, (1, 2)", etc.
 */
void VM__GLP_Imm(VMStatus *vmcur) {
  signed long narg1;
  register UInt atype = vmcur->arg_type;

  if ( vmcur->flags.is_long ) {
    ASM_LONG_GET_1ARG( vmcur->i_pos, vmcur->i_eye, narg1 );
  } else {
    ASM_SHORT_GET_1ARG( vmcur->i_pos, vmcur->i_eye, narg1 );
  }

  vmcur->arg1 = vm_gets[atype & 0x3](vmcur, narg1);
  vmcur->arg2 = vmcur->i_pos;
}

/* Questa funzione e' analoga alle precedenti, ma gestisce
 *  istruzioni con un solo argomento di tipo GLPI (Globale oppure Locale
 *  o Puntatore o Immediato intero).
 */
void VM__GLPI(VMStatus *vmcur) {
  signed long narg1;
  register UInt atype = vmcur->arg_type;

  if ( vmcur->flags.is_long ) {
    ASM_LONG_GET_1ARG( vmcur->i_pos, vmcur->i_eye, narg1 );
  } else {
    ASM_SHORT_GET_1ARG( vmcur->i_pos, vmcur->i_eye, narg1 );
  }

  vmcur->arg1 = vm_gets[atype & 0x3](vmcur, narg1);
}

/* Questa funzione e' analoga alle precedenti, ma gestisce
 *  istruzioni con un solo argomento di tipo immediato (memorizzato subito
 *  di seguito all'istruzione).
 */
void VM__Imm(VMStatus *vmcur) {vmcur->arg1 = (void *) vmcur->i_pos;}

/*****************************************************************************
 * Functions used to disassemble the instructions (see VM_Disassemble)       *
 *****************************************************************************/

/* Questa funzione serve a disassemblare gli argomenti di
 *  un'istruzione di tipo GLPI-GLPI.
 *  iarg e' una tabella di puntatori alle stringhe che corrisponderanno
 *  agli argomenti disassemblati.
 */
void VM__D_GLPI_GLPI(VMProgram *vmp, char **iarg) {
  VMStatus *vmcur = vmp->vmcur;
  UInt n, na = vmcur->idesc->numargs;
  UInt iaform[2] = {vmcur->arg_type & 3, (vmcur->arg_type >> 2) & 3};
  Intg iaint[2];

  assert(na <= 2);

  /* Recupero i numeri (interi) di registro/puntatore/etc. */
  switch (na) {
    case 1: {
      void *arg2;

      if ( vmcur->flags.is_long ) {
        ASM_LONG_GET_1ARG( vmcur->i_pos, vmcur->i_eye, iaint[0]);
        arg2 = vmcur->i_pos;
      } else {
        ASM_SHORT_GET_1ARG( vmcur->i_pos, vmcur->i_eye, iaint[0]);
        arg2 = vmcur->i_pos;
      }
      break; }
    case 2:
      if ( vmcur->flags.is_long ) {
        ASM_LONG_GET_2ARGS( vmcur->i_pos, vmcur->i_eye, iaint[0], iaint[1]);
      } else {
        ASM_SHORT_GET_2ARGS( vmcur->i_pos, vmcur->i_eye, iaint[0], iaint[1]);
      }
      break;
  }

  for (n = 0; n < na; n++) {
    UInt iaf = iaform[n];
    UInt iat = vmcur->idesc->t_id;

    assert(iaf < 4);

    {
      Intg iai = iaint[n], uiai = iai;
      char rc, tc;
      const char typechars[NUM_TYPES] = "cirpo";

      tc = typechars[iat];
      if (uiai < 0) {uiai = -uiai; rc = 'v';} else rc = 'r';
      switch(iaf) {
        case CAT_GREG:
          sprintf(vmp->iarg_str[n], "g%c%c" SIntg, rc, tc, uiai);
          break;
        case CAT_LREG:
          sprintf(vmp->iarg_str[n], "%c%c" SIntg, rc, tc, uiai);
          break;
        case CAT_PTR:
          if ( iai < 0 )
            sprintf(vmp->iarg_str[n], "%c[ro0 - " SIntg "]", tc, uiai);
          else if ( iai == 0 )
            sprintf(vmp->iarg_str[n], "%c[ro0]", tc);
          else
            sprintf(vmp->iarg_str[n], "%c[ro0 + " SIntg "]", tc, uiai);
          break;
        case CAT_IMM:
          if ( iat == TYPE_CHAR ) iai = (Intg) ((Char) iai);
          sprintf(vmp->iarg_str[n], SIntg, iai);
          break;
      }
    }

    *(iarg++) = vmp->iarg_str[n];
  }
  return;
}

/* Analoga alla precedente, ma per istruzioni CALL. */
void VM__D_CALL(VMProgram *vmp, char **iarg) {
  VMStatus *vmcur = vmp->vmcur;
  register UInt na = vmcur->idesc->numargs;

  assert(na == 1);

  *iarg = vmp->iarg_str[0];
  if ( (vmcur->arg_type & 3) == CAT_IMM ) {
    UInt iat = vmcur->idesc->t_id;
    Intg call_num;
    void *arg2;

    if ( vmcur->flags.is_long ) {
      ASM_LONG_GET_1ARG( vmcur->i_pos, vmcur->i_eye, call_num);
      arg2 = vmcur->i_pos;
    } else {
      ASM_SHORT_GET_1ARG( vmcur->i_pos, vmcur->i_eye, call_num);
      arg2 = vmcur->i_pos;
    }

    if ( iat == TYPE_CHAR ) call_num = (Intg) ((Char) call_num);
    {
      VMProcTable *pt = & vmp->proc_table;
      if ( (call_num < 1) || (call_num > Arr_NumItem(pt->installed)) ) {
        sprintf(vmp->iarg_str[0], SIntg, call_num);
        return;

      } else {
        char *call_name;
        VMProcInstalled *p;
        p = Arr_ItemPtr(pt->installed, VMProcInstalled, call_num);
        call_name = Str_Cut(p->name, 40, 85);
        sprintf(vmp->iarg_str[0], SIntg"('%.40s')", call_num, call_name);
        free(call_name);
        return;
      }

    }

  } else {
    VM__D_GLPI_GLPI(vmp, iarg);
  }
}

/* Analoga alla precedente, ma per istruzioni di salto (jmp, jc). */
void VM__D_JMP(VMProgram *vmp, char **iarg) {
  VMStatus *vmcur = vmp->vmcur;
  register UInt na = vmcur->idesc->numargs;

  assert(na == 1);

  *iarg = vmp->iarg_str[0];
  if ( (vmcur->arg_type & 3) == CAT_IMM ) {
    UInt iat = vmcur->idesc->t_id;
    Intg m_num;
    Intg position;
    void *arg2;

    if ( vmcur->flags.is_long ) {
      ASM_LONG_GET_1ARG( vmcur->i_pos, vmcur->i_eye, m_num);
      arg2 = vmcur->i_pos;
    } else {
      ASM_SHORT_GET_1ARG( vmcur->i_pos, vmcur->i_eye, m_num);
      arg2 = vmcur->i_pos;
    }

    if ( iat == TYPE_CHAR ) m_num = (Intg) ((Char) m_num);

    position = (vmcur->dasm_pos + m_num)*sizeof(VMByteX4);
    sprintf(vmp->iarg_str[0], SIntg, position);
    return;

  } else {
    VM__D_GLPI_GLPI(vmp, iarg);
  }
}

/* Analoga alla precedente, ma per istruzioni del tipo GLPI-Imm. */
void VM__D_GLPI_Imm(VMProgram *vmp, char **iarg) {
  VMStatus *vmcur = vmp->vmcur;
  UInt iaf = vmcur->arg_type & 3, iat = vmcur->idesc->t_id;
  Intg iai;
  VMByteX4 *arg2;

  assert(vmcur->idesc->numargs == 2);
  assert(iat < 4);

  /* Recupero il numero (intero) di registro/puntatore/etc. */
  if ( vmcur->flags.is_long ) {
    ASM_LONG_GET_1ARG( vmcur->i_pos, vmcur->i_eye, iai);
    arg2 = vmcur->i_pos;
  } else {
    ASM_SHORT_GET_1ARG( vmcur->i_pos, vmcur->i_eye, iai);
    arg2 = vmcur->i_pos;
  }

  /* Primo argomento */
  {
    Intg uiai = iai;
    char rc, tc;
    const char typechars[NUM_TYPES] = "cirpo";

    tc = typechars[iat];
    if (uiai < 0) {uiai = -uiai; rc = 'v';} else rc = 'r';
    switch(iaf) {
      case CAT_GREG:
        sprintf(vmp->iarg_str[0], "g%c%c" SIntg, rc, tc, uiai);
        break;
      case CAT_LREG:
        sprintf(vmp->iarg_str[0], "%c%c" SIntg, rc, tc, uiai);
        break;
      case CAT_PTR:
        if ( iai < 0 )
          sprintf(vmp->iarg_str[0], "%c[ro0 - " SIntg "]", tc, uiai);
        else if ( iai == 0 )
          sprintf(vmp->iarg_str[0], "%c[ro0]", tc);
        else
          sprintf(vmp->iarg_str[0], "%c[ro0 + " SIntg "]", tc, uiai);
        break;
      case CAT_IMM:
        sprintf(vmp->iarg_str[0], SIntg, iai);
        break;
    }
  }

  /* Secondo argomento */
  switch (iat) {
    case TYPE_CHAR:
      sprintf( vmp->iarg_str[1], SChar, *((Char *) arg2) );
      break;
    case TYPE_INTG:
      sprintf( vmp->iarg_str[1], SIntg, *((Intg *) arg2) );
      break;
    case TYPE_REAL:
      sprintf( vmp->iarg_str[1], SReal, *((Real *) arg2) );
      break;
    case TYPE_POINT:
      sprintf( vmp->iarg_str[1], SPoint,
               ((Point *) arg2)->x, ((Point *) arg2)->y );
      break;
  }

  *(iarg++) = vmp->iarg_str[0];
  *iarg = vmp->iarg_str[1];
}

/*****************************************************************************
 * Functions for (de)inizialization                                          *
 *****************************************************************************/
Task VM_Init(VMProgram **new_vmp) {
  VMProgram *nv;
  nv = (VMProgram *) malloc(sizeof(VMProgram));
  if (nv == NULL) return Failed;
#if 0
  nv->vm_modules_list = (Array *) NULL;
  nv->sheets = (Collection *) NULL;
  nv->current_sheet_id = -1;
  nv->current_sheet = (VMSheet *) NULL;
#endif
  nv->labels = (Collection *) NULL;
  nv->references = (Collection *) NULL;
  nv->vm_globals = 0;
  nv->vm_dflags.hexcode = 0;
  nv->vm_aflags.forcelong = 0;
  nv->stack = (Array *) NULL;

  TASK( VM_Proc_Init(nv) );
  TASK( VM_Sym_Init(nv) );
  *new_vmp = nv;
  return Success;
}

void VM_Destroy(VMProgram *vmp) {
  if (vmp == (VMProgram *) NULL) return;
  if (vmp->vm_globals != 0) {
    int i;
    for(i = 0; i < NUM_TYPES; i++) free( vmp->vm_global[i] );
  }
  if ( vmp->stack != NULL )
    if ( Arr_NumItem(vmp->stack) != 0 ) {
      MSG_WARNING("Run finished with non empty stack.");
    }
  Clc_Destroy(vmp->references);
  Clc_Destroy(vmp->labels);
  Arr_Destroy(vmp->stack);
  VM_Sym_Destroy(vmp);
  VM_Proc_Destroy(vmp);
  free(vmp);
}

#if 0
/* Installa un nuovo modulo di programma con nome name.
 * Un modulo e' semplicemente un pezzo di codice che puo' essere eseguito.
 * E' possibile installare 2 tipi di moduli:
 *  1) (caso t = MODULE_IS_VM_CODE) il modulo e' costituito da
      codice eseguibile dalla VM
 *    (p.vm_code e' il puntatore alla prima istruzione nel codice);
 *  2) (caso t = MODULE_IS_C_FUNC) il modulo e' semplicemente una funzione
 *    scritta in C (p.c_func e' il puntatore alla funzione)
 * Restituisce il numero assegnato al modulo (> 0), oppure 0 se qualcosa
 * e' andato storto. Tale numero, se usato da un istruzione call, provoca
 * l'esecuzione del modulo come procedura.
 */
Task VM_Module_Install(VMProgram *vmp, Intg *new_module,
 VMModuleType t, const char *name, VMModulePtr p) {

  /* Creo la lista dei moduli se non esiste */
  if ( vmp->vm_modules_list == NULL ) {
    vmp->vm_modules_list = Array_New(sizeof(VMModule), VM_TYPICAL_NUM_MODULES);
    if ( vmp->vm_modules_list == NULL ) return Failed;
  }

  {
    VMModule new;
    new.type = t;
    new.name = strdup(name);
    new.ptr = p;
    TASK( Arr_Push( vmp->vm_modules_list, & new ) );
  }

  *new_module = Arr_NumItem(vmp->vm_modules_list);
  {
    VMSym s;
    Name n = {strlen(name), name};
    VM_Sym_Procedure(& s, & n, *new_module);
    TASK( VM_Sym_Add(vmp, & s) );
  }
  return Success;
}

/* This function defines an undefined module (previously created
 * with VM_Module_Undefined).
 */
Task VM_Module_Define(VMProgram *vmp, Intg module_num,
 VMModuleType t, VMModulePtr p) {
  VMModule *m;
  if ( vmp->vm_modules_list == NULL ) {
    MSG_ERROR("La lista dei moduli e' vuota!");
    return Failed;
  }
  if ((module_num < 1) || (module_num > Arr_NumItem(vmp->vm_modules_list))) {
    MSG_ERROR("Impossibile definire un modulo non esistente.");
    return Failed;
  }
  m = ((VMModule *) vmp->vm_modules_list->ptr) + (module_num - 1);
  if ( m->type != MODULE_UNDEFINED ) {
    MSG_ERROR("Questo modulo non puo' essere definito!");
    return Failed;
  }
  m->type = t;
  m->ptr = p;
  return Success;
}

/* This function creates a new undefined module with name name. */
Task VM_Module_Undefined(VMProgram *vmp, Intg *new_module, const char *name) {
  return VM_Module_Install(vmp, new_module,
   MODULE_UNDEFINED, name, (VMModulePtr) {{0, NULL}});
}

/* This function returns the module-number which will be
 * associated with the next module that will be installed.
 */
Intg VM_Module_Next(VMProgram *vmp) {
  if ( vmp->vm_modules_list == NULL ) return 1;
  return Arr_NumItem(vmp->vm_modules_list) + 1;
}

/* This function checks the status of definitions of all the
 * created modules. If one of the modules is undefined (and report_errs == 1)
 * it prints an error message for every undefined module.
 */
Task VM_Module_Check(VMProgram *vmp, int report_errs) {
  VMModule *m;
  Intg mn;
  int status = Success;

  if ( vmp->vm_modules_list == NULL ) return Success;
  m = Arr_FirstItemPtr(vmp->vm_modules_list, VMModule);
  for (mn = Arr_NumItem(vmp->vm_modules_list); mn > 0; mn--) {
    if ( m->type == MODULE_UNDEFINED ) {
      status = Failed;
      if ( report_errs ) {
        MSG_ERROR("'%s' <-- Modulo non definito!", m->name);
      }
    }
    ++m;
  }
  return status;
}
#endif

/* Sets the number of global registers and variables for each type. */
Task VM_Module_Globals(VMProgram *vmp, Intg num_var[], Intg num_reg[]) {
  int i;

  assert(vmp != (VMProgram *) NULL);

  if ( vmp->vm_globals != 0 ) {
    int i;
    for(i = 0; i < NUM_TYPES; i++) {
      free( vmp->vm_global[i] );
      vmp->vm_global[i] = NULL;
      vmp->vm_gmin[i] = 1;
      vmp->vm_gmax[i] = -1;
    }
  }

  for(i = 0; i < NUM_TYPES; i++) {
    Intg nv = num_var[i], nr = num_reg[i];
    if (nv < 0 || nr < 0) {
      MSG_ERROR("Errore nella definizione dei numeri di registri globali.");
      return Failed;
    }
  }

  for(i = 0; i < NUM_TYPES; i++) {
    Intg nv = num_var[i], nr = num_reg[i];
    if ( (vmp->vm_global[i] = calloc(nv+nr+1, size_of_type[i])) == NULL ) {
      MSG_ERROR("Errore nella allocazione dei registri globali.");
      vmp->vm_globals = -1;
      return Failed;
    }
    vmp->vm_gmin[i] = -nv;
    vmp->vm_gmax[i] = nr;
  }

  vmp->vm_globals = 1;
  {
    register Obj *reg_obj;
    reg_obj = ((Obj *) vmp->vm_global[TYPE_OBJ]) + vmp->vm_gmin[TYPE_OBJ];
    vmp->box_vm_current = reg_obj + 1;
    vmp->box_vm_arg1    = reg_obj + 2;
    vmp->box_vm_arg2    = reg_obj + 3;
  }
  return Success;
}

/* Sets the value for the global register (or variable)
 * of type type, whose number is reg. *value is the value assigned
 * to the register (variable).
 */
Task VM_Module_Global_Set(VMProgram *vmp, Intg type, Intg reg, void *value) {
  if ( (type < 0) || (type >= NUM_TYPES ) ) {
    MSG_ERROR("VM_Module_Global_Set: Non esistono registri di questo tipo!");
    return Failed;
  }

  if ( (reg < vmp->vm_gmin[type]) || (reg > vmp->vm_gmax[type]) ) {
    MSG_ERROR("VM_Module_Global_Set: Riferimento a registro non allocato!");
    return Failed;
  }

  {
    int s = size_of_type[type];
    void *dest;
    dest = vmp->vm_global[type] + (reg - vmp->vm_gmin[type]) * s;
    memcpy(dest, value, s);
  }
  return Success;
}

/*****************************************************************************
 * Functions to execute code                                                 *
 *****************************************************************************/

/* Execute the module number m of program vmp.
 * If initial != NULL, *initial is the initial status of the virtual machine.
 */
Task VM_Module_Execute(VMProgram *vmp, unsigned int call_num) {
  VMProcTable *pt = & vmp->proc_table;
  VMProcInstalled *p;
  register VMByteX4 *i_pos;
  VMStatus vm;
  static Generic reg0[NUM_TYPES]; /* Registri locali numero zero */
#ifdef DEBUG_EXEC
  Intg i = 0;
#endif

  /* Controlliamo che il modulo sia installato! */
  if ( (call_num < 1) || (call_num > Arr_NumItem(pt->installed)) ) {
    MSG_ERROR("Call to the undefined procedure %d.", call_num);
    return Failed;
  }

  p = Arr_ItemPtr( pt->installed, VMProcInstalled, call_num );
  switch (p->type) {
    case VMPROC_IS_C_CODE: return p->code.c(vmp);
    case VMPROC_IS_VM_CODE: break;
    default:
      MSG_ERROR("Call into the broken procedure %d.", call_num);
      return Failed;
  }

  vmp->vmcur = & vm;

  {
    int i;
    for(i = 0; i < NUM_TYPES; i++) {
      vm.lmin[i] = 0; vm.lmax[i] = 0; vm.local[i] = & reg0[i];
      if ( vmp->vm_global[i] != NULL ) {
        vm.gmin[i] = vmp->vm_gmin[i]; vm.gmax[i] = vmp->vm_gmax[i];
        vm.global[i] = vmp->vm_global[i] + size_of_type[i]*(-vmp->vm_gmin[i]);
      } else {
        vm.gmin[i] = 1; vm.gmax[i] = -1; vm.global[i] = NULL;
      }
    }
  }

  vm.p = p;
  vm.i_pos = i_pos = (VMByteX4 *) p->code.vm.ptr;
  vm.flags.exit = vm.flags.error = 0;
  {register int i; for(i = 0; i < NUM_TYPES; i++) vm.alc[i] = 0;}

  do {
    register int is_long;
    register VMByteX4 i_eye;

#ifdef DEBUG_EXEC
    fprintf(stderr, "module = "SIntg", pos = "SIntg" - reading instruction.\n",
           mnum, i*sizeof(VMByteX4));
#endif

    /* Leggo i dati fondamentali dell'istruzione: tipo e lunghezza. */
    ASM_GET_FORMAT(vm.i_pos, i_eye, is_long);
    vm.flags.is_long = is_long;
    if ( is_long ) {
      ASM_LONG_GET_HEADER(vm.i_pos, i_eye, vm.i_type, vm.i_len, vm.arg_type);
      vm.i_eye = i_eye;
      vm.flags.is_long = 1;

    } else {
      ASM_SHORT_GET_HEADER(vm.i_pos, i_eye, vm.i_type, vm.i_len, vm.arg_type);
      vm.i_eye = i_eye;
      vm.flags.is_long = 0;
    }

    if ( vm.i_type >= ASM_ILLEGAL ) {
      MSG_ERROR("Istruzione non riconosciuta!");
      return Failed;
    }

    /* Trovo il descrittore di istruzione */
    vm.idesc = & vm_instr_desc_table[vm.i_type];

    /* Localizza in memoria gli argomenti */
    if ( vm.idesc->numargs > 0 ) vm.idesc->get_args(& vm);
    /* Esegue l'istruzione */
    if ( ! vm.flags.error ) vm.idesc->execute(vmp);

    /* Passo alla prossima istruzione.
     * vm.i_len can be modified by 'vm.idesc->execute(vmp)' when executing
     * instructions such as 'jmp' or 'jc'
     */
    vm.i_pos = (i_pos += vm.i_len);
#ifdef DEBUG_EXEC
    i += vm.i_len;
#endif

  } while ( ! vm.flags.exit );

  /* Delete the registers allocated with the 'new*' instructions */
  {
    register int i;
    for(i = 0; i < NUM_TYPES; i++)
      if ( (vm.alc[i] & 1) != 0 )
        free(vm.local[i] + vm.lmin[i]*size_of_type[i]);
  }

  return vm.flags.error ? Failed : Success;
}

/*****************************************************************************
 * Functions to disassemble code                                             *
 *****************************************************************************/

/* Imposta le opzioni per il disassemblaggio:
 * L'opzione puo' essere settata con un valore > 0, resettata con 0
 * e lasciata inalterata con un valore < 0.
 * 1) hexcode: scrive nel listato anche i codici esadecimali delle
 *  istruzioni.
 */
void VM_DSettings(VMProgram *vmp, int hexcode) {
  /* Per settare le opzioni di assemblaggio, disassemblaggio, etc. */
  vmp->vm_dflags.hexcode = hexcode;
}

/* Traduce il codice binario della VM, in formato testo.
 * prog e' il puntatore all'inizio del codice, dim e' la dimensione del codice
 * da tradurre (espresso in "numero di VMByteX4").
 */
Task VM_Disassemble(VMProgram *vmp, FILE *output, void *prog, UInt dim) {
  register VMByteX4 *i_pos = (VMByteX4 *) prog;
  VMStatus vm;
  UInt pos, nargs;
  const char *iname;
  char *iarg[VM_MAX_NUMARGS];

  MSG_LOCATION("VM_Disassemble");

  vmp->vmcur = & vm;
  vm.flags.exit = vm.flags.error = 0;
  for ( pos = 0; pos < dim; ) {
    register VMByteX4 i_eye;
    register int is_long;

    vm.i_pos = i_pos;
    vm.dasm_pos = pos;

    /* Leggo i dati fondamentali dell'istruzione: tipo e lunghezza. */
    ASM_GET_FORMAT(vm.i_pos, i_eye, is_long);
    vm.flags.is_long = is_long;
    if ( is_long ) {
      ASM_LONG_GET_HEADER(vm.i_pos, i_eye, vm.i_type, vm.i_len, vm.arg_type);
      vm.i_eye = i_eye;
      vm.flags.is_long = 1;

    } else {
      ASM_SHORT_GET_HEADER(vm.i_pos, i_eye, vm.i_type, vm.i_len, vm.arg_type);
      vm.i_eye = i_eye;
      vm.flags.is_long = 0;
    }
#ifdef DEBUG_VM_D_EVERY_ONE
    printf("Instruction at position "SUInt": "
     "{is_long = %d, length = "SUInt", type = "SUInt
     ", arg_type = "SUInt")\n",
     pos, vm.flags.is_long, vm.i_len, vm.i_type, vm.arg_type);
#endif

    if ( (vm.i_type < 1) || (vm.i_type >= ASM_ILLEGAL) ) {
      iname = "???";
      vm.i_len = 1;
      nargs = 0;

    } else {
      /* Trovo il descrittore di istruzione */
      vm.idesc = & vm_instr_desc_table[vm.i_type];
      iname = vm.idesc->name;

      /* Localizza in memoria gli argomenti */
      nargs = vm.idesc->numargs;

      vm.idesc->disasm(vmp, iarg);
      if ( vm.flags.exit ) return Failed;
    }

    if ( vm.flags.error ) {
      fprintf(output, SUInt "\t%8.8lx\tError!",
              (UInt) (pos * sizeof(VMByteX4)), *i_pos);

    } else {
      int i;
      VMByteX4 *i_pos2 = i_pos;

      /* Stampo l'istruzione e i suoi argomenti */
      fprintf( output, SUInt "\t", (UInt) (pos * sizeof(VMByteX4)) );
      if ( vmp->vm_dflags.hexcode )
        fprintf(output, "%8.8lx\t", *(i_pos2++));
      fprintf(output, "%s", iname);

      if ( nargs > 0 ) {
        UInt n;

        assert(nargs <= VM_MAX_NUMARGS);

        fprintf(output, " %s", iarg[0]);
        for ( n = 1; n < nargs; n++ )
          fprintf(output, ", %s", iarg[n]);
      }
      fprintf(output, "\n");

      /* Stampo i restanti codici dell'istruzione in esadecimale */
      if ( vmp->vm_dflags.hexcode ) {
        for( i = 1; i < vm.i_len; i++ )
          fprintf(output, "\t%8.8lx\n", *(i_pos2++));
      }
    }

    /* Passo alla prossima istruzione */
    if ( vm.i_len < 1 ) return Failed;

    vm.i_pos = (i_pos += vm.i_len);
    pos += vm.i_len;
  }
  return Success;
}

#if 0
/******************************************************************************
 * Functions to handle sheets: a sheet is a place where you can put temporary *
 * code, which can then be istalled as a new module or handled in other ways. *
 ******************************************************************************/

static Task VM__Sheet_Destroy(void *s) {
  Array *program = ((VMSheet *) s)->program;
  if ( program != (Array *) NULL ) Arr_Destroy( program );
  return Success;
}

/* This function creates a new sheet. A sheet is a place where to put
 * the assembly code (bytecode) produced by VM_Assemble and is identified
 * by an integer assigned by this function. This integer is passed back
 * through *sheet_id.
 */
Task VM_Sheet_New(VMProgram *vmp, int *sheet_id) {
  VMSheet sheet;
  Array *program;

  if ( vmp->sheets == (Collection *) NULL ) {
     TASK( Clc_New(& vmp->sheets, sizeof(VMSheet), VM_TYPICAL_NUM_SHEETS) );
     Clc_Destructor(vmp->sheets, VM__Sheet_Destroy);
  }

  TASK( Arr_New(& program, sizeof(VMByteX4), VM_TYPICAL_MOD_DIM) );

  sheet.program = program;
  sheet.status.error = 0;
  sheet.status.inhibit = 0;
  return Clc_Occupy(vmp->sheets, & sheet, sheet_id);
}

/* Destroys the sheet 'sheet_id' created with 'VM_Sheet_New'.
 */
Task VM_Sheet_Destroy(VMProgram *vmp, int sheet_id) {
  return Clc_Release(vmp->sheets, sheet_id);
}

/* Get the VMSheet structure which describes sheet number 'sheet_id'.
 */
Task VM_Sheet(VMProgram *vmp, VMSheet **s, int sheet_id) {
  assert( vmp->sheets != (Collection *) NULL);
  return Clc_Object_Ptr(vmp->sheets, (void **) s, sheet_id);
}

/* Get the ID of the active sheet (the current target of VM_Assemble).
 */
int VM_Sheet_Get_Current(VMProgram *vmp) {
  return vmp->current_sheet_id;
}

/* Set ''sheet_id to be the active sheet.
 */
Task VM_Sheet_Set_Current(VMProgram *vmp, int sheet_id) {
  VMSheet *sheet;
  assert( vmp->sheets != (Collection *) NULL);
  TASK( Clc_Object_Ptr(vmp->sheets, (void **) & sheet, sheet_id) );
  vmp->current_sheet_id = sheet_id;
  vmp->current_sheet = sheet;
  return Success;
}

/* Remove all the code assembled inside the sheet 'sheet_id'.
 * WARNING: Labels and their references are not removed!
 */
Task VM_Sheet_Clear(VMProgram *vmp, int sheet_id) {
  VMSheet *sheet;
  assert( vmp->sheets != (Collection *) NULL);
  TASK( Clc_Object_Ptr(vmp->sheets, (void **) & sheet, sheet_id) );
  TASK( Arr_Clear(sheet->program) );
  return Success;
}

/* Install the sheet 'sheet_id' and assign to it the module number 'module'.
 * After this function has been executed, the VM will recognize the instruction
 * 'call module' as a call to the code containing in the sheet 'sheet_id'.
 */
Task VM_Sheet_Install(VMProgram *vmp, Intg module, int sheet_id) {
  VMSheet *sheet;
  VMModulePtr module_ptr;
  assert( vmp->sheets != (Collection *) NULL);
  TASK( Clc_Object_Ptr(vmp->sheets, (void **) & sheet, sheet_id) );
  module_ptr.vm.dim = Arr_NumItem(sheet->program);
  TASK( Arr_Data_Only(sheet->program, & module_ptr.vm.code) );
  return VM_Module_Define(vmp, module, MODULE_IS_VM_CODE, module_ptr);
}

/* Print as plain text the code contained inside the sheet 'sheet_id'
 * inside out.
 */
Task VM_Sheet_Disassemble(VMProgram *vmp, int sheet_id, FILE *out) {
  VMSheet *sheet;
  assert( vmp->sheets != (Collection *) NULL);
  TASK( Clc_Object_Ptr(vmp->sheets, (void **) & sheet, sheet_id) );
  {
    Array *prg = sheet->program;
    int prg_len = Arr_NumItem(prg);
    void *prg_ptr = Arr_Ptr(prg);
    return VM_Disassemble(vmp, out, prg_ptr, prg_len);
  }
}
#endif

/* This function creates a new label. A label is a number which refers to a
 * position in the assembled code. It can be a defined label (meaning that
 * the sheet and the position of the label in the sheet is known)
 * or can be an undefined label (meaning that we don't know where the label
 * will be pointing, but we want to jump there in some way).
 * This last behaviour is obtained passing position = -1.
 * In this case the function will create a list containing the unresolved
 * references to the label. Later, when the position of the label will be known
 * and is specified with VM_Label_Define, all the unresolved references
 * will be resolved.
 * NOTE: valid positions start from 0, not 1!
 */
Task VM_Label_New(VMProgram *vmp, int *label, int sheet_id, int position) {
  VMLabel l;
  if ( vmp->labels == (Collection *) NULL ) {
    TASK( Clc_New(& vmp->labels, sizeof(VMLabel), VM_TYPICAL_NUM_LABELS) );
  }

  l.sheet_id = sheet_id;
  l.position = position;
  l.chain_unresolved = -1; /* No unresolved references */
  TASK( Clc_Occupy(vmp->labels, (void *) & l, label) );
  return Success;
}

/* Same as VM_Label_New, but sheet_id is the current active sheet and
 * position is the current position in that sheet.
 */
Task VM_Label_New_Here(VMProgram *vmp, int *label) {
  return VM_Label_New( vmp, label, vmp->proc_table.target_proc_num,
   Arr_NumItem(vmp->proc_table.target_proc->code) );
}

/* This function assemble the jump instruction corresponding to the
 * reference 'r' to the label 'l'. It is called by 'VM_Label_Define'.
 */
static Task Resolve_Reference(VMProgram *vmp, VMReference *r, VMLabel *l) {
  VMProcTable *pt = & vmp->proc_table;
  unsigned int saved_proc_num = VM_Proc_Target_Get(vmp);
  VMProc *tmp_proc;

  TASK( VM_Proc_Empty(vmp, pt->tmp_proc) );
  TASK( VM_Proc_Target_Set(vmp, pt->tmp_proc) );
  tmp_proc = pt->target_proc;
  VM_Assemble(vmp, r->kind, CAT_IMM, l->position - r->position);
  TASK( VM_Proc_Target_Set(vmp, saved_proc_num) );

  {
    void *src = Arr_FirstItemPtr(tmp_proc->code, void);
    int src_size = Arr_NumItem(tmp_proc->code);
    Array *dest =  pt->target_proc->code; /* Destination sheet */
    int dest_pos = r->position + 1; /* NEED TO ADD 1 */
    TASK(Arr_Overwrite(dest, dest_pos, src, src_size));
  }
  return Success;
}

/* Specify the position of a undefined label.
 */
Task VM_Label_Define(VMProgram *vmp, int label, int sheet_id, int position) {
  VMLabel *l = NULL;
  TASK( Clc_Object_Ptr(vmp->labels,  & *(void **) l, label) );
  assert(l->position == -1); /* Should be undefined! */
  assert(l->sheet_id == sheet_id);
  l->position = position;

  /* Now we need to resolve past references to this label */
  while (l->chain_unresolved != -1) {
    VMReference *r = NULL;
    int cur_ref;
    cur_ref = l->chain_unresolved;
    TASK( Clc_Object_Ptr(vmp->references, & *(void **) r, cur_ref) );

    TASK( Resolve_Reference(vmp, r, l) );

    l->chain_unresolved = r->next;
    TASK( Clc_Release(vmp->references, cur_ref) );
  };
  return Success;
}

/* Same as VM_Label_Define, but sheet_id is the current active sheet and
 * position is the current position in that sheet.
 */
Task VM_Label_Define_Here(VMProgram *vmp, int label) {
  return VM_Label_Define( vmp, label, vmp->proc_table.target_proc_num,
   Arr_NumItem(vmp->proc_table.target_proc->code) );
}

/* Remove a label from the list of labels. The label should not have
 * unresolved references.
 */
Task VM_Label_Destroy(VMProgram *vmp, int label) {
  VMLabel *l = NULL;
  TASK( Clc_Object_Ptr(vmp->labels, & *(void **) l, label) );
  if ( l->chain_unresolved != -1 ) {
    MSG_ERROR("Trying to destroy a label with unresolved references.");
    return Failed;
  }
  TASK( Clc_Release(vmp->labels, label) );
  return Success;
}

Task VM_Label_Jump(VMProgram *vmp, int label, int is_conditional) {
  VMProcTable *pt = & vmp->proc_table;
  VMLabel *l = NULL;
  int not_defined;
  AsmCode asm_of_jmp = is_conditional ? ASM_JC_I : ASM_JMP_I;
  int target_proc_num = pt->target_proc_num;
  int current_position = Arr_NumItem(pt->target_proc->code);

  TASK( Clc_Object_Ptr(vmp->labels, & *(void **) l, label) );
  not_defined = (l->position == -1);

  if ( l->sheet_id != target_proc_num ) {
    MSG_ERROR("This label refers to code outside the current sheet.");
    return Failed;
  }

  if ( ! not_defined ) {
    if ( l->position < 0 ) {
      MSG_ERROR("Found label referring to invalid position.");
      return Failed;
    }
    VM_Assemble(vmp, asm_of_jmp, CAT_IMM, l->position - current_position);
    return Success;

  } else {
    /* We cannot assemble the instruction now, since we don't know where
     * the label is. So we store this reference in the list of references.
     */
    VMReference r;
    int ref_index;

    if ( vmp->references == (Collection *) NULL ) {
      TASK( Clc_New(& vmp->references, sizeof(VMReference),
       VM_TYPICAL_NUM_LABELS) );
    }

    r.kind = asm_of_jmp;
    r.position = current_position;
    r.next = l->chain_unresolved;
    TASK( Clc_Occupy(vmp->references,  (void *) & r, & ref_index) );

    l->chain_unresolved = ref_index;

    /* For now we assemble a dummy instance of the jump instruction */
    VM_Assemble(vmp, asm_of_jmp, CAT_IMM, (Intg) 0);
    return Success;
  }
}

/*****************************************************************************
 * Functions to assemble code                                                *
 *****************************************************************************/

/* Imposta le opzioni per l'assemblaggio:
 * L'opzione puo' essere settata con un valore > 0, resettata con 0
 * e lasciata inalterata con un valore < 0.
 *  1) forcelong: forza la scrittura di tutte le istruzioni in formato lungo.
 *  2) error: indica se, nel corso della scrittura del programma,
 *   si e' verificato un errore;
 *  3) inhibit: inibisce la scrittura del programma (per errori ad esempio).
 * NOTA: Le opzioni da error in poi "appartengono" al programma attualmente
 *  in scrittura.
 */
void VM_ASettings(VMProgram *vmp, int forcelong, int error, int inhibit) {
  VMProcTable *pt = & vmp->proc_table;
  vmp->vm_aflags.forcelong = forcelong;
  pt->target_proc->status.error = error;
  pt->target_proc->status.inhibit = inhibit;
}

/* This function executes the final steps to prepare the program
 * to be installed as a module and to be executed.
 * num_reg and num_var are the pointers to arrays of NUM_TYPES elements
 * containing the numbers of registers and variables used by the program
 * for every type.
 * module is the module-number of an undefined module which will be used
 * to install the program.
 */
Task VM_Code_Prepare(VMProgram *vmp, Intg *num_var, Intg *num_reg) {
  VMProcTable *pt = & vmp->proc_table;
  int previous_sheet;
  UInt tmp_sheet_id = 0;
  Array *entry = pt->target_proc->code;
  Task exit_status = Failed;

  VM_Assemble(vmp, ASM_RET);

  previous_sheet = VM_Proc_Target_Get(vmp);
  TASK( VM_Proc_Code_New(vmp, & tmp_sheet_id) );
  if IS_FAILED( VM_Proc_Target_Set(vmp, tmp_sheet_id) ) goto exit;

  {
    register Intg i;
    Intg instruction[NUM_TYPES] = {
      ASM_NEWC_II, ASM_NEWI_II, ASM_NEWR_II, ASM_NEWP_II, ASM_NEWO_II
    };

    for(i = 0; i < NUM_TYPES; i++) {
      register Intg nv = num_var[i], nr = num_reg[i];
      if ( nv < 0 || nr < 0 ) {
        MSG_ERROR("Errore nella chiamata di VM_Code_Prepare.");
        goto exit;
      }
      if ( nv > 0 || nr > 0 )
        VM_Assemble(vmp, instruction[i], CAT_IMM, nv, CAT_IMM, nr);
    }
  }

  /* Insert the program just written inside tmp_code at the beginning
   * of the main program 'entry' (which is the one selected before entering
   * this function).
   */
  {
    Array *code_to_insert = pt->target_proc->code;
    int code_to_insert_len = Arr_NumItem(code_to_insert);
    void *code_to_insert_ptr = Arr_Ptr(code_to_insert);

    if IS_FAILED(Arr_Insert(entry, 1, code_to_insert_len, code_to_insert_ptr) )
      goto exit;
  }

  exit_status = Success;

exit:
  (void) VM_Proc_Target_Set(vmp, previous_sheet);
  if ( tmp_sheet_id > 0 )
    (void) VM_Proc_Code_Destroy(vmp, tmp_sheet_id);
  return exit_status;
}

/* Assembla l'istruzione specificata da instr, scrivendo il codice
 * binario ad essa corrispondente nella destinazione specificata
 * dalla funzione VM_Asm_Out_Set().
 */
void VM_Assemble(VMProgram *vmp, AsmCode instr, ...) {
  va_list ap;
  VMProcTable *pt = & vmp->proc_table;
  int i, t;
  VMInstrDesc *idesc;
  int is_short;
  struct {
    TypeID t;  /* Tipi degli argomenti */
    AsmArg c;  /* Categorie degli argomenti */
    void *ptr; /* Puntatori ai valori degli argomenti */
    Intg  vi;   /* Destinazione dei valori...   */
    Real  vr;   /* ...immediati degli argomenti */
    Point vp;
  } arg[VM_MAX_NUMARGS];

  MSG_LOCATION("VM_Assemble");

  /* Esco subito se e' settato il flag di inibizione! */
  if ( pt->target_proc->status.inhibit ) return;

  if ( (instr < 1) || (instr >= ASM_ILLEGAL) ) {
    MSG_ERROR("Istruzione non riconosciuta!");
    return;
  }

  idesc = & vm_instr_desc_table[instr];

  assert( idesc->numargs <= VM_MAX_NUMARGS );

  va_start(ap, instr);

  /* Prendo argomento per argomento */
  t = 0; /* Indice di argomento */
  is_short = 1;
  for ( i = 0; i < idesc->numargs; i++ ) {
    Intg vi = 0;

    /* Prendo dalla lista degli argomenti della funzione la categoria
    * dell'argomento dell'istruzione.
    */
    switch ( arg[t].c = va_arg(ap, AsmArg) ) {
      case CAT_LREG:
      case CAT_GREG:
      case CAT_PTR:
        arg[t].t = TYPE_INTG;
        arg[t].vi = vi = va_arg(ap, Intg);
        arg[t].ptr = (void *) (& arg[t].vi);
        break;

      case CAT_IMM:
        switch (idesc->t_id) {
          case TYPE_CHAR:
            arg[t].t = TYPE_CHAR;
            arg[t].vi = va_arg(ap, Intg); vi = 0;
            arg[t].ptr = (void *) (& arg[t].vi);
            break;
          case TYPE_INTG:
            arg[t].t = TYPE_INTG;
            arg[t].vi = vi = va_arg(ap, Intg);
            arg[t].ptr = (void *) (& arg[t].vi);
            break;
          case TYPE_REAL:
            is_short = 0;
            arg[t].t = TYPE_REAL;
            arg[t].vr = va_arg(ap, Real);
            arg[t].ptr = (void *) (& arg[t].vr);
            break;
          case TYPE_POINT:
            is_short = 0;
            arg[t].t = TYPE_POINT;
            arg[t].vp.x = va_arg(ap, Real);
            arg[t].vp.y = va_arg(ap, Real);
            arg[t].ptr = (void *) (& arg[t].vp);
            break;
          default:
            is_short = 0;
            break;
        }
        break;

      default:
        MSG_ERROR("Categoria di argomenti sconosciuta!");
        pt->target_proc->status.error = 1;
        pt->target_proc->status.inhibit = 1;
        break;
    }

    if (is_short) {
      /* Controllo che l'argomento possa essere "contenuto"
      * nel formato corto.
      */
      vi &= ~0x7fL;
      if ( (vi != 0) && (vi != ~0x7fL) )
        is_short = 0;
    }

    ++t;
  }

  va_end(ap);

  assert(t == idesc->numargs);

  /* Cerco di capire se e' possibile scrivere l'istruzione in formato corto */
  if ( vmp->vm_aflags.forcelong ) is_short = 0;
  if ( (is_short == 1) && (t <= 2) ) {
    /* L'istruzione va scritta in formato corto! */
    VMByteX4 buffer[1], *i_pos = buffer;
    register VMByteX4 i_eye;
    UInt atype;
    Array *prog = pt->target_proc->code;

    for ( ; t < 2; t++ ) {
      arg[t].c = 0;
      arg[t].vi = 0;
    }

    atype = (arg[1].c << 2) | arg[0].c;
    ASM_SHORT_PUT_HEADER(i_pos, i_eye, instr, /* is_long = */ 0,
                         /* i_len = */ 1, atype);
    ASM_SHORT_PUT_2ARGS(i_pos, i_eye, arg[0].vi, arg[1].vi);

    if IS_FAILED( Arr_Push(prog, buffer) ) {
      pt->target_proc->status.error = 1;
      pt->target_proc->status.inhibit = 1;
      return;
    }
    return;

  } else {
    /* L'istruzione va scritta in formato lungo! */
    UInt idim, iheadpos;
    VMByteX4 iw[MAX_SIZE_IN_IWORDS];
    Array *prog = pt->target_proc->code;

    /* Lascio il posto per la "testa" dell'istruzione (non conoscendo ancora
    * la dimensione dell'istruzione, non posso scrivere adesso la testa.
    * Potro' farlo solo alla fine, dopo aver scritto tutti gli argomenti!)
    */
    iheadpos = Arr_NumItem(prog) + 1;
    Arr_MInc(prog, idim = 2);

    for ( i = 0; i < t; i++ ) {
      UInt adim, aiwdim;

      adim = size_of_type[arg[i].t];
      aiwdim = (adim + sizeof(VMByteX4) - 1) / sizeof(VMByteX4);
      iw[aiwdim - 1] = 0;
      (void) memcpy( iw, arg[i].ptr, adim );

      if IS_FAILED( Arr_MPush(prog, iw, aiwdim) ) {
        pt->target_proc->status.error = 1;
        pt->target_proc->status.inhibit = 1;
        return;
      }

      idim += aiwdim;
    }

    {
      VMByteX4 *i_pos;
      register VMByteX4 i_eye;
      UInt atype;

      /* Trovo il puntatore alla testa dell'istruzione */
      i_pos = Arr_ItemPtr(prog, VMByteX4, iheadpos);

      for ( ; t < 2; t++ )
        arg[t].c = 0;

      /* Scrivo la "testa" dell'istruzione */
      atype = (arg[1].c << 2) | arg[0].c;
      ASM_LONG_PUT_HEADER(i_pos, i_eye, instr, /* is_long = */ 1,
                          /* i_len = */ idim, atype);
    }
  }
}
