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

/* This files implements the functionality of the Virtual Machine (VM)
 * of Box: defines functions to assemble, disassemble, execute basic
 * instructions.
 */
/*#define DEBUG_EXEC*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "mem.h"
#include "str.h"
#include "messages.h"
#include "array.h"
#include "occupation.h"
#include "virtmach.h"
#include "vmsym.h"
#include "vmproc.h"
#include "vmalloc.h"

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

/* Static functions defined in this file */
static void Update_gr0(BoxVM *vm);

/* This array lets us to obtain the size of a type by type index.
 * (Useful in what follows)
 */
const UInt size_of_type[NUM_TYPES] = {
  sizeof(Char),
  sizeof(Int),
  sizeof(Real),
  sizeof(Point),
  sizeof(Obj)
};

/* Restituisce il puntatore al registro globale n-esimo. */
static void *VM__Get_G(VMStatus *vmcur, Int n) {
  register TypeID t = vmcur->idesc->t_id;

  assert(t >= 0 && t < NUM_TYPES);

#ifdef VM_SAFE_EXEC1
  if (n < vmcur->gmin[t] || n > vmcur->gmax[t]) {
    MSG_ERROR("Riferimento a registro globale non allocato!");
    vmcur->flags.error = vmcur->flags.exit = 1;
    return NULL;
  }
#endif
  return ((void *) vmcur->global[t] + (Int) (n * size_of_type[t]));
}

/* Restituisce il puntatore al registro locale n-esimo. */
static void *VM__Get_L(VMStatus *vmcur, Int n) {
  register TypeID t = vmcur->idesc->t_id;

  assert(t >= 0 && t < NUM_TYPES);

#ifdef VM_SAFE_EXEC1
  if ( (n < vmcur->lmin[t]) || (n > vmcur->lmax[t]) ) {
  MSG_ERROR("Trying to access unallocated local register(t:%I,n:%I)!", t,  n);
  vmcur->flags.error = vmcur->flags.exit = 1;
  return NULL;
  }
#endif
  return ((void *) vmcur->local[t] + (Int) (n * size_of_type[t]));
}

/* Restituisce il puntatore alla locazione di memoria (ro0 + n). */
static void *VM__Get_P(VMStatus *vmcur, Int n) {
  return *((void **) vmcur->local[TYPE_OBJ]) + n;
}

/* Restituisce il puntatore ad un intero/reale di valore n. */
static void *VM__Get_I(VMStatus *vmcur, Int n) {
  register TypeID t = vmcur->idesc->t_id;
  static Int i = 0;
  static union { Char c; Int i; Real r; } v[2], *value;

  assert(t >= TYPE_CHAR && t <= TYPE_REAL);

  value = & v[i]; i ^= 1;
  switch (t) {
  case TYPE_CHAR:
    value->c = (Char) n; return (void *) (& value->c);
  case TYPE_INT:
    value->i =  (Int) n; return (void *) (& value->i);
  case TYPE_REAL:
    value->r = (Real) n; return (void *) (& value->r);
  default: /* This shouldn't happen! */
    return (void *) (& value->i); break;
  }
}

/* Array di puntatori alle 4 funzioni sopra: */
static void *(*vm_gets[4])(VMStatus *, Int) = {
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
  Int iaint[2];

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
      Int iai = iaint[n], uiai = iai;
      char rc, tc;
      const char typechars[NUM_TYPES] = "cirpo";

      tc = typechars[iat];
      if (uiai < 0) {uiai = -uiai; rc = 'v';} else rc = 'r';
      switch(iaf) {
        case CAT_GREG:
          sprintf(vmp->iarg_str[n], "g%c%c" SInt, rc, tc, uiai);
          break;
        case CAT_LREG:
          sprintf(vmp->iarg_str[n], "%c%c" SInt, rc, tc, uiai);
          break;
        case CAT_PTR:
          if ( iai < 0 )
            sprintf(vmp->iarg_str[n], "%c[ro0 - " SInt "]", tc, uiai);
          else if ( iai == 0 )
            sprintf(vmp->iarg_str[n], "%c[ro0]", tc);
          else
            sprintf(vmp->iarg_str[n], "%c[ro0 + " SInt "]", tc, uiai);
          break;
        case CAT_IMM:
          if ( iat == TYPE_CHAR ) iai = (Int) ((Char) iai);
          sprintf(vmp->iarg_str[n], SInt, iai);
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
    Int call_num;
    void *arg2;

    if ( vmcur->flags.is_long ) {
      ASM_LONG_GET_1ARG( vmcur->i_pos, vmcur->i_eye, call_num);
      arg2 = vmcur->i_pos;
    } else {
      ASM_SHORT_GET_1ARG( vmcur->i_pos, vmcur->i_eye, call_num);
      arg2 = vmcur->i_pos;
    }

    if ( iat == TYPE_CHAR ) call_num = (Int) ((Char) call_num);
    {
      VMProcTable *pt = & vmp->proc_table;
      if (call_num < 1 || call_num > BoxArr_Num_Items(& pt->installed)) {
        sprintf(vmp->iarg_str[0], SInt, call_num);
        return;

      } else {
        char *call_name;
        VMProcInstalled *p;
        p = (VMProcInstalled *) BoxArr_Item_Ptr(& pt->installed, call_num);
        call_name = Str_Cut(p->desc, 40, 85);
        sprintf(vmp->iarg_str[0], SInt"('%.40s')", call_num, call_name);
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
    Int m_num;
    Int position;
    void *arg2;

    if ( vmcur->flags.is_long ) {
      ASM_LONG_GET_1ARG( vmcur->i_pos, vmcur->i_eye, m_num);
      arg2 = vmcur->i_pos;
    } else {
      ASM_SHORT_GET_1ARG( vmcur->i_pos, vmcur->i_eye, m_num);
      arg2 = vmcur->i_pos;
    }

    if ( iat == TYPE_CHAR ) m_num = (Int) ((Char) m_num);

    position = (vmcur->dasm_pos + m_num)*sizeof(VMByteX4);
    sprintf(vmp->iarg_str[0], SInt, position);
    return;

  } else {
    VM__D_GLPI_GLPI(vmp, iarg);
  }
}

/* Analoga alla precedente, ma per istruzioni del tipo GLPI-Imm. */
void VM__D_GLPI_Imm(VMProgram *vmp, char **iarg) {
  VMStatus *vmcur = vmp->vmcur;
  UInt iaf = vmcur->arg_type & 3, iat = vmcur->idesc->t_id;
  Int iai;
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
    Int uiai = iai;
    char rc, tc;
    const char typechars[NUM_TYPES] = "cirpo";

    tc = typechars[iat];
    if (uiai < 0) {uiai = -uiai; rc = 'v';} else rc = 'r';
    switch(iaf) {
      case CAT_GREG:
        sprintf(vmp->iarg_str[0], "g%c%c" SInt, rc, tc, uiai);
        break;
      case CAT_LREG:
        sprintf(vmp->iarg_str[0], "%c%c" SInt, rc, tc, uiai);
        break;
      case CAT_PTR:
        if ( iai < 0 )
          sprintf(vmp->iarg_str[0], "%c[ro0 - " SInt "]", tc, uiai);
        else if ( iai == 0 )
          sprintf(vmp->iarg_str[0], "%c[ro0]", tc);
        else
          sprintf(vmp->iarg_str[0], "%c[ro0 + " SInt "]", tc, uiai);
        break;
      case CAT_IMM:
        sprintf(vmp->iarg_str[0], SInt, iai);
        break;
    }
  }

  /* Secondo argomento */
  switch (iat) {
    case TYPE_CHAR:
      sprintf( vmp->iarg_str[1], SChar, *((Char *) arg2) );
      break;
    case TYPE_INT:
      sprintf( vmp->iarg_str[1], SInt, *((Int *) arg2) );
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

Task BoxVM_Init(BoxVM *vm) {
  vm->vm_globals = 0;
  vm->attr.hexcode = 0;
  vm->attr.forcelong = 0;
  vm->attr.identdata = 0;

  BoxOpTable_Build(& vm->op_table);

  BoxArr_Init(& vm->stack, sizeof(Obj), 10);
  BoxArr_Init(& vm->data_segment, sizeof(char), CMP_TYPICAL_DATA_SIZE);

  if (BoxArr_Is_Err(& vm->stack) || BoxArr_Is_Err(& vm->data_segment))
    return Failed;

  TASK( VM_Proc_Init(vm) );
  BoxVMSymTable_Init(& vm->sym_table);
  TASK( BoxVM_Alloc_Init(vm) );
  return Success;
}

static void _Free_Globals(VMProgram *vmp) {
  int i;
  for(i = 0; i < NUM_TYPES; i++) {
    void *ptr = vmp->vm_global[i];
    if (ptr != NULL)
      free(ptr + vmp->vm_gmin[i]*size_of_type[i]);
    vmp->vm_global[i] = NULL;
    vmp->vm_gmin[i] = 1;
    vmp->vm_gmax[i] = -1;
  }
  vmp->vm_globals = 0;
}

void BoxVM_Finish(BoxVM *vm) {
  if (vm == (VMProgram *) NULL) return;
  if (vm->vm_globals != 0) _Free_Globals(vm);

  if (BoxArr_Num_Items(& vm->stack) != 0)
    MSG_WARNING("Run finished with non empty stack.");
  BoxArr_Finish(& vm->stack);

  BoxArr_Finish(& vm->data_segment);

  BoxVM_Alloc_Destroy(vm);
  BoxVMSymTable_Finish(& vm->sym_table);
  VM_Proc_Destroy(vm);
  BoxOpTable_Destroy(& vm->op_table);
}

BoxVM *BoxVM_New(void) {
  BoxVM *vm = BoxMem_Alloc(sizeof(BoxVM));
  if (vm == NULL) return NULL;
  if (BoxVM_Init(vm) == Failed) {
    BoxMem_Free(vm);
    return NULL;
  }
  return vm;
}

void BoxVM_Destroy(BoxVM *vm) {
  if (vm == NULL) return;
  BoxVM_Finish(vm);
  BoxMem_Free(vm);
}

BoxOpInfo *BoxVM_Get_Op_Info(BoxVM *vm, BoxGOp g_op) {
  assert(g_op >= 0 && g_op < BOX_NUM_GOPS);
  return & vm->op_table.info[g_op];
}

/* Sets the number of global registers and variables for each type. */
Task BoxVM_Alloc_Global_Regs(BoxVM *vm, Int num_var[], Int num_reg[]) {
  int i;
  Obj *reg_obj;

  assert(vm != NULL);

  if (vm->vm_globals != 0) _Free_Globals(vm);

  for(i = 0; i < NUM_TYPES; i++) {
    Int nv = num_var[i], nr = num_reg[i];
    void *ptr;

    if (nv < 0 || nr < 0) {
      MSG_ERROR("Wrong allocation numbers for global registers.");
      _Free_Globals(vm);
      return Failed;
    }

    if (nr < 3) nr = 3; /* gro0, gro1, gro2 are always needed! */
    ptr = calloc(nv + nr + 1, size_of_type[i]);
    if (ptr == NULL) {
      MSG_ERROR("Error in the allocation of the local registers.");
      _Free_Globals(vm);
      return Failed;
    }

    vm->vm_global[i] = ptr + nv*size_of_type[i];
    vm->vm_gmin[i] = -nv;
    vm->vm_gmax[i] = nr;
    vm->vm_globals = 1; /* This line must stay here, not outside the loop! */
  }

  reg_obj = (Obj *) vm->vm_global[TYPE_OBJ];
  vm->box_vm_current = reg_obj + 1;
  vm->box_vm_arg1    = reg_obj + 2;
  vm->box_vm_arg2    = reg_obj + 3;
  Update_gr0(vm);
  return Success;
}

/* Sets the value for the global register (or variable)
 * of type type, whose number is reg. *value is the value assigned
 * to the register (variable).
 */
Task VM_Module_Global_Set(VMProgram *vmp, Int type, Int reg, void *value) {
  void *dest;

  if (type < 0 || type >= NUM_TYPES) {
    MSG_ERROR("VM_Module_Global_Set: Unknown register type!");
    return Failed;
  }

  if (reg < vmp->vm_gmin[type] || reg > vmp->vm_gmax[type]) {
    MSG_ERROR("VM_Module_Global_Set: Reference to unallocated register!");
    return Failed;
  }

  dest = vmp->vm_global[type] + reg * size_of_type[type];
  memcpy(dest, value, size_of_type[type]);
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
  VMStatus vm, *vm_save;
  BoxValue reg0[NUM_TYPES]; /* Registri locali numero zero */
#ifdef DEBUG_EXEC
  Int i = 0;
#endif

  /* Controlliamo che il modulo sia installato! */
  if (call_num < 1 || call_num > BoxArr_Num_Items(& pt->installed)) {
    MSG_ERROR("Call to the undefined procedure %d.", call_num);
    return Failed;
  }

  p = (VMProcInstalled *) BoxArr_Item_Ptr(& pt->installed, call_num);
  switch (p->type) {
    case VMPROC_IS_C_CODE: return p->code.c(vmp);
    case VMPROC_IS_VM_CODE: break;
    default:
      MSG_ERROR("Call into the broken procedure %d.", call_num);
      return Failed;
  }

  vm_save = vmp->vmcur;
  vmp->vmcur = & vm;

  {
    int i;
    for(i = 0; i < NUM_TYPES; i++) {
      vm.lmin[i] = 0; vm.lmax[i] = 0; vm.local[i] = & reg0[i];
      if (vmp->vm_global[i] != NULL) {
        vm.gmin[i] = vmp->vm_gmin[i]; vm.gmax[i] = vmp->vm_gmax[i];
        vm.global[i] = vmp->vm_global[i];
      } else {
        vm.gmin[i] = 1; vm.gmax[i] = -1; vm.global[i] = NULL;
      }
    }
  }


  vm.p = p;
  VM_Proc_Ptr_And_Length(vmp, & vm.i_pos, (UInt *) NULL, p->code.proc_num);
  i_pos = vm.i_pos;
  vm.flags.exit = vm.flags.error = 0;
  {int i; for(i = 0; i < NUM_TYPES; i++) vm.alc[i] = 0;}

  do {
    register int is_long;
    register VMByteX4 i_eye;

#ifdef DEBUG_EXEC
    fprintf(stderr, "module = "SInt", pos = "SInt" - reading instruction.\n",
            call_num, i*sizeof(VMByteX4));
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
      vmp->vmcur = vm_save;
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

  vmp->vmcur = vm_save;
  return vm.flags.error ? Failed : Success;
}

/*****************************************************************************
 * Functions to disassemble code                                             *
 *****************************************************************************/

int BoxVM_Set_Force_Long(BoxVM *vm, int force_long) {
  int is_long = vm->attr.forcelong;
  if ((force_long | 1) == 1) /* If force_long != 0, 1 the flag is not set */
    vm->attr.forcelong = force_long;
  return is_long;
}

void BoxVM_Set_Attr(BoxVM *vm, BoxVMAttr mask, BoxVMAttr value) {
  if ((mask & BOXVM_ATTR_ASM_LONG_FMT) != 0)
    vm->attr.forcelong = ((value & BOXVM_ATTR_ASM_LONG_FMT) != 0);
  if ((mask & BOXVM_ATTR_DASM_WITH_HEX) != 0)
    vm->attr.hexcode = ((value & BOXVM_ATTR_DASM_WITH_HEX) != 0);
  if ((mask & BOXVM_ATTR_ADD_DATA_IDENT) != 0)
    vm->attr.identdata = ((value & BOXVM_ATTR_ADD_DATA_IDENT) != 0);
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
      fprintf(output, SUInt "\t"BoxVMByteX4_Fmt"x\tError!",
              (UInt) (pos * sizeof(VMByteX4)), *i_pos);

    } else {
      int i;
      VMByteX4 *i_pos2 = i_pos;

      /* Stampo l'istruzione e i suoi argomenti */
      fprintf( output, SUInt "\t", (UInt) (pos * sizeof(VMByteX4)) );
      if ( vmp->attr.hexcode )
        fprintf(output, BoxVMByteX4_Fmt"\t", *(i_pos2++));
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
      if (vmp->attr.hexcode) {
        for(i = 1; i < vm.i_len; i++)
          fprintf(output, "\t"BoxVMByteX4_Fmt"\n", *(i_pos2++));
      }
    }

    /* Passo alla prossima istruzione */
    if ( vm.i_len < 1 ) return Failed;

    vm.i_pos = (i_pos += vm.i_len);
    pos += vm.i_len;
  }
  return Success;
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
  vmp->attr.forcelong = forcelong;
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
Task VM_Code_Prepare(VMProgram *vmp, Int *num_var, Int *num_reg) {
  VMProcTable *pt = & vmp->proc_table;
  int previous_sheet;
  UInt tmp_sheet_id = 0;
  BoxArr *entry = & pt->target_proc->code;
  Task exit_status = Failed;

  VM_Assemble(vmp, ASM_RET);

  previous_sheet = BoxVM_Proc_Target_Get(vmp);
  TASK( VM_Proc_Code_New(vmp, & tmp_sheet_id) );
  BoxVM_Proc_Target_Set(vmp, tmp_sheet_id);

  {
    register Int i;
    Int instruction[NUM_TYPES] = {
      ASM_NEWC_II, ASM_NEWI_II, ASM_NEWR_II, ASM_NEWP_II, ASM_NEWO_II
    };

    for(i = 0; i < NUM_TYPES; i++) {
      register Int nv = num_var[i], nr = num_reg[i];
      if (nv < 0 || nr < 0) {
        MSG_ERROR("Errore nella chiamata di VM_Code_Prepare.");
        goto exit;
      }
      if (nv > 0 || nr > 0)
        VM_Assemble(vmp, instruction[i], CAT_IMM, nv, CAT_IMM, nr);
    }
  }

  /* Insert the program just written inside tmp_code at the beginning
   * of the main program 'entry' (which is the one selected before entering
   * this function).
   */
  {
    BoxArr *code_to_insert = & pt->target_proc->code;
    int code_to_insert_len = BoxArr_Num_Items(code_to_insert);
    void *code_to_insert_ptr = BoxArr_First_Item_Ptr(code_to_insert);
    BoxArr_Insert(entry, 1, code_to_insert_ptr, code_to_insert_len);
  }

  exit_status = Success;

exit:
  (void) BoxVM_Proc_Target_Set(vmp, previous_sheet);
  if ( tmp_sheet_id > 0 )
    (void) VM_Proc_Code_Destroy(vmp, tmp_sheet_id);
  return exit_status;
}

/** Similar to VM_Assemble, but takes a va_list argument as a replacement
 * for the extra arguments.
 */
void VM_VA_Assemble(BoxVM *vmp, AsmCode instr, va_list ap) {
  VMProcTable *pt = & vmp->proc_table;
  int i, t;
  VMInstrDesc *idesc;
  int is_short;
  struct {
    TypeID t;  /* Tipi degli argomenti */
    AsmArg c;  /* Categorie degli argomenti */
    void *ptr; /* Puntatori ai valori degli argomenti */
    Int   vi;  /* Destinazione dei valori...   */
    Real  vr;  /* ...immediati degli argomenti */
    Point vp;
  } arg[VM_MAX_NUMARGS];

  /* Esco subito se e' settato il flag di inibizione! */
  if (pt->target_proc->status.inhibit) return;

  if (instr < 1 || instr >= ASM_ILLEGAL) {
    MSG_ERROR("Istruzione non riconosciuta!");
    return;
  }

  idesc = & vm_instr_desc_table[instr];

  assert( idesc->numargs <= VM_MAX_NUMARGS );

  /* Prendo argomento per argomento */
  t = 0; /* Indice di argomento */
  is_short = 1;
  for ( i = 0; i < idesc->numargs; i++ ) {
    Int vi = 0;

    /* Prendo dalla lista degli argomenti della funzione la categoria
    * dell'argomento dell'istruzione.
    */
    switch (arg[t].c = va_arg(ap, AsmArg)) {
      case CAT_LREG:
      case CAT_GREG:
      case CAT_PTR:
        arg[t].t = TYPE_INT;
        arg[t].vi = vi = va_arg(ap, Int);
        arg[t].ptr = (void *) (& arg[t].vi);
        break;

      case CAT_IMM:
        switch (idesc->t_id) {
          case TYPE_CHAR:
            arg[t].t = TYPE_CHAR;
            arg[t].vi = va_arg(ap, Int); vi = 0;
            arg[t].ptr = (void *) (& arg[t].vi);
            break;
          case TYPE_INT:
            arg[t].t = TYPE_INT;
            arg[t].vi = vi = va_arg(ap, Int);
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

  assert(t == idesc->numargs);

  /* Cerco di capire se e' possibile scrivere l'istruzione in formato corto */
  if ( vmp->attr.forcelong ) is_short = 0;
  if ( (is_short == 1) && (t <= 2) ) {
    /* L'istruzione va scritta in formato corto! */
    VMByteX4 buffer[1], *i_pos = buffer;
    register VMByteX4 i_eye;
    UInt atype;
    BoxArr *prog = & pt->target_proc->code;

    for ( ; t < 2; t++ ) {
      arg[t].c = 0;
      arg[t].vi = 0;
    }

    atype = (arg[1].c << 2) | arg[0].c;
    ASM_SHORT_PUT_HEADER(i_pos, i_eye, instr, /* is_long = */ 0,
                         /* i_len = */ 1, atype);
    ASM_SHORT_PUT_2ARGS(i_pos, i_eye, arg[0].vi, arg[1].vi);

    BoxArr_Push(prog, buffer);
    return;

  } else {
    /* L'istruzione va scritta in formato lungo! */
    UInt idim, iheadpos;
    VMByteX4 iw[MAX_SIZE_IN_IWORDS];
    BoxArr *prog = & pt->target_proc->code;

    /* Lascio il posto per la "testa" dell'istruzione (non conoscendo ancora
     * la dimensione dell'istruzione, non posso scrivere adesso la testa.
     * Potro' farlo solo alla fine, dopo aver scritto tutti gli argomenti!)
     */
    iheadpos = BoxArr_Num_Items(prog) + 1;
    BoxArr_MPush(prog, NULL, idim = 2);

    for ( i = 0; i < t; i++ ) {
      UInt adim, aiwdim;

      adim = size_of_type[arg[i].t];
      aiwdim = (adim + sizeof(VMByteX4) - 1) / sizeof(VMByteX4);
      iw[aiwdim - 1] = 0;
      (void) memcpy( iw, arg[i].ptr, adim );

      BoxArr_MPush(prog, iw, aiwdim);
      idim += aiwdim;
    }

    {
      VMByteX4 *i_pos;
      register VMByteX4 i_eye;
      UInt atype;

      /* Trovo il puntatore alla testa dell'istruzione */
      i_pos = (VMByteX4 *) BoxArr_Item_Ptr(prog, iheadpos);

      for ( ; t < 2; t++ )
        arg[t].c = 0;

      /* Scrivo la "testa" dell'istruzione */
      atype = (arg[1].c << 2) | arg[0].c;
      ASM_LONG_PUT_HEADER(i_pos, i_eye, instr, /* is_long = */ 1,
                          /* i_len = */ idim, atype);
    }
  }
}

/** Assembla l'istruzione specificata da instr, scrivendo il codice
 * binario ad essa corrispondente nella destinazione specificata
 * dalla funzione VM_Asm_Out_Set().
 */
void VM_Assemble(BoxVM *vm, AsmCode instr, ...) {
  va_list ap;
  va_start(ap, instr);
  VM_VA_Assemble(vm, instr, ap);
  va_end(ap);
}

/****************************************************************************
 * Code to handle the DATA SEGMENT, the region of memory where values for   *
 * strings and other objects is put.                                        *
 ****************************************************************************/

typedef struct {
  Int type;
  Int size;
} DataItem;

/* This function adds a new piece of data to the data segment.
 * NOTE: It returns the address of the data item with respect to the beginning
 *  of the data segment.
 */
UInt BoxVM_Data_Add(BoxVM *vm, const void *data, UInt size, Int type) {
  Int addr;

  /* Now we insert the data descriptor (for debug mainly), if necessary */
  if (vm->attr.identdata) {
    DataItem di;
    di.type = type;
    di.size = size;
    BoxArr_MPush(& vm->data_segment, & di, sizeof(di));
  }

  /* And now we insert the piece of data */
  addr = BoxArr_Num_Items(& vm->data_segment);
  BoxArr_MPush(& vm->data_segment, data, size);
  return addr;
}

/* Make sure gr0 is pointing to the data segment */
static void Update_gr0(BoxVM *vm) {
  Obj data_segment_ptr;
  data_segment_ptr.block = NULL; /* the VM will handle deallocation! */
  data_segment_ptr.ptr = BoxArr_First_Item_Ptr(& vm->data_segment);
  ASSERT_TASK( VM_Module_Global_Set(vm, TYPE_OBJ, (Int) 0,
                                    & data_segment_ptr) );
}

void BoxVM_Data_Display(BoxVM *vm, FILE *stream) {
  char *data;
  Int size, pos, ds;
  DataItem *di;

  size = BoxArr_Num_Items(& vm->data_segment);

  if (!vm->attr.identdata) {
    fprintf(stream, "*** DATA SEGMENT WITH SIZE "SUInt" ***\n", size);
    return;
  }

  data = (char *) BoxArr_First_Item_Ptr(& vm->data_segment);

  if (size < 1) {
    fprintf(stream, "*** EMPTY DATA-SEGMENT ***\n");
    return;
  }

  fprintf(stream, "*** CONTENT OF THE DATA-SEGMENT ***\n");

  pos = 0;
  while (pos + sizeof(DataItem) <= size) {
    di = (DataItem *) data;
    fprintf(stream, "  Address "SInt", size "SInt": data of type '"SInt"':\n",
            pos, di->size, di->type);

    ds = sizeof(DataItem) + di->size;
    pos += ds;
    if (di->size < 0 || pos > size) {
      fprintf(stream, "Error: bad data-block.\n");
      MSG_ERROR("Bad block size at position %d.", pos);
      return;
    }
    data += ds;
  }

  fprintf(stream, "*** END OF THE DATA-SEGMENT ***\n");
}
