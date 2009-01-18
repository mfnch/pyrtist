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

/* Questo file contiene la funzioni che eseguono le istruzioni della macchina
 * virtuale (spesso denotata con VM).
 * Questo file e' incluso direttamente dal file "virtmach.c"
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "messages.h"
#include "virtmach.h"
#include "vmalloc.h"

static void VM__Exec_Ret(VMProgram *vmp) {vmp->vmcur->flags.exit = 1;}

static void VM__Exec_Call_I(VMProgram *vmp) {
  VMStatus *vm = vmp->vmcur;
  if IS_SUCCESSFUL( VM_Module_Execute(vmp, *((Int *) vm->arg1)) ) return;
  vm->flags.error = vm->flags.exit = 1;
}

static void VM__Exec_Line_I(VMProgram *vmp) {
  VMStatus *vm = vmp->vmcur;
  vm->line = *((Int *) vm->arg1);
  Msg_Line_Set(vm->line);
}

#define VM__NEW(name, TYPE_ID, Type)                                   \
static void VM__Exec_ ## name ## _II(VMProgram *vmp) {                 \
  VMStatus *vmcur = vmp->vmcur;                                        \
  register Type *ptr;                                                  \
  register Int numvar = *((Int *) vmcur->arg1),                        \
    numreg = *((Int *) vmcur->arg2), numtot = numvar + numreg + 1;     \
  if ( (vmcur->alc[TYPE_ID] & 1) != 0 ) goto err;                      \
  vmcur->alc[TYPE_ID] |= 1;                                            \
  vmcur->lmin[TYPE_ID] = -numvar; vmcur->lmax[TYPE_ID] = numreg;       \
  ptr = (Type *) calloc(numtot, sizeof(Type));                         \
  vmcur->local[TYPE_ID] = ptr + numvar;                                \
  if ( (ptr != NULL) && (numvar >= 0) && (numtot > numvar) ) return;   \
  MSG_FATAL(#name ": Cannot allocate the register memory region.");    \
  vmcur->flags.error = vmcur->flags.exit = 1; return;                  \
err:  MSG_FATAL(#name ": Registers have already been allocated!");     \
  vmcur->flags.error = vmcur->flags.exit = 1;                          \
}

VM__NEW(NewC, TYPE_CHAR,  Char)
VM__NEW(NewI, TYPE_INT,  Int)
VM__NEW(NewR, TYPE_REAL,  Real)
VM__NEW(NewP, TYPE_POINT, Point)
VM__NEW(NewO, TYPE_OBJ,   Obj)

static void VM__Exec_Mov_CC(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Char *) vmcur->arg1) = *((Char *) vmcur->arg2);
}
static void VM__Exec_Mov_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) = *((Int *) vmcur->arg2);
}
static void VM__Exec_Mov_RR(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->arg1) = *((Real *) vmcur->arg2);
}
static void VM__Exec_Mov_PP(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Point *) vmcur->arg1) = *((Point *) vmcur->arg2);
}
static void VM__Exec_Mov_OO(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Obj *) vmcur->arg1) = *((Obj *) vmcur->arg2);
}

static void VM__Exec_BNot_I(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) = ~ *((Int *) vmcur->arg1);
}
static void VM__Exec_BAnd_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) &= *((Int *) vmcur->arg2);
}
static void VM__Exec_BXor_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) ^= *((Int *) vmcur->arg2);
}
static void VM__Exec_BOr_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) |= *((Int *) vmcur->arg2);
}

static void VM__Exec_Shl_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) <<= *((Int *) vmcur->arg2);
}
static void VM__Exec_Shr_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) >>= *((Int *) vmcur->arg2);
}

static void VM__Exec_Inc_I(VMProgram *vmp) { ++ *((Int *) vmp->vmcur->arg1); }
static void VM__Exec_Inc_R(VMProgram *vmp) { ++ *((Real *) vmp->vmcur->arg1); }
static void VM__Exec_Dec_I(VMProgram *vmp) { -- *((Int *) vmp->vmcur->arg1); }
static void VM__Exec_Dec_R(VMProgram *vmp) { -- *((Real *) vmp->vmcur->arg1); }

static void VM__Exec_Pow_II(VMProgram *vmp) { /* bad implementation!!! */
  VMStatus *vmcur = vmp->vmcur;
  Int i, r = 1, b = *((Int *) vmcur->arg1), p = *((Int *) vmcur->arg2);
  for (i = 0; i < p; i++) r *= b;
  *((Int *) vmcur->arg1) = r;
}
static void VM__Exec_Pow_RR(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->arg1) =
   pow(*((Real *) vmcur->arg1), *((Real *) vmcur->arg2));
}

static void VM__Exec_Add_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) += *((Int *) vmcur->arg2);
}
static void VM__Exec_Add_RR(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->arg1) += *((Real *) vmcur->arg2);
}
static void VM__Exec_Add_PP(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  ((Point *) vmcur->arg1)->x += ((Point *) vmcur->arg2)->x;
  ((Point *) vmcur->arg1)->y += ((Point *) vmcur->arg2)->y;
}

static void VM__Exec_Sub_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) -= *((Int *) vmcur->arg2);
}
static void VM__Exec_Sub_RR(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->arg1) -= *((Real *) vmcur->arg2);
}
static void VM__Exec_Sub_PP(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  ((Point *) vmcur->arg1)->x -= ((Point *) vmcur->arg2)->x;
  ((Point *) vmcur->arg1)->y -= ((Point *) vmcur->arg2)->y;
}

static void VM__Exec_Mul_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) *= *((Int *) vmcur->arg2);
}
static void VM__Exec_Mul_RR(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->arg1) *= *((Real *) vmcur->arg2);
}

static void VM__Exec_Div_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) /= *((Int *) vmcur->arg2);
}
static void VM__Exec_Div_RR(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->arg1) /= *((Real *) vmcur->arg2);
}
static void VM__Exec_Rem_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) %= *((Int *) vmcur->arg2);
}

static void VM__Exec_Neg_I(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) = -*((Int *) vmcur->arg1);
}
static void VM__Exec_Neg_R(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->arg1) = -*((Real *) vmcur->arg1);
}
static void VM__Exec_Neg_P(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  ((Point *) vmcur->arg1)->x = -((Point *) vmcur->arg1)->x;
  ((Point *) vmcur->arg1)->y = -((Point *) vmcur->arg1)->y;
}

static void VM__Exec_PMulR_PR(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  Real r = *((Real *) vmcur->local[TYPE_REAL]);
  ((Point *) vmcur->arg1)->x *= r;
  ((Point *) vmcur->arg1)->y *= r;
}
static void VM__Exec_PDivR_PR(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  Real r = *((Real *) vmcur->local[TYPE_REAL]);
  ((Point *) vmcur->arg1)->x /= r;
  ((Point *) vmcur->arg1)->y /= r;
}

static void VM__Exec_Real_C(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->local[TYPE_REAL]) = (Real) *((Char *) vmcur->arg1);
}
static void VM__Exec_Real_I(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->local[TYPE_REAL]) = (Real) *((Int *) vmcur->arg1);
}
static void VM__Exec_Int_R(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT]) = (Int) *((Real *) vmcur->arg1);
}
static void VM__Exec_Point_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  ((Point *) vmcur->local[TYPE_POINT])->x = (Real) *((Int *) vmcur->arg1);
  ((Point *) vmcur->local[TYPE_POINT])->y = (Real) *((Int *) vmcur->arg2);
}
static void VM__Exec_Point_RR(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  ((Point *) vmcur->local[TYPE_POINT])->x = *((Real *) vmcur->arg1);
  ((Point *) vmcur->local[TYPE_POINT])->y = *((Real *) vmcur->arg2);
}
static void VM__Exec_ProjX_P(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->local[TYPE_REAL]) = ((Point *) vmcur->arg1)->x;
}
static void VM__Exec_ProjY_P(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->local[TYPE_REAL]) = ((Point *) vmcur->arg1)->y;
}

static void VM__Exec_PPtrX_P(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  Obj *obj = (Obj *) vmcur->local[TYPE_OBJ];
  obj->block = (void *) NULL;
  obj->ptr = & (((Point *) vmcur->arg1)->x);
}

static void VM__Exec_PPtrY_P(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  Obj *obj = (Obj *) vmcur->local[TYPE_OBJ];
  obj->block = (void *) NULL;
  obj->ptr = & (((Point *) vmcur->arg1)->y);
}

static void VM__Exec_Eq_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) =
   *((Int *) vmcur->arg1) == *((Int *) vmcur->arg2);
}
static void VM__Exec_Eq_RR(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT]) =
   *((Real *) vmcur->arg1) == *((Real *) vmcur->arg2);
}
static void VM__Exec_Eq_PP(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT]) =
     ( ((Point *) vmcur->arg1)->x == ((Point *) vmcur->arg2)->x )
  && ( ((Point *) vmcur->arg1)->y == ((Point *) vmcur->arg2)->y );
}
static void VM__Exec_Ne_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) =
   *((Int *) vmcur->arg1) != *((Int *) vmcur->arg2);
}
static void VM__Exec_Ne_RR(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT]) =
   *((Real *) vmcur->arg1) != *((Real *) vmcur->arg2);
}
static void VM__Exec_Ne_PP(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT]) =
     ( ((Point *) vmcur->arg1)->x != ((Point *) vmcur->arg2)->x )
  || ( ((Point *) vmcur->arg1)->y != ((Point *) vmcur->arg2)->y );
}

static void VM__Exec_Lt_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) =
   *((Int *) vmcur->arg1) < *((Int *) vmcur->arg2);
}
static void VM__Exec_Lt_RR(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT]) =
   *((Real *) vmcur->arg1) < *((Real *) vmcur->arg2);
}
static void VM__Exec_Le_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) =
   *((Int *) vmcur->arg1) <= *((Int *) vmcur->arg2);
}
static void VM__Exec_Le_RR(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT]) =
   *((Real *) vmcur->arg1) <= *((Real *) vmcur->arg2);
}
static void VM__Exec_Gt_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) =
   *((Int *) vmcur->arg1) > *((Int *) vmcur->arg2);
}
static void VM__Exec_Gt_RR(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT]) =
   *((Real *) vmcur->arg1) > *((Real *) vmcur->arg2);
}
static void VM__Exec_Ge_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) =
   *((Int *) vmcur->arg1) >= *((Int *) vmcur->arg2);
}
static void VM__Exec_Ge_RR(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT]) =
   *((Real *) vmcur->arg1) >= *((Real *) vmcur->arg2);
}

static void VM__Exec_LNot_I(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) = ! *((Int *) vmcur->arg1);
}
static void VM__Exec_LAnd_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) =
   *((Int *) vmcur->arg1) && *((Int *) vmcur->arg2);
}
static void VM__Exec_LOr_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) =
   *((Int *) vmcur->arg1) || *((Int *) vmcur->arg2);
}

static void VM__Exec_Malloc_II(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  Int size = *((Int *) vmcur->arg1),
      type = *((Int *) vmcur->arg2);
  Obj *obj = (Obj *) vmcur->local[TYPE_OBJ];
  VM_Alloc(obj, size, type);
  if (obj->block != (void *) NULL) return;
  MSG_FATAL("VM_Exec_Malloc_II: memory request failed!");
}

static void VM__Exec_Mln_O(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  VM_Link((Obj *) vmcur->arg1);
}

static void VM__Exec_MUnln_O(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  VM_Unlink(vmp, (Obj *) vmcur->arg1);
}

static void VM__Exec_MCopy_OO(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  (void) memcpy(((Obj *) vmcur->arg1)->ptr,         /* destination */
                ((Obj *) vmcur->arg2)->ptr,         /* source */
                *((Int *) vmcur->local[TYPE_INT])); /* size */
}

static void VM__Exec_Lea(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  Obj *obj = (Obj *) vmcur->local[TYPE_OBJ];
  obj->block = (void *) NULL;
  obj->ptr = vmcur->arg1;
}

static void VM__Exec_Lea_OO(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  Obj *obj = (Obj *) vmcur->arg1;
  obj->block = (void *) NULL;
  obj->ptr = vmcur->arg2;
}

static void VM__Exec_Push_O(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  BoxArr_Push(& vmp->stack, vmcur->arg1);
}

static void VM__Exec_Pop_O(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  BoxArr_Pop(& vmp->stack, vmcur->arg1);
}

static void VM__Exec_Jmp_I(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  vmcur->i_len = *((Int *) vmcur->arg1);
}

static void VM__Exec_Jc_I(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  if (*((Int *) vmcur->local[TYPE_INT]))
    vmcur->i_len = *((Int *) vmcur->arg1);
}

static void VM__Exec_Add_O(VMProgram *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  ((Obj *) vmcur->arg1)->ptr += *((Int *) vmcur->local[TYPE_INT]);
}

/******************************************************************************
 * La seguente tabella descrive le istruzioni e specifica quali funzioni      *
 * debbano essere chiamate per localizzare la posizione in memoria degli      *
 * argomenti e quali per eseguire l'azione associata all'istruzione stessa.   *
 ******************************************************************************/
VMInstrDesc vm_instr_desc_table[] = {
  { },
  { "line", 1, TYPE_INT, VM__GLPI,     VM__Exec_Line_I, VM__D_GLPI_GLPI },  /* line imm_i         */
  { "call", 1, TYPE_INT, VM__GLPI,     VM__Exec_Call_I, VM__D_CALL      },  /* call reg_i         */
  { "call", 1, TYPE_INT, VM__Imm,      VM__Exec_Call_I, VM__D_CALL      },  /* call imm_i         */
  { "newc", 2, TYPE_INT, VM__GLP_GLPI, VM__Exec_NewC_II,VM__D_GLPI_GLPI },  /* newc imm_i, imm_i  */
  { "newi", 2, TYPE_INT, VM__GLP_GLPI, VM__Exec_NewI_II,VM__D_GLPI_GLPI },  /* newi imm_i, imm_i  */
  { "newr", 2, TYPE_INT, VM__GLP_GLPI, VM__Exec_NewR_II,VM__D_GLPI_GLPI },  /* newr imm_i, imm_i  */
  { "newp", 2, TYPE_INT, VM__GLP_GLPI, VM__Exec_NewP_II,VM__D_GLPI_GLPI },  /* newp imm_i, imm_i  */
  { "newo", 2, TYPE_INT, VM__GLP_GLPI, VM__Exec_NewO_II,VM__D_GLPI_GLPI },  /* newo imm_i, imm_i  */
  { "mov",  2, TYPE_CHAR, VM__GLP_Imm,  VM__Exec_Mov_CC, VM__D_GLPI_Imm  }, /* mov reg_c, imm_c   */
  { "mov",  2, TYPE_INT, VM__GLP_Imm,  VM__Exec_Mov_II, VM__D_GLPI_Imm  },  /* mov reg_i, imm_i   */
  { "mov",  2, TYPE_REAL, VM__GLP_Imm,  VM__Exec_Mov_RR, VM__D_GLPI_Imm  }, /* mov reg_r, imm_r   */
  { "mov",  2, TYPE_POINT,VM__GLP_Imm,  VM__Exec_Mov_PP, VM__D_GLPI_Imm  }, /* mov reg_p, imm_p   */
  { "mov",  2, TYPE_CHAR, VM__GLP_GLPI, VM__Exec_Mov_CC, VM__D_GLPI_GLPI }, /* mov reg_c, reg_c   */
  { "mov",  2, TYPE_INT, VM__GLP_GLPI, VM__Exec_Mov_II, VM__D_GLPI_GLPI },  /* mov reg_i, reg_i   */
  { "mov",  2, TYPE_REAL, VM__GLP_GLPI, VM__Exec_Mov_RR, VM__D_GLPI_GLPI }, /* mov reg_r, reg_r   */
  { "mov",  2, TYPE_POINT,VM__GLP_GLPI, VM__Exec_Mov_PP, VM__D_GLPI_GLPI }, /* mov reg_p, reg_p   */
  { "mov",  2, TYPE_OBJ,  VM__GLP_GLPI, VM__Exec_Mov_OO, VM__D_GLPI_GLPI }, /* mov reg_o, reg_o   */
  { "bnot", 1, TYPE_INT, VM__GLPI,     VM__Exec_BNot_I, VM__D_GLPI_GLPI },  /* bnot reg_i         */
  { "band", 2, TYPE_INT, VM__GLP_GLPI, VM__Exec_BAnd_II,VM__D_GLPI_GLPI },  /* band reg_i, reg_i  */
  { "bxor", 2, TYPE_INT, VM__GLP_GLPI, VM__Exec_BXor_II,VM__D_GLPI_GLPI },  /* bxor reg_i, reg_i  */
  { "bor",  2, TYPE_INT, VM__GLP_GLPI, VM__Exec_BOr_II, VM__D_GLPI_GLPI },  /* bor reg_i, reg_i   */
  { "shl",  2, TYPE_INT, VM__GLP_GLPI, VM__Exec_Shl_II, VM__D_GLPI_GLPI },  /* shl reg_i, reg_i   */
  { "shr",  2, TYPE_INT, VM__GLP_GLPI, VM__Exec_Shr_II, VM__D_GLPI_GLPI },  /* shr reg_i, reg_i   */
  { "inc",  1, TYPE_INT, VM__GLPI,     VM__Exec_Inc_I,  VM__D_GLPI_GLPI },  /* inc reg_i          */
  { "inc",  1, TYPE_REAL, VM__GLPI,     VM__Exec_Inc_R,  VM__D_GLPI_GLPI }, /* inc reg_r          */
  { "dec",  1, TYPE_INT, VM__GLPI,     VM__Exec_Dec_I,  VM__D_GLPI_GLPI },  /* dec reg_i          */
  { "dec",  1, TYPE_REAL, VM__GLPI,     VM__Exec_Dec_R,  VM__D_GLPI_GLPI }, /* dec reg_r          */
  { "pow",  2, TYPE_INT, VM__GLP_GLPI, VM__Exec_Pow_II, VM__D_GLPI_GLPI },  /* pow reg_i, reg_i   */
  { "pow",  2, TYPE_REAL, VM__GLP_GLPI, VM__Exec_Pow_RR, VM__D_GLPI_GLPI }, /* pow reg_r, reg_r   */
  { "add",  2, TYPE_INT, VM__GLP_GLPI, VM__Exec_Add_II, VM__D_GLPI_GLPI },  /* add reg_i, reg_i   */
  { "add",  2, TYPE_REAL, VM__GLP_GLPI, VM__Exec_Add_RR, VM__D_GLPI_GLPI }, /* add reg_r, reg_r   */
  { "add",  2, TYPE_POINT,VM__GLP_GLPI, VM__Exec_Add_PP, VM__D_GLPI_GLPI }, /* add reg_p, reg_p   */
  { "sub",  2, TYPE_INT, VM__GLP_GLPI, VM__Exec_Sub_II, VM__D_GLPI_GLPI },  /* sub reg_i, reg_i   */
  { "sub",  2, TYPE_REAL, VM__GLP_GLPI, VM__Exec_Sub_RR, VM__D_GLPI_GLPI }, /* sub reg_r, reg_r   */
  { "sub",  2, TYPE_POINT,VM__GLP_GLPI, VM__Exec_Sub_PP, VM__D_GLPI_GLPI }, /* sub reg_p, reg_p   */
  { "mul",  2, TYPE_INT, VM__GLP_GLPI, VM__Exec_Mul_II, VM__D_GLPI_GLPI },  /* mul reg_i, reg_i   */
  { "mul",  2, TYPE_REAL, VM__GLP_GLPI, VM__Exec_Mul_RR, VM__D_GLPI_GLPI }, /* mul reg_r, reg_r   */
  { "div",  2, TYPE_INT, VM__GLP_GLPI, VM__Exec_Div_II, VM__D_GLPI_GLPI },  /* div reg_i, reg_i   */
  { "div",  2, TYPE_REAL, VM__GLP_GLPI, VM__Exec_Div_RR, VM__D_GLPI_GLPI }, /* div reg_r, reg_r   */
  { "rem",  2, TYPE_INT, VM__GLP_GLPI, VM__Exec_Rem_II, VM__D_GLPI_GLPI },  /* rem reg_i, reg_i   */
  { "neg",  1, TYPE_INT, VM__GLPI,     VM__Exec_Neg_I,  VM__D_GLPI_GLPI },  /* neg reg_i          */
  { "neg",  1, TYPE_REAL, VM__GLPI,     VM__Exec_Neg_R,  VM__D_GLPI_GLPI }, /* neg reg_r          */
  { "neg",  1, TYPE_POINT,VM__GLPI,     VM__Exec_Neg_P,  VM__D_GLPI_GLPI }, /* neg reg_p          */
  { "pmulr",1, TYPE_POINT,VM__GLPI,   VM__Exec_PMulR_PR, VM__D_GLPI_GLPI }, /* pmulr reg_p        */
  { "pdivr",1, TYPE_POINT,VM__GLPI,   VM__Exec_PDivR_PR, VM__D_GLPI_GLPI }, /* pdivr reg_p        */
  { "eq?",  2, TYPE_INT, VM__GLP_GLPI,  VM__Exec_Eq_II, VM__D_GLPI_GLPI },  /* eq? reg_i, reg_i   */
  { "eq?",  2, TYPE_REAL, VM__GLP_GLPI,  VM__Exec_Eq_RR, VM__D_GLPI_GLPI }, /* eq? reg_r, reg_r   */
  { "eq?",  2, TYPE_POINT,VM__GLP_GLPI,  VM__Exec_Eq_PP, VM__D_GLPI_GLPI }, /* eq? reg_p, reg_p   */
  { "ne?",  2, TYPE_INT, VM__GLP_GLPI,  VM__Exec_Ne_II, VM__D_GLPI_GLPI },  /* ne? reg_i, reg_i   */
  { "ne?",  2, TYPE_REAL, VM__GLP_GLPI,  VM__Exec_Ne_RR, VM__D_GLPI_GLPI }, /* ne? reg_r, reg_r   */
  { "ne?",  2, TYPE_POINT,VM__GLP_GLPI,  VM__Exec_Ne_PP, VM__D_GLPI_GLPI }, /* ne? reg_p, reg_p   */
  { "lt?",  2, TYPE_INT, VM__GLP_GLPI,  VM__Exec_Lt_II, VM__D_GLPI_GLPI },  /* lt? reg_i, reg_i   */
  { "lt?",  2, TYPE_REAL, VM__GLP_GLPI,  VM__Exec_Lt_RR, VM__D_GLPI_GLPI }, /* lt? reg_r, reg_r   */
  { "le?",  2, TYPE_INT, VM__GLP_GLPI,  VM__Exec_Le_II, VM__D_GLPI_GLPI },  /* le? reg_i, reg_i   */
  { "le?",  2, TYPE_REAL, VM__GLP_GLPI,  VM__Exec_Le_RR, VM__D_GLPI_GLPI }, /* le? reg_r, reg_r   */
  { "gt?",  2, TYPE_INT, VM__GLP_GLPI,  VM__Exec_Gt_II, VM__D_GLPI_GLPI },  /* gt? reg_i, reg_i   */
  { "gt?",  2, TYPE_REAL, VM__GLP_GLPI,  VM__Exec_Gt_RR, VM__D_GLPI_GLPI }, /* gt? reg_r, reg_r   */
  { "ge?",  2, TYPE_INT, VM__GLP_GLPI,  VM__Exec_Ge_II, VM__D_GLPI_GLPI },  /* ge? reg_i, reg_i   */
  { "ge?",  2, TYPE_REAL, VM__GLP_GLPI,  VM__Exec_Ge_RR, VM__D_GLPI_GLPI }, /* ge? reg_r, reg_r   */
  { "lnot", 1, TYPE_INT, VM__GLP_GLPI, VM__Exec_LNot_I, VM__D_GLPI_GLPI },  /* lnot reg_i         */
  { "land", 2, TYPE_INT, VM__GLP_GLPI,VM__Exec_LAnd_II, VM__D_GLPI_GLPI },  /* land reg_i, reg_i  */
  { "lor",  2, TYPE_INT, VM__GLP_GLPI, VM__Exec_LOr_II, VM__D_GLPI_GLPI },  /* lor  reg_i, reg_i  */
  { "real", 1, TYPE_CHAR, VM__GLPI,     VM__Exec_Real_C, VM__D_GLPI_GLPI }, /* real reg_c         */
  { "real", 1, TYPE_INT, VM__GLPI,     VM__Exec_Real_I, VM__D_GLPI_GLPI },  /* real reg_i         */
  { "intg", 1, TYPE_REAL, VM__GLPI,     VM__Exec_Int_R, VM__D_GLPI_GLPI },  /* intg reg_r         */
  { "point",2, TYPE_INT, VM__GLP_GLPI,VM__Exec_Point_II,VM__D_GLPI_GLPI },  /* point reg_i, reg_i */
  { "point",2, TYPE_REAL, VM__GLP_GLPI,VM__Exec_Point_RR,VM__D_GLPI_GLPI }, /* point reg_r, reg_r */
  { "projx",1, TYPE_POINT,VM__GLPI,    VM__Exec_ProjX_P, VM__D_GLPI_GLPI }, /* projx reg_p        */
  { "projy",1, TYPE_POINT,VM__GLPI,    VM__Exec_ProjY_P, VM__D_GLPI_GLPI }, /* projy reg_p        */
  { "pptrx",1, TYPE_POINT,VM__GLPI,    VM__Exec_PPtrX_P, VM__D_GLPI_GLPI }, /* pptrx reg_p        */
  { "pptry",1, TYPE_POINT,VM__GLPI,    VM__Exec_PPtrY_P, VM__D_GLPI_GLPI }, /* pptry reg_p        */
  { "ret",  0, TYPE_NONE, NULL,            VM__Exec_Ret, VM__D_GLPI_GLPI }, /* ret                */
  {"malloc",2, TYPE_INT,VM__GLP_GLPI,VM__Exec_Malloc_II, VM__D_GLPI_GLPI }, /* malloc reg_i       */
  {   "mln",1, TYPE_OBJ,  VM__GLPI,      VM__Exec_Mln_O, VM__D_GLPI_GLPI }, /* mln reg_o        */
  { "munln",1, TYPE_OBJ,  VM__GLPI,    VM__Exec_MUnln_O, VM__D_GLPI_GLPI }, /* munln reg_o        */
  { "mcopy",2, TYPE_OBJ,  VM__GLP_GLPI,VM__Exec_MCopy_OO,VM__D_GLPI_GLPI }, /* mcopy reg_o, reg_o */
  {  "lea", 1, TYPE_CHAR, VM__GLPI,        VM__Exec_Lea, VM__D_GLPI_GLPI }, /* lea c[ro0+...]     */
  {  "lea", 1, TYPE_INT, VM__GLPI,        VM__Exec_Lea, VM__D_GLPI_GLPI },  /* lea i[ro0+...]     */
  {  "lea", 1, TYPE_REAL, VM__GLPI,        VM__Exec_Lea, VM__D_GLPI_GLPI }, /* lea r[ro0+...]     */
  {  "lea", 1, TYPE_POINT,VM__GLPI,        VM__Exec_Lea, VM__D_GLPI_GLPI }, /* lea p[ro0+...]     */
  {  "lea", 2, TYPE_OBJ,  VM__GLP_GLPI, VM__Exec_Lea_OO, VM__D_GLPI_GLPI }, /* lea reg_o, o[ro0+...] */
  { "push", 1, TYPE_OBJ, VM__GLPI,     VM__Exec_Push_O, VM__D_GLPI_GLPI }, /* push reg_o         */
  {  "pop", 1, TYPE_OBJ, VM__GLPI,      VM__Exec_Pop_O, VM__D_GLPI_GLPI }, /* pop reg_o          */
  {  "jmp", 1, TYPE_INT, VM__GLPI,      VM__Exec_Jmp_I,       VM__D_JMP  }, /* jmp reg_i          */
  {   "jc", 1, TYPE_INT, VM__GLPI,       VM__Exec_Jc_I,       VM__D_JMP  }, /* jc  reg_i          */
  {  "add", 1, TYPE_OBJ, VM__GLPI,      VM__Exec_Add_O,  VM__D_GLPI_GLPI }  /* add reg_o          */
};