/****************************************************************************
 * Copyright (C) 2008-2011 by Matteo Franchin                               *
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

/* This files implements the functionality of the Virtual Machine (VM)
 * of Box: defines functions to assemble, disassemble, execute basic
 * instructions.
 */
#define DEBUG_EXEC 0

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "types.h"
#include "defaults.h"
#include "mem.h"
#include "strutils.h"
#include "messages.h"
#include "array.h"
#include "occupation.h"
#include "vmx.h"
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
const size_t size_of_type[NUM_TYPES] = {
  sizeof(Char),
  sizeof(Int),
  sizeof(Real),
  sizeof(Point),
  sizeof(Obj)
};


void BoxVMX_Init(BoxVMX *vmx, BoxVM *vm) {
  vmx->buf_pos = 0;
}



static void *My_Get_Arg_Ptrs(BoxVMX *vmx, int kind, BoxInt n) {
  if (kind < 2) {                 /* kind == 0, 1 (local, global registers) */
    BoxType t = vmx->idesc->t_id;
    BoxVMRegs *regs = & ((kind == 0) ? vmx->global : vmx->local)[t];
    size_t s = size_of_type[t];

#ifdef VM_SAFE_EXEC1
    if (n < regs->min || n > regs->max) {
      MSG_ERROR("Trying to access unallocated %s register(t:%I, n:%I)!",
                (kind == 0) ? "global" : "local", t, n);
      vmx->flags.error = vmx->flags.exit = 1;
      return NULL;
    }
#endif
    return regs->ptr + n*s;

  } else if (kind == 2) {                           /* kind == 2 (pointers) */
    return *((void **) vmx->local[TYPE_OBJ].ptr) + n;

  } else {                                        /* kind == 3 (immediates) */
    BoxType t = vmx->idesc->t_id;
    BoxValue *value = & vmx->buf[vmx->buf_pos];
    vmx->buf_pos ^= 1;

    switch (t) {
    case TYPE_CHAR:
      value->boxchar = (BoxChar) n; return & value->boxchar;
    case TYPE_INT:
      value->boxint  =  (BoxInt) n; return & value->boxint;
    case TYPE_REAL:
      value->boxreal = (BoxReal) n; return & value->boxreal;
    default:
      MSG_FATAL("My_Get_Arg_Ptrs: Unexpected type.")
      assert(0);
      return NULL; break;
    }
  }
}

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
void VM__GLP_GLPI(BoxVMX *vmx) {
  signed long narg1, narg2;
  register UInt atype = vmx->arg_type;

  if (vmx->flags.is_long) {
    ASM_LONG_GET_2ARGS(vmx->i_pos, vmx->i_eye, narg1, narg2);
  } else {
    ASM_SHORT_GET_2ARGS(vmx->i_pos, vmx->i_eye, narg1, narg2);
  }

  vmx->arg1 = My_Get_Arg_Ptrs(vmx, atype & 0x3, narg1);
  vmx->arg2 = My_Get_Arg_Ptrs(vmx, (atype >> 2) & 0x3, narg2);
#if DEBUG_EXEC == 1
  printf("Cathegories: arg1 = %lu - arg2 = %lu\n",
         atype & 0x3, (atype >> 2) & 0x3);
#endif
}

/* Questa funzione e' analoga alla precedente, ma gestisce
 *  istruzioni come: "mov ri1, 123456", "mov rf2, 3.14", "mov rp5, (1, 2)", etc.
 */
void VM__GLP_Imm(BoxVMX *vmx) {
  signed long narg1;
  register BoxUInt atype = vmx->arg_type;

  if (vmx->flags.is_long) {
    ASM_LONG_GET_1ARG(vmx->i_pos, vmx->i_eye, narg1);
  } else {
    ASM_SHORT_GET_1ARG(vmx->i_pos, vmx->i_eye, narg1);
  }

  vmx->arg1 = My_Get_Arg_Ptrs(vmx, atype & 0x3, narg1);
  vmx->arg2 = vmx->i_pos;
}

/* Questa funzione e' analoga alle precedenti, ma gestisce
 *  istruzioni con un solo argomento di tipo GLPI (Globale oppure Locale
 *  o Puntatore o Immediato intero).
 */
void VM__GLPI(BoxVMX *vmx) {
  signed long narg1;
  register BoxUInt atype = vmx->arg_type;

  if ( vmx->flags.is_long ) {
    ASM_LONG_GET_1ARG(vmx->i_pos, vmx->i_eye, narg1);
  } else {
    ASM_SHORT_GET_1ARG(vmx->i_pos, vmx->i_eye, narg1);
  }

  vmx->arg1 = My_Get_Arg_Ptrs(vmx, atype & 0x3, narg1);
}

/* Questa funzione e' analoga alle precedenti, ma gestisce
 *  istruzioni con un solo argomento di tipo immediato (memorizzato subito
 *  di seguito all'istruzione).
 */
void VM__Imm(BoxVMX *vmx) {
  vmx->arg1 = (void *) vmx->i_pos;
}

/*****************************************************************************
 * Functions used to disassemble the instructions (see BoxVM_Disassemble)    *
 *****************************************************************************/

/* Questa funzione serve a disassemblare gli argomenti di
 * un'istruzione di tipo GLPI-GLPI.
 * iarg e' una tabella di puntatori alle stringhe che corrisponderanno
 * agli argomenti disassemblati.
 */
void VM__D_GLPI_GLPI(BoxVMX *vmx, char **out) {
  BoxUInt n, na = vmx->idesc->numargs;
  BoxUInt iaform[2] = {vmx->arg_type & 3, (vmx->arg_type >> 2) & 3};
  BoxInt iaint[2];

  assert(na <= 2);

  /* Recupero i numeri (interi) di registro/puntatore/etc. */
  switch (na) {
  case 1:
    {
      void *arg2;

      if (vmx->flags.is_long) {
        ASM_LONG_GET_1ARG(vmx->i_pos, vmx->i_eye, iaint[0]);
        arg2 = vmx->i_pos;
      } else {
        ASM_SHORT_GET_1ARG(vmx->i_pos, vmx->i_eye, iaint[0]);
        arg2 = vmx->i_pos;
      }
      break;
    }
  case 2:
    if (vmx->flags.is_long) {
      ASM_LONG_GET_2ARGS(vmx->i_pos, vmx->i_eye, iaint[0], iaint[1]);
    } else {
      ASM_SHORT_GET_2ARGS(vmx->i_pos, vmx->i_eye, iaint[0], iaint[1]);
    }
    break;
  }

  for (n = 0; n < na; n++) {
    BoxUInt iaf = iaform[n];
    BoxUInt iat = vmx->idesc->t_id;

    assert(iaf < 4);

    {
      BoxInt iai = iaint[n], uiai = iai;
      char rc, tc;
      const char typechars[NUM_TYPES] = "cirpo";

      tc = typechars[iat];
      if (uiai < 0) {uiai = -uiai; rc = 'v';} else rc = 'r';
      switch(iaf) {
        case CAT_GREG:
          sprintf(out[n], "g%c%c" SInt, rc, tc, uiai);
          break;
        case CAT_LREG:
          sprintf(out[n], "%c%c" SInt, rc, tc, uiai);
          break;
        case CAT_PTR:
          if ( iai < 0 )
            sprintf(out[n], "%c[ro0 - " SInt "]", tc, uiai);
          else if ( iai == 0 )
            sprintf(out[n], "%c[ro0]", tc);
          else
            sprintf(out[n], "%c[ro0 + " SInt "]", tc, uiai);
          break;
        case CAT_IMM:
          if (iat == TYPE_CHAR) iai = (Int) ((Char) iai);
          sprintf(out[n], SInt, iai);
          break;
      }
    }
  }
}

/* Analoga alla precedente, ma per istruzioni CALL. */
void VM__D_CALL(BoxVMX *vmx, char **out) {
  register BoxUInt na = vmx->idesc->numargs;

  assert(na == 1);

  if ((vmx->arg_type & 3) == CAT_IMM) {
    BoxUInt iat = vmx->idesc->t_id;
    BoxInt call_num;
    void *arg2;

    if (vmx->flags.is_long) {
      ASM_LONG_GET_1ARG(vmx->i_pos, vmx->i_eye, call_num);
      arg2 = vmx->i_pos;
    } else {
      ASM_SHORT_GET_1ARG(vmx->i_pos, vmx->i_eye, call_num);
      arg2 = vmx->i_pos;
    }

    if (iat == TYPE_CHAR) call_num = (Int) ((Char) call_num);
    {
      VMProcTable *pt = & vmx->vm->proc_table;
      if (call_num < 1 || call_num > BoxArr_Num_Items(& pt->installed)) {
        sprintf(out[0], SInt, call_num);
        return;

      } else {
        char *call_name;
        VMProcInstalled *p;
        p = (VMProcInstalled *) BoxArr_Item_Ptr(& pt->installed, call_num);
        call_name = Str_Cut(p->desc, 40, 85);
        sprintf(out[0], SInt"('%.40s')", call_num, call_name);
        BoxMem_Free(call_name);
        return;
      }

    }

  } else {
    VM__D_GLPI_GLPI(vmx, out);
  }
}

/* Analoga alla precedente, ma per istruzioni di salto (jmp, jc). */
void VM__D_JMP(BoxVMX *vmx, char **out) {
  register BoxUInt na = vmx->idesc->numargs;

  assert(na == 1);

  if ((vmx->arg_type & 3) == CAT_IMM) {
    BoxUInt iat = vmx->idesc->t_id;
    BoxInt m_num, position;
    void *arg2;

    if (vmx->flags.is_long) {
      ASM_LONG_GET_1ARG(vmx->i_pos, vmx->i_eye, m_num);
      arg2 = vmx->i_pos;
    } else {
      ASM_SHORT_GET_1ARG(vmx->i_pos, vmx->i_eye, m_num);
      arg2 = vmx->i_pos;
    }

    if (iat == TYPE_CHAR) m_num = (Int) ((Char) m_num);

    position = (vmx->vm->dasm_pos + m_num)*sizeof(VMByteX4);
    sprintf(out[0], SInt, position);

  } else {
    VM__D_GLPI_GLPI(vmx, out);
  }
}

/* Analoga alla precedente, ma per istruzioni del tipo GLPI-Imm. */
void VM__D_GLPI_Imm(BoxVMX *vmx, char **out) {
  BoxUInt iaf = vmx->arg_type & 3, iat = vmx->idesc->t_id;
  BoxInt iai;
  VMByteX4 *arg2;

  assert(vmx->idesc->numargs == 2);
  assert(iat < 4);

  /* Recupero il numero (intero) di registro/puntatore/etc. */
  if (vmx->flags.is_long) {
    ASM_LONG_GET_1ARG(vmx->i_pos, vmx->i_eye, iai);
    arg2 = vmx->i_pos;
  } else {
    ASM_SHORT_GET_1ARG(vmx->i_pos, vmx->i_eye, iai);
    arg2 = vmx->i_pos;
  }

  /* Primo argomento */
  {
    BoxInt uiai = iai;
    char rc, tc;
    const char typechars[NUM_TYPES] = "cirpo";

    tc = typechars[iat];
    if (uiai < 0) {uiai = -uiai; rc = 'v';} else rc = 'r';
    switch(iaf) {
      case CAT_GREG:
        sprintf(out[0], "g%c%c" SInt, rc, tc, uiai);
        break;
      case CAT_LREG:
        sprintf(out[0], "%c%c" SInt, rc, tc, uiai);
        break;
      case CAT_PTR:
        if (iai < 0)
          sprintf(out[0], "%c[ro0 - " SInt "]", tc, uiai);
        else if (iai == 0)
          sprintf(out[0], "%c[ro0]", tc);
        else
          sprintf(out[0], "%c[ro0 + " SInt "]", tc, uiai);
        break;
      case CAT_IMM:
        sprintf(out[0], SInt, iai);
        break;
    }
  }

  /* Secondo argomento */
  switch (iat) {
    case BOXTYPE_CHAR:
      sprintf(out[1], SChar, *((Char *) arg2));
      break;
    case BOXTYPE_INT:
      sprintf(out[1], SInt, *((Int *) arg2));
      break;
    case BOXTYPE_REAL:
      sprintf(out[1], SReal, *((Real *) arg2));
      break;
    case BOXTYPE_POINT:
      sprintf(out[1], SPoint,
               ((Point *) arg2)->x, ((Point *) arg2)->y);
      break;
  }
}

/*****************************************************************************
 * Functions for (de)inizialization                                          *
 *****************************************************************************/

Task BoxVM_Init(BoxVM *vm) {
  int i;

  vm->attr.hexcode = 0;
  vm->attr.forcelong = 0;
  vm->attr.identdata = 0;

  vm->has.globals = 0;
  vm->has.op_table = 0;

  /* Table of actions for each VM instruction */
  vm->exec_table = BoxVM_Get_Exec_Table();

  /* Reset the global register boundaries and pointers */
  for(i = 0; i < NUM_TYPES; i++) {
    BoxVMRegs *gregs = & vm->global[i];
    gregs->ptr = NULL;
    gregs->min = 1;
    gregs->max = -1;
  }

  BoxArr_Init(& vm->stack, sizeof(Obj), 10);
  BoxArr_Init(& vm->data_segment, sizeof(char), CMP_TYPICAL_DATA_SIZE);
  BoxArr_Init(& vm->backtrace, sizeof(BoxVMTrace), 32);

  vm->fail_msg = (char *) NULL;

  if (BoxArr_Is_Err(& vm->stack) || BoxArr_Is_Err(& vm->data_segment))
    return Failed;

  BoxVM_Proc_Init(vm);
  BoxVMSymTable_Init(& vm->sym_table);
  TASK( BoxVM_Alloc_Init(vm) );
  return Success;
}

static void _Free_Globals(BoxVM *vmp) {
  int i;
  for(i = 0; i < NUM_TYPES; i++) {
    BoxVMRegs *gregs = & vmp->global[i];
    if (gregs->ptr != NULL)
      BoxMem_Free(gregs->ptr + gregs->min*size_of_type[i]);
    gregs->ptr = NULL;
    gregs->min = 1;
    gregs->max = -1;
  }
  vmp->has.globals = 0;
}

void BoxVM_Finish(BoxVM *vm) {
  if (vm == (BoxVM *) NULL) return;
  if (vm->has.globals)
    _Free_Globals(vm);

  if (BoxArr_Num_Items(& vm->stack) != 0)
    MSG_WARNING("Run finished with non empty stack.");
  BoxArr_Finish(& vm->stack);

  BoxArr_Finish(& vm->data_segment);
  BoxArr_Finish(& vm->backtrace);

  if (vm->fail_msg != NULL)
    BoxMem_Free(vm->fail_msg);

  BoxVM_Alloc_Destroy(vm);
  BoxVMSymTable_Finish(& vm->sym_table);
  BoxVM_Proc_Finish(vm);

  if (vm->has.op_table)
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

void BoxVM_Set_Fail_Msg(BoxVM *vm, const char *msg) {
  if (vm->fail_msg != NULL)
    BoxMem_Free(vm->fail_msg);
  vm->fail_msg = (msg != NULL) ? BoxMem_Strdup(msg) : NULL;
}

BoxOpInfo *BoxVM_Get_Op_Info(BoxVM *vm, BoxGOp g_op) {
  assert(g_op >= 0 && g_op < BOX_NUM_GOPS);
  if (!vm->has.op_table) {
    BoxOpTable_Build(& vm->op_table);
    vm->has.op_table = 1;
  }
  return & vm->op_table.info[g_op];
}

/* Sets the number of global registers and variables for each type. */
Task BoxVM_Alloc_Global_Regs(BoxVM *vm, Int num_var[], Int num_reg[]) {
  int i;
  BoxPtr *reg_obj;

  assert(vm != NULL);

  if (vm->has.globals)
    _Free_Globals(vm);

  for(i = 0; i < NUM_TYPES; i++) {
    Int nv = num_var[i], nr = num_reg[i];
    BoxVMRegs *gregs = & vm->global[i];
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

    gregs->ptr = ptr + nv*size_of_type[i];
    gregs->min = -nv;
    gregs->max = nr;
    vm->has.globals = 1; /* This line must stay here, not outside the loop! */
  }

  reg_obj = (Obj *) vm->global[TYPE_OBJ].ptr;
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
void BoxVM_Module_Global_Set(BoxVM *vmp, Int type, Int reg, void *value) {
  void *dest;
  BoxVMRegs *gregs;

  if (type < 0 || type >= NUM_TYPES) {
    MSG_FATAL("VM_Module_Global_Set: Unknown register type!");
    assert(0);
  }

  gregs = & vmp->global[type];
  if (reg < gregs->min || reg > gregs->max) {
    MSG_FATAL("VM_Module_Global_Set: Reference to unallocated register!");
    assert(0);
  }

  dest = gregs->ptr + reg*size_of_type[type];
  memcpy(dest, value, size_of_type[type]);
}

/*****************************************************************************
 * Functions to execute code                                                 *
 *****************************************************************************/

/* NOTE: we don't want this function to change gro1/gro2, as it may be
 *   indirectly called by the instruction create, which is not supposed to do
 *   so.
 */
BoxTask BoxVM_Module_Execute_With_Args(BoxVMX *vmx, BoxVMCallNum cn,
                                       BoxPtr *parent, BoxPtr *child) {
  BoxTask t;
  BoxPtr save_parent = *vmx->vm->box_vm_current,
         save_child = *vmx->vm->box_vm_arg1;

  if (parent != NULL) {
    *vmx->vm->box_vm_current = *parent;
    BoxVM_Obj_Link(vmx->vm->box_vm_current);

  } else
    BoxPtr_Nullify(vmx->vm->box_vm_current);

  if (child != NULL) {
    *vmx->vm->box_vm_arg1 = *child;
    BoxVM_Obj_Link(vmx->vm->box_vm_arg1);

  } else
    BoxPtr_Nullify(vmx->vm->box_vm_arg1);

  t = BoxVM_Module_Execute(vmx, cn);

  if (!BoxPtr_Is_Detached(vmx->vm->box_vm_current))
    BoxVM_Obj_Unlink(vmx->vm, vmx->vm->box_vm_current);

  if (!BoxPtr_Is_Detached(vmx->vm->box_vm_arg1))
    BoxVM_Obj_Unlink(vmx->vm, vmx->vm->box_vm_arg1);

  *vmx->vm->box_vm_current = save_parent;
  *vmx->vm->box_vm_arg1 = save_child;
  return t;
}

/* Execute the module number m of program vmp.
 * If initial != NULL, *initial is the initial status of the virtual machine.
 */
Task BoxVM_Module_Execute(BoxVMX *vmx, BoxVMCallNum call_num) {
  const BoxVMInstrDesc *exec_table = vmx->vm->exec_table;
  VMProcTable *pt = & vmx->vm->proc_table;
  VMProcInstalled *p;
  register VMByteX4 *i_pos, *i_pos0;
  BoxVMX vm;
  BoxValue reg0[NUM_TYPES]; /* Registri locali numero zero */

#if DEBUG_EXEC == 1
  Int i = 0;
#endif

  /* Controlliamo che il modulo sia installato! */
  if (call_num < 1 || call_num > BoxArr_Num_Items(& pt->installed)) {
    vmx->flags.error = 1;
    vmx->flags.exit = 1;
    MSG_ERROR("Call to the undefined procedure %d.", call_num);
    return Failed;
  }

#if DEBUG_EXEC == 1
    fprintf(stderr, "call module = "SInt": %p@%p\n",
            call_num, BOX_VM_THIS_PTR(vmp, void),
            BOX_VM_ARG_PTR(vmp, void));
#endif

  p = (VMProcInstalled *) BoxArr_Item_Ptr(& pt->installed, call_num);
  switch (p->type) {
  case BOXVMPROC_IS_C_CODE:
    return p->code.c(vmx);
  case BOXVMPROC_IS_VM_CODE:
    break;
  default:
    MSG_ERROR("Call into the broken procedure %d.", call_num);
    return Failed;
  }


  {
    int i;
    vm.global = vmx->global;
    for(i = 0; i < NUM_TYPES; i++) {
      BoxVMRegs *lnew = & vm.local[i];
      lnew->min = lnew->max = 0;
      lnew->ptr = & reg0[i];
      vm.alc[i] = 0;
    }
  }

  vm.p = p;
  BoxVM_Proc_Get_Ptr_And_Length(vmx->vm, & vm.i_pos, NULL, p->code.proc_id);
  i_pos0 = i_pos = vm.i_pos;
  vm.flags.exit = vm.flags.error = 0;

  do {
    register int is_long;
    register VMByteX4 i_eye;

#if DEBUG_EXEC == 1
    fprintf(stderr, "module = "SInt", pos = "SInt" - reading instruction.\n",
            call_num, i*sizeof(VMByteX4));
#endif

    /* Leggo i dati fondamentali dell'istruzione: tipo e lunghezza. */
    ASM_GET_FORMAT(vm.i_pos, i_eye, is_long);
    vm.flags.is_long = is_long;
    if (is_long) {
      ASM_LONG_GET_HEADER(vm.i_pos, i_eye, vm.i_type, vm.i_len, vm.arg_type);
      vm.i_eye = i_eye;
      vm.flags.is_long = 1;

    } else {
      ASM_SHORT_GET_HEADER(vm.i_pos, i_eye, vm.i_type, vm.i_len, vm.arg_type);
      vm.i_eye = i_eye;
      vm.flags.is_long = 0;
    }

    if (vm.i_type >= BOX_NUM_OPS) {
      MSG_ERROR("Unknown VM instruction!");
      return Failed;
    }

    /* Trovo il descrittore di istruzione */
    vm.idesc = & exec_table[vm.i_type];

    /* Localizza in memoria gli argomenti */
    if (vm.idesc->numargs > 0)
      vm.idesc->get_args(& vm);

    /* Esegue l'istruzione */
    if (!vm.flags.error)
      vm.idesc->execute(& vm);

    /* Passo alla prossima istruzione.
     * vm.i_len can be modified by 'vm.idesc->execute(vmp)' when executing
     * instructions such as 'jmp' or 'jc'
     */
    vm.i_pos = (i_pos += vm.i_len);
#if DEBUG_EXEC == 1
    i += vm.i_len;
#endif

  } while (!vm.flags.exit);

  /* Fill the backtrace */
  if (vm.flags.error) {
    BoxVMTrace *trace = BoxArr_Push(& vmx->vm->backtrace, NULL);
    trace->call_num = call_num;
    trace->vm_pos = (void *) vm.i_pos - (void *) i_pos0;
  }

  /* Destroy the objects remaining in the roX registers */
  if (vm.alc[BOXTYPE_PTR] & 1) {
    BoxVMRegs *lregs = & vm.local[BOXTYPE_PTR];
    BoxPtr *ro = (BoxPtr *) lregs->ptr + lregs->min;
    int i, n = lregs->max - lregs->min + 1;
    /* ^^^ NOTE: lregs->min is negative! */

    for(i = 0; i < n; i++, ro++) {
      if (!BoxPtr_Is_Detached(ro))
        BoxVM_Unlink(vm.vm, ro);
    }
  }

  {
    /* Delete the registers allocated with the 'new*' instructions */
    register int i;
    for(i = 0; i < NUM_TYPES; i++)
      if ((vm.alc[i] & 1) != 0) {
        BoxVMRegs *lregs = & vm.local[i];
        BoxMem_Free(lregs->ptr + lregs->min*size_of_type[i]);
      }
  }

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
Task BoxVM_Disassemble(BoxVM *vmp, FILE *output, void *prog, UInt dim) {
  const BoxVMInstrDesc *exec_table = vmp->exec_table;
  register VMByteX4 *i_pos = (VMByteX4 *) prog;
  UInt pos, nargs;
  const char *iname;
  char iarg_buffers[VM_MAX_NUMARGS][64], /* max 64 characters per argument */
       *iarg[VM_MAX_NUMARGS];
  int i;

  for(i = 0; i < VM_MAX_NUMARGS; i++)
    iarg[i] = iarg_buffers[i];

  vmp->vmcur = & vm;
  vm.flags.exit = vm.flags.error = 0;
  for (pos = 0; pos < dim;) {
    VMByteX4 i_eye;
    int is_long;

    vm.i_pos = i_pos;
    vmp->dasm_pos = pos;

    /* Leggo i dati fondamentali dell'istruzione: tipo e lunghezza. */
    ASM_GET_FORMAT(vm.i_pos, i_eye, is_long);
    vm.flags.is_long = is_long;
    if (is_long) {
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

    if (vm.i_type < 0 || vm.i_type >= BOX_NUM_OPS) {
      iname = "???";
      vm.i_len = 1;
      nargs = 0;

    } else {
      /* Trovo il descrittore di istruzione */
      vm.idesc = & exec_table[vm.i_type];
      iname = vm.idesc->name;

      /* Localizza in memoria gli argomenti */
      nargs = vm.idesc->numargs;

      vm.idesc->disasm(vmp, iarg);
      if (vm.flags.exit)
        return Failed;
    }

    if (vm.flags.error) {
      fprintf(output, SUInt "\t"BoxVMByteX4_Fmt"x\tError!",
              (UInt) (pos * sizeof(VMByteX4)), *i_pos);

    } else {
      int i;
      VMByteX4 *i_pos2 = i_pos;

      /* Stampo l'istruzione e i suoi argomenti */
      fprintf(output, SUInt "\t", (UInt) (pos * sizeof(VMByteX4)));
      if (vmp->attr.hexcode)
        fprintf(output, BoxVMByteX4_Fmt"\t", *(i_pos2++));
      fprintf(output, "%s", iname);

      if (nargs > 0) {
        UInt n;

        assert(nargs <= VM_MAX_NUMARGS);

        fprintf(output, " %s", iarg[0]);
        for (n = 1; n < nargs; n++)
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
    if (vm.i_len < 1) return Failed;

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
void BoxVM_ASettings(BoxVM *vmp, int forcelong, int error, int inhibit) {
  VMProcTable *pt = & vmp->proc_table;
  vmp->attr.forcelong = forcelong;
  pt->target_proc->status.error = error;
  pt->target_proc->status.inhibit = inhibit;
}

/** Similar to BoxVM_Assemble, but takes a va_list argument as a replacement
 * for the extra arguments.
 */
void BoxVM_VA_Assemble(BoxVM *vmp, BoxOp instr, va_list ap) {
  const BoxVMInstrDesc *exec_table = vmp->exec_table, *idesc;
  VMProcTable *pt = & vmp->proc_table;
  int i, t;
  int is_short;
  struct {
    TypeID t;  /* Tipi degli argomenti */
    BoxOp c;   /* Categorie degli argomenti */
    void *ptr; /* Puntatori ai valori degli argomenti */
    Int   vi;  /* Destinazione dei valori...   */
    Real  vr;  /* ...immediati degli argomenti */
    Point vp;
  } arg[VM_MAX_NUMARGS];

  /* Esco subito se e' settato il flag di inibizione! */
  if (pt->target_proc->status.inhibit) return;

  if (instr < 0 || instr >= BOX_NUM_OPS) {
    MSG_ERROR("Unrecognised VM instruction while assembling (%s).",
              (instr < 0) ? "op < 0" : "op > BOX_NUM_OPS");
    return;
  }

  idesc = & exec_table[instr];

  assert(idesc->numargs <= VM_MAX_NUMARGS);

  /* Prendo argomento per argomento */
  t = 0; /* Indice di argomento */
  is_short = 1;
  for (i = 0; i < idesc->numargs; i++) {
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
      if (vi != 0 && vi != ~0x7fL)
        is_short = 0;
    }

    ++t;
  }

  assert(t == idesc->numargs);

  /* Cerco di capire se e' possibile scrivere l'istruzione in formato corto */
  if (vmp->attr.forcelong) is_short = 0;
  if (is_short == 1 && t <= 2) {
    /* L'istruzione va scritta in formato corto! */
    VMByteX4 buffer[1], *i_pos = buffer;
    register VMByteX4 i_eye;
    UInt atype;
    BoxArr *prog = & pt->target_proc->code;

    for (; t < 2; t++) {
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
void BoxVM_Assemble(BoxVM *vm, BoxOp instr, ...) {
  va_list ap;
  va_start(ap, instr);
  BoxVM_VA_Assemble(vm, instr, ap);
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
  BoxVM_Module_Global_Set(vm, TYPE_OBJ, (Int) 0, & data_segment_ptr);
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

void BoxVM_Backtrace_Clear(BoxVM *vm) {
  BoxArr_Clear(& vm->backtrace);
}

void BoxVM_Backtrace_Print(BoxVM *vm, FILE *stream) {
  BoxVMTrace *traces = BoxArr_First_Item_Ptr(& vm->backtrace);
  size_t n = BoxArr_Num_Items(& vm->backtrace);

  if (n == 0)
    fprintf(stream, "Empty traceback.\n");

  else {
    size_t i;
    fprintf(stream, "Traceback (innermost call comes last):\n");
    for(i = 0; i < n; i++) {
      BoxVMTrace *trace = & traces[n - 1 - i];
      BoxVMCallNum call_num = trace->call_num;
      BoxVMProcID proc_id = BoxVM_Proc_Get_ID(vm, call_num);
      if (proc_id != BOXVMPROCID_NONE) {
        size_t vm_pos = trace->vm_pos;
        char *desc = BoxVM_Proc_Get_Description(vm, call_num);
        BoxSrcPos *src_pos = BoxVM_Proc_Get_Source_Of(vm, proc_id, vm_pos);

        if (src_pos != NULL) {
          char *src_pos_str = BoxSrcPos_To_Str(src_pos);
          fprintf(stream, "  In %s at %s (VM address %ld).\n",
                  desc, src_pos_str, (long) vm_pos);
          BoxMem_Free(src_pos_str);

        } else
          fprintf(stream, "  In %s at %ld.\n", desc, (long) vm_pos);

        BoxMem_Free(desc);

      } else {
        fprintf(stream, "  In C code (ERROR?).\n");
      }
    }
  }

  if (vm->fail_msg != NULL)
    fprintf(stream, "Failure: %s\n", vm->fail_msg);
}