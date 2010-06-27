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
#include <assert.h>

#include "messages.h"
#include "virtmach.h"
#include "vmalloc.h"
#include "bltinarray.h"
#include "str.h"

static void VM__Exec_Ret(BoxVM *vmp) {vmp->vmcur->flags.exit = 1;}

static void VM__Exec_Call_I(BoxVM *vmp) {
  VMStatus *vm = vmp->vmcur;
  if IS_SUCCESSFUL( BoxVM_Module_Execute(vmp, *((Int *) vm->arg1)) )
    return;
  vm->flags.error = vm->flags.exit = 1;
}

#define VM__NEW(name, TYPE_ID, Type)                                   \
static void VM__Exec_ ## name ## _II(BoxVM *vmp) {                     \
  VMStatus *vmcur = vmp->vmcur;                                        \
  BoxVMRegs *regs = & vmcur->local[TYPE_ID];                           \
  register Type *ptr;                                                  \
  register Int numvar = *((Int *) vmcur->arg1),                        \
    numreg = *((Int *) vmcur->arg2), numtot = numvar + numreg + 1;     \
  if ((vmcur->alc[TYPE_ID] & 1) != 0) goto err;                        \
  vmcur->alc[TYPE_ID] |= 1;                                            \
  regs->min = -numvar; regs->max = numreg;                             \
  ptr = (Type *) calloc(numtot, sizeof(Type));                         \
  regs->ptr = (ptr + numvar);                                          \
  if (ptr != NULL && numvar >= 0 && numtot > numvar) return;           \
  MSG_FATAL(#name ": Cannot allocate the register memory region.");    \
  vmcur->flags.error = vmcur->flags.exit = 1; return;                  \
err:  MSG_FATAL(#name ": Registers have already been allocated!");     \
  vmcur->flags.error = vmcur->flags.exit = 1;                          \
}

VM__NEW(NewC, BOXTYPE_CHAR,  BoxChar)
VM__NEW(NewI, BOXTYPE_INT,   BoxInt)
VM__NEW(NewR, BOXTYPE_REAL,  BoxReal)
VM__NEW(NewP, BOXTYPE_POINT, BoxPoint)
VM__NEW(NewO, BOXTYPE_PTR,   BoxPtr)

static void VM__Exec_Mov_CC(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Char *) vmcur->arg1) = *((Char *) vmcur->arg2);
}
static void VM__Exec_Mov_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) = *((Int *) vmcur->arg2);
}
static void VM__Exec_Mov_RR(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->arg1) = *((Real *) vmcur->arg2);
}
static void VM__Exec_Mov_PP(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Point *) vmcur->arg1) = *((Point *) vmcur->arg2);
}
static void VM__Exec_Mov_OO(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Obj *) vmcur->arg1) = *((Obj *) vmcur->arg2);
}

static void VM__Exec_BNot_I(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) = ~ *((Int *) vmcur->arg1);
}
static void VM__Exec_BAnd_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) &= *((Int *) vmcur->arg2);
}
static void VM__Exec_BXor_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) ^= *((Int *) vmcur->arg2);
}
static void VM__Exec_BOr_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) |= *((Int *) vmcur->arg2);
}

static void VM__Exec_Shl_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) <<= *((Int *) vmcur->arg2);
}
static void VM__Exec_Shr_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) >>= *((Int *) vmcur->arg2);
}

static void VM__Exec_Inc_I(BoxVM *vmp) { ++ *((Int *) vmp->vmcur->arg1); }
static void VM__Exec_Inc_R(BoxVM *vmp) { ++ *((Real *) vmp->vmcur->arg1); }
static void VM__Exec_Dec_I(BoxVM *vmp) { -- *((Int *) vmp->vmcur->arg1); }
static void VM__Exec_Dec_R(BoxVM *vmp) { -- *((Real *) vmp->vmcur->arg1); }

static void VM__Exec_Pow_II(BoxVM *vmp) { /* bad implementation!!! */
  VMStatus *vmcur = vmp->vmcur;
  Int i, r = 1, b = *((Int *) vmcur->arg1), p = *((Int *) vmcur->arg2);
  for (i = 0; i < p; i++) r *= b;
  *((Int *) vmcur->arg1) = r;
}
static void VM__Exec_Pow_RR(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->arg1) =
   pow(*((Real *) vmcur->arg1), *((Real *) vmcur->arg2));
}

static void VM__Exec_Add_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) += *((Int *) vmcur->arg2);
}
static void VM__Exec_Add_RR(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->arg1) += *((Real *) vmcur->arg2);
}
static void VM__Exec_Add_PP(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  ((Point *) vmcur->arg1)->x += ((Point *) vmcur->arg2)->x;
  ((Point *) vmcur->arg1)->y += ((Point *) vmcur->arg2)->y;
}

static void VM__Exec_Sub_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) -= *((Int *) vmcur->arg2);
}
static void VM__Exec_Sub_RR(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->arg1) -= *((Real *) vmcur->arg2);
}
static void VM__Exec_Sub_PP(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  ((Point *) vmcur->arg1)->x -= ((Point *) vmcur->arg2)->x;
  ((Point *) vmcur->arg1)->y -= ((Point *) vmcur->arg2)->y;
}

static void VM__Exec_Mul_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) *= *((Int *) vmcur->arg2);
}
static void VM__Exec_Mul_RR(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->arg1) *= *((Real *) vmcur->arg2);
}

static void VM__Exec_Div_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) /= *((Int *) vmcur->arg2);
}
static void VM__Exec_Div_RR(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->arg1) /= *((Real *) vmcur->arg2);
}
static void VM__Exec_Rem_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) %= *((Int *) vmcur->arg2);
}

static void VM__Exec_Neg_I(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) = -*((Int *) vmcur->arg1);
}
static void VM__Exec_Neg_R(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->arg1) = -*((Real *) vmcur->arg1);
}
static void VM__Exec_Neg_P(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  ((Point *) vmcur->arg1)->x = -((Point *) vmcur->arg1)->x;
  ((Point *) vmcur->arg1)->y = -((Point *) vmcur->arg1)->y;
}

static void VM__Exec_PMulR_PR(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  Real r = *((Real *) vmcur->local[TYPE_REAL].ptr);
  ((Point *) vmcur->arg1)->x *= r;
  ((Point *) vmcur->arg1)->y *= r;
}
static void VM__Exec_PDivR_PR(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  Real r = *((Real *) vmcur->local[TYPE_REAL].ptr);
  ((Point *) vmcur->arg1)->x /= r;
  ((Point *) vmcur->arg1)->y /= r;
}

static void VM__Exec_Real_C(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->local[TYPE_REAL].ptr) = (Real) *((Char *) vmcur->arg1);
}
static void VM__Exec_Real_I(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->local[TYPE_REAL].ptr) = (Real) *((Int *) vmcur->arg1);
}
static void VM__Exec_Int_R(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT].ptr) = (Int) *((Real *) vmcur->arg1);
}
static void VM__Exec_Int_C(BoxVM *vm) {
  VMStatus *vmcur = vm->vmcur;
  *((Int *) vmcur->local[TYPE_INT].ptr) = (Int) *((Char *) vmcur->arg1);
}
static void VM__Exec_Point_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  ((Point *) vmcur->local[TYPE_POINT].ptr)->x = (Real) *((Int *) vmcur->arg1);
  ((Point *) vmcur->local[TYPE_POINT].ptr)->y = (Real) *((Int *) vmcur->arg2);
}
static void VM__Exec_Point_RR(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  ((Point *) vmcur->local[TYPE_POINT].ptr)->x = *((Real *) vmcur->arg1);
  ((Point *) vmcur->local[TYPE_POINT].ptr)->y = *((Real *) vmcur->arg2);
}
static void VM__Exec_ProjX_P(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->local[TYPE_REAL].ptr) = ((Point *) vmcur->arg1)->x;
}
static void VM__Exec_ProjY_P(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Real *) vmcur->local[TYPE_REAL].ptr) = ((Point *) vmcur->arg1)->y;
}

static void VM__Exec_PPtrX_P(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  Obj *obj = (Obj *) vmcur->local[TYPE_OBJ].ptr;
  obj->block = (void *) NULL;
  obj->ptr = & (((Point *) vmcur->arg1)->x);
}

static void VM__Exec_PPtrY_P(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  Obj *obj = (Obj *) vmcur->local[TYPE_OBJ].ptr;
  obj->block = (void *) NULL;
  obj->ptr = & (((Point *) vmcur->arg1)->y);
}

static void VM__Exec_Eq_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) =
   *((Int *) vmcur->arg1) == *((Int *) vmcur->arg2);
}
static void VM__Exec_Eq_RR(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT].ptr) =
   *((Real *) vmcur->arg1) == *((Real *) vmcur->arg2);
}
static void VM__Exec_Eq_PP(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT].ptr) =
     ( ((Point *) vmcur->arg1)->x == ((Point *) vmcur->arg2)->x )
  && ( ((Point *) vmcur->arg1)->y == ((Point *) vmcur->arg2)->y );
}
static void VM__Exec_Eq_OO(BoxVM *vm) {
  VMStatus *vmcur = vm->vmcur;
  *((Int *) vmcur->local[TYPE_INT].ptr) =
     (((Obj *) vmcur->arg1)->ptr == ((Obj *) vmcur->arg2)->ptr);
}
static void VM__Exec_Ne_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) =
   *((Int *) vmcur->arg1) != *((Int *) vmcur->arg2);
}
static void VM__Exec_Ne_RR(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT].ptr) =
   *((Real *) vmcur->arg1) != *((Real *) vmcur->arg2);
}
static void VM__Exec_Ne_PP(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT].ptr) =
     ( ((Point *) vmcur->arg1)->x != ((Point *) vmcur->arg2)->x )
  || ( ((Point *) vmcur->arg1)->y != ((Point *) vmcur->arg2)->y );
}
static void VM__Exec_Ne_OO(BoxVM *vm) {
  VMStatus *vmcur = vm->vmcur;
  *((Int *) vmcur->local[TYPE_INT].ptr) =
     (((Obj *) vmcur->arg1)->ptr != ((Obj *) vmcur->arg2)->ptr);
}

static void VM__Exec_Lt_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) =
   *((Int *) vmcur->arg1) < *((Int *) vmcur->arg2);
}
static void VM__Exec_Lt_RR(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT].ptr) =
   *((Real *) vmcur->arg1) < *((Real *) vmcur->arg2);
}
static void VM__Exec_Le_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) =
   *((Int *) vmcur->arg1) <= *((Int *) vmcur->arg2);
}
static void VM__Exec_Le_RR(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT].ptr) =
   *((Real *) vmcur->arg1) <= *((Real *) vmcur->arg2);
}
static void VM__Exec_Gt_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) =
   *((Int *) vmcur->arg1) > *((Int *) vmcur->arg2);
}
static void VM__Exec_Gt_RR(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT].ptr) =
   *((Real *) vmcur->arg1) > *((Real *) vmcur->arg2);
}
static void VM__Exec_Ge_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) =
   *((Int *) vmcur->arg1) >= *((Int *) vmcur->arg2);
}
static void VM__Exec_Ge_RR(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->local[TYPE_INT].ptr) =
   *((Real *) vmcur->arg1) >= *((Real *) vmcur->arg2);
}

static void VM__Exec_LNot_I(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) = ! *((Int *) vmcur->arg1);
}
static void VM__Exec_LAnd_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) =
   *((Int *) vmcur->arg1) && *((Int *) vmcur->arg2);
}
static void VM__Exec_LOr_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  *((Int *) vmcur->arg1) =
   *((Int *) vmcur->arg1) || *((Int *) vmcur->arg2);
}

static void VM__Exec_Malloc_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  Int size = *((Int *) vmcur->arg1),
      type = *((Int *) vmcur->arg2);
  Obj *obj = (Obj *) vmcur->local[TYPE_OBJ].ptr;
  BoxVM_Alloc(obj, size, type);
  if (obj->block != (void *) NULL) return;
  MSG_FATAL("VM_Exec_Malloc_II: memory request failed!");
}

static void VM__Exec_Mln_O(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  BoxVM_Link((Obj *) vmcur->arg1);
}

static void VM__Exec_MUnln_O(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  BoxVM_Unlink(vmp, (Obj *) vmcur->arg1);
}

static void VM__Exec_MCopy_OO(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  (void) memcpy(((Obj *) vmcur->arg1)->ptr,             /* destination */
                ((Obj *) vmcur->arg2)->ptr,             /* source */
                *((Int *) vmcur->local[TYPE_INT].ptr)); /* size */
}

static void VM__Exec_Lea(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  Obj *obj = (Obj *) vmcur->local[TYPE_OBJ].ptr;
  obj->block = (void *) NULL;
  obj->ptr = vmcur->arg1;
}

static void VM__Exec_Lea_OO(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  BoxObj *obj = (BoxObj *) vmcur->arg1;
  obj->block = (void *) NULL;
  obj->ptr = vmcur->arg2;
}

static void VM__Exec_Push_O(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  BoxArr_Push(& vmp->stack, vmcur->arg1);
}

static void VM__Exec_Pop_O(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  BoxArr_Pop(& vmp->stack, vmcur->arg1);
}

static void VM__Exec_Jmp_I(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  vmcur->i_len = *((Int *) vmcur->arg1);
}

static void VM__Exec_Jc_I(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  if (*((Int *) vmcur->local[TYPE_INT].ptr))
    vmcur->i_len = *((Int *) vmcur->arg1);
}

static void VM__Exec_Add_O(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  ((Obj *) vmcur->arg1)->ptr += *((Int *) vmcur->local[TYPE_INT].ptr);
}

static void VM__Exec_Arinit_I(BoxVM *vm) {
  VMStatus *vmcur = vm->vmcur;
  BoxArray *arr = (BoxArray *) ((Obj *) vmcur->local[TYPE_OBJ].ptr)->ptr;
  BoxInt num_dim = *((Int *) vmcur->arg1);
  ASSERT_TASK( BoxArray_Init(arr, num_dim) );
}

static void VM__Exec_Arsize_I(BoxVM *vm) {
  VMStatus *vmcur = vm->vmcur;
  BoxArray *arr = (BoxArray *) ((Obj *) vmcur->local[TYPE_OBJ].ptr)->ptr;
  BoxInt size = *((Int *) vmcur->arg1);
  ASSERT_TASK( BoxArray_Set_Size(arr, size) );
}

static void VM__Exec_Araddr_II(BoxVM *vm) {
  VMStatus *vmcur = vm->vmcur;
  BoxArray *arr = (BoxArray *) ((Obj *) vmcur->local[TYPE_OBJ].ptr)->ptr;
  size_t addr =  *((Int *) vmcur->local[TYPE_INT].ptr);
  BoxInt index = *((Int *) vmcur->arg1),
         dim = *((Int *) vmcur->arg2);
  ASSERT_TASK( BoxArray_Calc_Address(arr, & addr, dim, index) );
  *((Int *) vmcur->local[TYPE_INT].ptr) = (Int) addr;
}

static void VM__Exec_Arget_OO(BoxVM *vm) {
  VMStatus *vmcur = vm->vmcur;
  BoxObj *item = (Obj *) vmcur->arg1;
  BoxArray *arr = (BoxArray *) ((Obj *) vmcur->arg2)->ptr;
  size_t addr =  *((Int *) vmcur->local[TYPE_INT].ptr);
  BoxArray_Access(arr, item, addr);
}

static void VM__Exec_Arnext_OO(BoxVM *vm) {
  /** Not implemented yet */
}

static void VM__Exec_Ardest_O(BoxVM *vm) {
  VMStatus *vmcur = vm->vmcur;
  BoxArray *arr = (BoxArray *) ((Obj *) vmcur->arg1)->ptr;
  BoxArray_Finish(vm, arr);
}

/******************************************************************************
 * La seguente tabella descrive le istruzioni e specifica quali funzioni      *
 * debbano essere chiamate per localizzare la posizione in memoria degli      *
 * argomenti e quali per eseguire l'azione associata all'istruzione stessa.   *
 ******************************************************************************/
VMInstrDesc vm_instr_desc_table[] = {
  {"call",  1, TYPE_INT,   VM__GLPI,     VM__Exec_Call_I,    VM__D_CALL     }, /* call reg_i         */
  {"call",  1, TYPE_INT,   VM__Imm,      VM__Exec_Call_I,    VM__D_CALL     }, /* call imm_i         */
  {"newc",  2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_NewC_II,   VM__D_GLPI_GLPI}, /* newc imm_i, imm_i  */
  {"newi",  2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_NewI_II,   VM__D_GLPI_GLPI}, /* newi imm_i, imm_i  */
  {"newr",  2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_NewR_II,   VM__D_GLPI_GLPI}, /* newr imm_i, imm_i  */
  {"newp",  2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_NewP_II,   VM__D_GLPI_GLPI}, /* newp imm_i, imm_i  */
  {"newo",  2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_NewO_II,   VM__D_GLPI_GLPI}, /* newo imm_i, imm_i  */
  {"mov",   2, TYPE_CHAR,  VM__GLP_Imm,  VM__Exec_Mov_CC,    VM__D_GLPI_Imm }, /* mov reg_c, imm_c   */
  {"mov",   2, TYPE_INT,   VM__GLP_Imm,  VM__Exec_Mov_II,    VM__D_GLPI_Imm }, /* mov reg_i, imm_i   */
  {"mov",   2, TYPE_REAL,  VM__GLP_Imm,  VM__Exec_Mov_RR,    VM__D_GLPI_Imm }, /* mov reg_r, imm_r   */
  {"mov",   2, TYPE_POINT, VM__GLP_Imm,  VM__Exec_Mov_PP,    VM__D_GLPI_Imm }, /* mov reg_p, imm_p   */
  {"mov",   2, TYPE_CHAR,  VM__GLP_GLPI, VM__Exec_Mov_CC,    VM__D_GLPI_GLPI}, /* mov reg_c, reg_c   */
  {"mov",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Mov_II,    VM__D_GLPI_GLPI}, /* mov reg_i, reg_i   */
  {"mov",   2, TYPE_REAL,  VM__GLP_GLPI, VM__Exec_Mov_RR,    VM__D_GLPI_GLPI}, /* mov reg_r, reg_r   */
  {"mov",   2, TYPE_POINT, VM__GLP_GLPI, VM__Exec_Mov_PP,    VM__D_GLPI_GLPI}, /* mov reg_p, reg_p   */
  {"mov",   2, TYPE_OBJ,   VM__GLP_GLPI, VM__Exec_Mov_OO,    VM__D_GLPI_GLPI}, /* mov reg_o, reg_o   */
  {"bnot",  1, TYPE_INT,   VM__GLPI,     VM__Exec_BNot_I,    VM__D_GLPI_GLPI}, /* bnot reg_i         */
  {"band",  2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_BAnd_II,   VM__D_GLPI_GLPI}, /* band reg_i, reg_i  */
  {"bxor",  2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_BXor_II,   VM__D_GLPI_GLPI}, /* bxor reg_i, reg_i  */
  {"bor",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_BOr_II,    VM__D_GLPI_GLPI}, /* bor reg_i, reg_i   */
  {"shl",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Shl_II,    VM__D_GLPI_GLPI}, /* shl reg_i, reg_i   */
  {"shr",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Shr_II,    VM__D_GLPI_GLPI}, /* shr reg_i, reg_i   */
  {"inc",   1, TYPE_INT,   VM__GLPI,     VM__Exec_Inc_I,     VM__D_GLPI_GLPI}, /* inc reg_i          */
  {"inc",   1, TYPE_REAL,  VM__GLPI,     VM__Exec_Inc_R,     VM__D_GLPI_GLPI}, /* inc reg_r          */
  {"dec",   1, TYPE_INT,   VM__GLPI,     VM__Exec_Dec_I,     VM__D_GLPI_GLPI}, /* dec reg_i          */
  {"dec",   1, TYPE_REAL,  VM__GLPI,     VM__Exec_Dec_R,     VM__D_GLPI_GLPI}, /* dec reg_r          */
  {"pow",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Pow_II,    VM__D_GLPI_GLPI}, /* pow reg_i, reg_i   */
  {"pow",   2, TYPE_REAL,  VM__GLP_GLPI, VM__Exec_Pow_RR,    VM__D_GLPI_GLPI}, /* pow reg_r, reg_r   */
  {"add",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Add_II,    VM__D_GLPI_GLPI}, /* add reg_i, reg_i   */
  {"add",   2, TYPE_REAL,  VM__GLP_GLPI, VM__Exec_Add_RR,    VM__D_GLPI_GLPI}, /* add reg_r, reg_r   */
  {"add",   2, TYPE_POINT, VM__GLP_GLPI, VM__Exec_Add_PP,    VM__D_GLPI_GLPI}, /* add reg_p, reg_p   */
  {"sub",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Sub_II,    VM__D_GLPI_GLPI}, /* sub reg_i, reg_i   */
  {"sub",   2, TYPE_REAL,  VM__GLP_GLPI, VM__Exec_Sub_RR,    VM__D_GLPI_GLPI}, /* sub reg_r, reg_r   */
  {"sub",   2, TYPE_POINT, VM__GLP_GLPI, VM__Exec_Sub_PP,    VM__D_GLPI_GLPI}, /* sub reg_p, reg_p   */
  {"mul",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Mul_II,    VM__D_GLPI_GLPI}, /* mul reg_i, reg_i   */
  {"mul",   2, TYPE_REAL,  VM__GLP_GLPI, VM__Exec_Mul_RR,    VM__D_GLPI_GLPI}, /* mul reg_r, reg_r   */
  {"div",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Div_II,    VM__D_GLPI_GLPI}, /* div reg_i, reg_i   */
  {"div",   2, TYPE_REAL,  VM__GLP_GLPI, VM__Exec_Div_RR,    VM__D_GLPI_GLPI}, /* div reg_r, reg_r   */
  {"rem",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Rem_II,    VM__D_GLPI_GLPI}, /* rem reg_i, reg_i   */
  {"neg",   1, TYPE_INT,   VM__GLPI,     VM__Exec_Neg_I,     VM__D_GLPI_GLPI}, /* neg reg_i          */
  {"neg",   1, TYPE_REAL,  VM__GLPI,     VM__Exec_Neg_R,     VM__D_GLPI_GLPI}, /* neg reg_r          */
  {"neg",   1, TYPE_POINT, VM__GLPI,     VM__Exec_Neg_P,     VM__D_GLPI_GLPI}, /* neg reg_p          */
  {"pmulr", 1, TYPE_POINT, VM__GLPI,     VM__Exec_PMulR_PR,  VM__D_GLPI_GLPI}, /* pmulr reg_p        */
  {"pdivr", 1, TYPE_POINT, VM__GLPI,     VM__Exec_PDivR_PR,  VM__D_GLPI_GLPI}, /* pdivr reg_p        */
  {"eq?",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Eq_II,     VM__D_GLPI_GLPI}, /* eq? reg_i, reg_i   */
  {"eq?",   2, TYPE_REAL,  VM__GLP_GLPI, VM__Exec_Eq_RR,     VM__D_GLPI_GLPI}, /* eq? reg_r, reg_r   */
  {"eq?",   2, TYPE_POINT, VM__GLP_GLPI, VM__Exec_Eq_PP,     VM__D_GLPI_GLPI}, /* eq? reg_p, reg_p   */
  {"eq?",   2, TYPE_OBJ,   VM__GLP_GLPI, VM__Exec_Eq_OO,     VM__D_GLPI_GLPI}, /* eq? reg_o, reg_o   */
  {"ne?",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Ne_II,     VM__D_GLPI_GLPI}, /* ne? reg_i, reg_i   */
  {"ne?",   2, TYPE_REAL,  VM__GLP_GLPI, VM__Exec_Ne_RR,     VM__D_GLPI_GLPI}, /* ne? reg_r, reg_r   */
  {"ne?",   2, TYPE_POINT, VM__GLP_GLPI, VM__Exec_Ne_PP,     VM__D_GLPI_GLPI}, /* ne? reg_p, reg_p   */
  {"ne?",   2, TYPE_OBJ,   VM__GLP_GLPI, VM__Exec_Ne_OO,     VM__D_GLPI_GLPI}, /* ne? reg_o, reg_o   */
  {"lt?",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Lt_II,     VM__D_GLPI_GLPI}, /* lt? reg_i, reg_i   */
  {"lt?",   2, TYPE_REAL,  VM__GLP_GLPI, VM__Exec_Lt_RR,     VM__D_GLPI_GLPI}, /* lt? reg_r, reg_r   */
  {"le?",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Le_II,     VM__D_GLPI_GLPI}, /* le? reg_i, reg_i   */
  {"le?",   2, TYPE_REAL,  VM__GLP_GLPI, VM__Exec_Le_RR,     VM__D_GLPI_GLPI}, /* le? reg_r, reg_r   */
  {"gt?",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Gt_II,     VM__D_GLPI_GLPI}, /* gt? reg_i, reg_i   */
  {"gt?",   2, TYPE_REAL,  VM__GLP_GLPI, VM__Exec_Gt_RR,     VM__D_GLPI_GLPI}, /* gt? reg_r, reg_r   */
  {"ge?",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Ge_II,     VM__D_GLPI_GLPI}, /* ge? reg_i, reg_i   */
  {"ge?",   2, TYPE_REAL,  VM__GLP_GLPI, VM__Exec_Ge_RR,     VM__D_GLPI_GLPI}, /* ge? reg_r, reg_r   */
  {"lnot",  1, TYPE_INT,   VM__GLP_GLPI, VM__Exec_LNot_I,    VM__D_GLPI_GLPI}, /* lnot reg_i         */
  {"land",  2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_LAnd_II,   VM__D_GLPI_GLPI}, /* land reg_i, reg_i  */
  {"lor",   2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_LOr_II,    VM__D_GLPI_GLPI}, /* lor  reg_i, reg_i  */
  {"real",  1, TYPE_CHAR,  VM__GLPI,     VM__Exec_Real_C,    VM__D_GLPI_GLPI}, /* real reg_c         */
  {"real",  1, TYPE_INT,   VM__GLPI,     VM__Exec_Real_I,    VM__D_GLPI_GLPI}, /* real reg_i         */
  {"int",   1, TYPE_CHAR,  VM__GLPI,     VM__Exec_Int_C,     VM__D_GLPI_GLPI}, /* int reg_c          */
  {"int",   1, TYPE_REAL,  VM__GLPI,     VM__Exec_Int_R,     VM__D_GLPI_GLPI}, /* int reg_r          */
  {"point", 2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Point_II,  VM__D_GLPI_GLPI}, /* point reg_i, reg_i */
  {"point", 2, TYPE_REAL,  VM__GLP_GLPI, VM__Exec_Point_RR,  VM__D_GLPI_GLPI}, /* point reg_r, reg_r */
  {"projx", 1, TYPE_POINT, VM__GLPI,     VM__Exec_ProjX_P,   VM__D_GLPI_GLPI}, /* projx reg_p        */
  {"projy", 1, TYPE_POINT, VM__GLPI,     VM__Exec_ProjY_P,   VM__D_GLPI_GLPI}, /* projy reg_p        */
  {"pptrx", 1, TYPE_POINT, VM__GLPI,     VM__Exec_PPtrX_P,   VM__D_GLPI_GLPI}, /* pptrx reg_p        */
  {"pptry", 1, TYPE_POINT, VM__GLPI,     VM__Exec_PPtrY_P,   VM__D_GLPI_GLPI}, /* pptry reg_p        */
  {"ret",   0, TYPE_NONE,  NULL,         VM__Exec_Ret,       VM__D_GLPI_GLPI}, /* ret                */
  {"malloc",2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Malloc_II, VM__D_GLPI_GLPI}, /* malloc reg_i       */
  {"mln",   1, TYPE_OBJ,   VM__GLPI,     VM__Exec_Mln_O,     VM__D_GLPI_GLPI}, /* mln reg_o          */
  {"munln", 1, TYPE_OBJ,   VM__GLPI,     VM__Exec_MUnln_O,   VM__D_GLPI_GLPI}, /* munln reg_o        */
  {"mcopy", 2, TYPE_OBJ,   VM__GLP_GLPI, VM__Exec_MCopy_OO,  VM__D_GLPI_GLPI}, /* mcopy reg_o, reg_o */
  {"lea",   1, TYPE_CHAR,  VM__GLPI,     VM__Exec_Lea,       VM__D_GLPI_GLPI}, /* lea c[ro0+...]     */
  {"lea",   1, TYPE_INT,   VM__GLPI,     VM__Exec_Lea,       VM__D_GLPI_GLPI}, /* lea i[ro0+...]     */
  {"lea",   1, TYPE_REAL,  VM__GLPI,     VM__Exec_Lea,       VM__D_GLPI_GLPI}, /* lea r[ro0+...]     */
  {"lea",   1, TYPE_POINT, VM__GLPI,     VM__Exec_Lea,       VM__D_GLPI_GLPI}, /* lea p[ro0+...]     */
  {"lea",   2, TYPE_OBJ,   VM__GLP_GLPI, VM__Exec_Lea_OO,    VM__D_GLPI_GLPI}, /* lea reg_o, o[ro0+...] */
  {"push",  1, TYPE_OBJ,   VM__GLPI,     VM__Exec_Push_O,    VM__D_GLPI_GLPI}, /* push reg_o         */
  {"pop",   1, TYPE_OBJ,   VM__GLPI,     VM__Exec_Pop_O,     VM__D_GLPI_GLPI}, /* pop reg_o          */
  {"jmp",   1, TYPE_INT,   VM__GLPI,     VM__Exec_Jmp_I,     VM__D_JMP      }, /* jmp reg_i          */
  {"jc",    1, TYPE_INT,   VM__GLPI,     VM__Exec_Jc_I,      VM__D_JMP      }, /* jc  reg_i          */
  {"add",   1, TYPE_OBJ,   VM__GLPI,     VM__Exec_Add_O,     VM__D_GLPI_GLPI}, /* add reg_o          */
  {"arinit",1, TYPE_INT,   VM__GLPI,     VM__Exec_Arinit_I,  VM__D_GLPI_GLPI}, /* arinit reg_i       */
  {"arsize",1, TYPE_INT,   VM__GLPI,     VM__Exec_Arsize_I,  VM__D_GLPI_GLPI}, /* arsize reg_i       */
  {"araddr",2, TYPE_INT,   VM__GLP_GLPI, VM__Exec_Araddr_II, VM__D_GLPI_GLPI}, /* araddr reg_i, reg_i */
  {"arget", 2, TYPE_OBJ,   VM__GLP_GLPI, VM__Exec_Arget_OO,  VM__D_GLPI_GLPI}, /* arget reg_o, reg_o  */
  {"arnext",2, TYPE_OBJ,   VM__GLP_GLPI, VM__Exec_Arnext_OO, VM__D_GLPI_GLPI}, /* arnext reg_o, reg_o */
  {"ardest",1, TYPE_OBJ,   VM__GLPI,     VM__Exec_Ardest_O,  VM__D_GLPI_GLPI}  /* ardest reg_o */
};

int BoxOp_Get_Num_Args(BoxOpcode op) {
  if (op < 1 || op >= BOX_NUM_OPS)
    return -1;
  else
    return vm_instr_desc_table[op].numargs;
}

BoxType BoxOp_Get_Arg_Type(BoxOpcode op) {
  if (op < 1 || op >= BOX_NUM_OPS)
    return BOXTYPE_NONE;
  else
    return vm_instr_desc_table[op].t_id;
}

typedef struct {
  BoxGOp      g_opcode;        /**< Generic Opcode */
  const char  *name;           /**< Name of the operation */
  char        num_args;        /**< Number of explicit arguments */
  char        arg_type;        /**< Type of the explicit arguments */
  const char  *input_regs,     /**< List of */
              *output_regs,    /**<*/
              *assembler,      /**<*/
              *disassembler;   /**<*/
  void        *executor;       /**<*/
} BoxOpTable4Humans;

static BoxOpTable4Humans op_table_for_humans[] = {
  {  BOXGOP_CALL,   "call", 1, 'i',     "a1",  NULL, "x-", "c-", VM__Exec_Call_I   }, /* call ri       */
  {  BOXGOP_CALL,   "call", 1, 'i',     "a1",  NULL, "i-", "c-", VM__Exec_Call_I   }, /* call ii       */
  {  BOXGOP_NEWC,   "newc", 2, 'i',  "a1,a2",  NULL, "xx", "xx", VM__Exec_NewC_II  }, /* newc ii, ii   */
  {  BOXGOP_NEWC,   "newi", 2, 'i',  "a1,a2",  NULL, "xx", "xx", VM__Exec_NewI_II  }, /* newi ii, ii   */
  {  BOXGOP_NEWC,   "newr", 2, 'i',  "a1,a2",  NULL, "xx", "xx", VM__Exec_NewR_II  }, /* newr ii, ii   */
  {  BOXGOP_NEWC,   "newp", 2, 'i',  "a1,a2",  NULL, "xx", "xx", VM__Exec_NewP_II  }, /* newp ii, ii   */
  {  BOXGOP_NEWC,   "newo", 2, 'i',  "a1,a2",  NULL, "xx", "xx", VM__Exec_NewO_II  }, /* newo ii, ii   */
  {   BOXGOP_MOV,    "mov", 2, 'c',     "a2",  "a1", "xi", "xi", VM__Exec_Mov_CC   }, /* mov rc, ic    */
  {   BOXGOP_MOV,    "mov", 2, 'i',     "a2",  "a1", "xi", "xi", VM__Exec_Mov_II   }, /* mov ri, ii    */
  {   BOXGOP_MOV,    "mov", 2, 'r',     "a2",  "a1", "xi", "xi", VM__Exec_Mov_RR   }, /* mov rr, ir    */
  {   BOXGOP_MOV,    "mov", 2, 'p',     "a2",  "a1", "xi", "xi", VM__Exec_Mov_PP   }, /* mov rp, ip    */
  {   BOXGOP_MOV,    "mov", 2, 'c',     "a2",  "a1", "xx", "xx", VM__Exec_Mov_CC   }, /* mov rc, rc    */
  {   BOXGOP_MOV,    "mov", 2, 'i',     "a2",  "a1", "xx", "xx", VM__Exec_Mov_II   }, /* mov ri, ri    */
  {   BOXGOP_MOV,    "mov", 2, 'r',     "a2",  "a1", "xx", "xx", VM__Exec_Mov_RR   }, /* mov rr, rr    */
  {   BOXGOP_MOV,    "mov", 2, 'p',     "a2",  "a1", "xx", "xx", VM__Exec_Mov_PP   }, /* mov rp, rp    */
  {   BOXGOP_MOV,    "mov", 2, 'o',     "a2",  "a1", "xx", "xx", VM__Exec_Mov_OO   }, /* mov ro, ro    */
  {  BOXGOP_BNOT,   "bnot", 1, 'i',     "a1",  "a1", "x-", "xx", VM__Exec_BNot_I   }, /* bnot ri       */
  {  BOXGOP_BAND,   "band", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_BAnd_II  }, /* band ri, ri   */
  {  BOXGOP_BXOR,   "bxor", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_BXor_II  }, /* bxor ri, ri   */
  {   BOXGOP_BOR,    "bor", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_BOr_II   }, /* bor ri, ri    */
  {   BOXGOP_SHL,    "shl", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Shl_II   }, /* shl ri, ri    */
  {   BOXGOP_SHR,    "shr", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Shr_II   }, /* shr ri, ri    */
  {   BOXGOP_INC,    "inc", 1, 'i',     "a1",  "a1", "x-", "xx", VM__Exec_Inc_I    }, /* inc ri        */
  {   BOXGOP_INC,    "inc", 1, 'r',     "a1",  "a1", "x-", "xx", VM__Exec_Inc_R    }, /* inc rr        */
  {   BOXGOP_DEC,    "dec", 1, 'i',     "a1",  "a1", "x-", "xx", VM__Exec_Dec_I    }, /* dec ri        */
  {   BOXGOP_DEC,    "dec", 1, 'r',     "a1",  "a1", "x-", "xx", VM__Exec_Dec_R    }, /* dec rr        */
  {   BOXGOP_POW,    "pow", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Pow_II   }, /* pow ri, ri    */
  {   BOXGOP_POW,    "pow", 2, 'r',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Pow_RR   }, /* pow rr, rr    */
  {   BOXGOP_ADD,    "add", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Add_II   }, /* add ri, ri    */
  {   BOXGOP_ADD,    "add", 2, 'r',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Add_RR   }, /* add rr, rr    */
  {   BOXGOP_ADD,    "add", 2, 'p',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Add_PP   }, /* add rp, rp    */
  {   BOXGOP_SUB,    "sub", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Sub_II   }, /* sub ri, ri    */
  {   BOXGOP_SUB,    "sub", 2, 'r',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Sub_RR   }, /* sub rr, rr    */
  {   BOXGOP_SUB,    "sub", 2, 'p',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Sub_PP   }, /* sub rp, rp    */
  {   BOXGOP_MUL,    "mul", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Mul_II   }, /* mul ri, ri    */
  {   BOXGOP_MUL,    "mul", 2, 'r',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Mul_RR   }, /* mul rr, rr    */
  {   BOXGOP_DIV,    "div", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Div_II   }, /* div ri, ri    */
  {   BOXGOP_DIV,    "div", 2, 'r',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Div_RR   }, /* div rr, rr    */
  {   BOXGOP_REM,    "rem", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Rem_II   }, /* rem ri, ri    */
  {   BOXGOP_NEG,    "neg", 1, 'i',     "a1",  "a1", "x-", "xx", VM__Exec_Neg_I    }, /* neg ri        */
  {   BOXGOP_NEG,    "neg", 1, 'r',     "a1",  "a1", "x-", "xx", VM__Exec_Neg_R    }, /* neg rr        */
  {   BOXGOP_NEG,    "neg", 1, 'p',     "a1",  "a1", "x-", "xx", VM__Exec_Neg_P    }, /* neg rp        */
  { BOXGOP_PMULR,  "pmulr", 1, 'p', "a1,rr0",  "a1", "x-", "xx", VM__Exec_PMulR_PR }, /* pmulr rp      */
  { BOXGOP_PDIVR,  "pdivr", 1, 'p', "a1,rr0",  "a1", "x-", "xx", VM__Exec_PDivR_PR }, /* pdivr rp      */
  {    BOXGOP_EQ,    "eq?", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Eq_II    }, /* eq? ri, ri    */
  {    BOXGOP_EQ,    "eq?", 2, 'r',  "a1,a2", "ri0", "xx", "xx", VM__Exec_Eq_RR    }, /* eq? rr, rr    */
  {    BOXGOP_EQ,    "eq?", 2, 'p',  "a1,a2", "ri0", "xx", "xx", VM__Exec_Eq_PP    }, /* eq? rp, rp    */
  {    BOXGOP_EQ,    "eq?", 2, 'o',  "a1,a2", "ri0", "xx", "xx", VM__Exec_Eq_OO    }, /* eq? ro, ro    */
  {    BOXGOP_NE,    "ne?", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Ne_II    }, /* ne? ri, ri    */
  {    BOXGOP_NE,    "ne?", 2, 'r',  "a1,a2", "ri0", "xx", "xx", VM__Exec_Ne_RR    }, /* ne? rr, rr    */
  {    BOXGOP_NE,    "ne?", 2, 'p',  "a1,a2", "ri0", "xx", "xx", VM__Exec_Ne_PP    }, /* ne? rp, rp    */
  {    BOXGOP_NE,    "ne?", 2, 'o',  "a1,a2", "ri0", "xx", "xx", VM__Exec_Ne_OO    }, /* ne? rp, rp    */
  {    BOXGOP_LT,    "lt?", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Lt_II    }, /* lt? ri, ri    */
  {    BOXGOP_LT,    "lt?", 2, 'r',  "a1,a2", "ri0", "xx", "xx", VM__Exec_Lt_RR    }, /* lt? rr, rr    */
  {    BOXGOP_LE,    "le?", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Le_II    }, /* le? ri, ri    */
  {    BOXGOP_LE,    "le?", 2, 'r',  "a1,a2", "ri0", "xx", "xx", VM__Exec_Le_RR    }, /* le? rr, rr    */
  {    BOXGOP_GT,    "gt?", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Gt_II    }, /* gt? ri, ri    */
  {    BOXGOP_GT,    "gt?", 2, 'r',  "a1,a2", "ri0", "xx", "xx", VM__Exec_Gt_RR    }, /* gt? rr, rr    */
  {    BOXGOP_GE,    "ge?", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_Ge_II    }, /* ge? ri, ri    */
  {    BOXGOP_GE,    "ge?", 2, 'r',  "a1,a2", "ri0", "xx", "xx", VM__Exec_Ge_RR    }, /* ge? rr, rr    */
  {  BOXGOP_LNOT,   "lnot", 1, 'i',     "a1",  "a1", "x-", "xx", VM__Exec_LNot_I   }, /* lnot ri       */
  {  BOXGOP_LAND,   "land", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_LAnd_II  }, /* land ri, ri   */
  {   BOXGOP_LOR,    "lor", 2, 'i',  "a1,a2",  "a1", "xx", "xx", VM__Exec_LOr_II   }, /* lor  ri, ri   */
  {  BOXGOP_REAL,   "real", 1, 'c',     "a1", "rr0", "x-", "xx", VM__Exec_Real_C   }, /* real rc       */
  {  BOXGOP_REAL,   "real", 1, 'i',     "a1", "rr0", "x-", "xx", VM__Exec_Real_I   }, /* real ri       */
  {   BOXGOP_INT,    "int", 1, 'c',     "a1", "ri0", "x-", "xx", VM__Exec_Int_C    }, /* int rc        */
  {   BOXGOP_INT,    "int", 1, 'r',     "a1", "ri0", "x-", "xx", VM__Exec_Int_R    }, /* int rr        */
  { BOXGOP_POINT,  "point", 2, 'i',  "a1,a2", "rp0", "xx", "xx", VM__Exec_Point_II }, /* point ri, ri  */
  { BOXGOP_POINT,  "point", 2, 'r',  "a1,a2", "rp0", "xx", "xx", VM__Exec_Point_RR }, /* point rr, rr  */
  { BOXGOP_PROJX,  "projx", 1, 'p',     "a1", "rr0", "x-", "xx", VM__Exec_ProjX_P  }, /* projx rp      */
  { BOXGOP_PROJY,  "projy", 1, 'p',     "a1", "rr0", "x-", "xx", VM__Exec_ProjY_P  }, /* projy rp      */
  { BOXGOP_PPTRX,  "pptrx", 1, 'p',     "a1", "ro0", "x-", "xx", VM__Exec_PPtrX_P  }, /* pptrx rp      */
  { BOXGOP_PPTRY,  "pptry", 1, 'p',     "a1", "ro0", "x-", "xx", VM__Exec_PPtrY_P  }, /* pptry rp      */
  {   BOXGOP_RET,    "ret", 0, 'n',     NULL,  NULL, "--", "xx", VM__Exec_Ret      }, /* ret           */
  {BOXGOP_MALLOC, "malloc", 2, 'i',  "a1,a2", "ro0", "xx", "xx", VM__Exec_Malloc_II}, /* malloc ri, ri */
  {   BOXGOP_MLN,    "mln", 1, 'o',     "a1",  NULL, "x-", "xx", VM__Exec_Mln_O    }, /* mln ro        */
  { BOXGOP_MUNLN,  "munln", 1, 'o',     "a1",  NULL, "x-", "xx", VM__Exec_MUnln_O  }, /* munln ro      */
  { BOXGOP_MCOPY,  "mcopy", 2, 'o',
                                 "a1,a2,ri0",  NULL, "xx", "xx", VM__Exec_MCopy_OO }, /* mcopy ro, ro    */
  {   BOXGOP_LEA,    "lea", 1, 'c',     "a1", "ro0", "x-", "xx", VM__Exec_Lea      }, /* lea c[ro0+...]  */
  {   BOXGOP_LEA,    "lea", 1, 'i',     "a1", "ro0", "x-", "xx", VM__Exec_Lea      }, /* lea i[ro0+...]  */
  {   BOXGOP_LEA,    "lea", 1, 'r',     "a1", "ro0", "x-", "xx", VM__Exec_Lea      }, /* lea r[ro0+...]  */
  {   BOXGOP_LEA,    "lea", 1, 'p',     "a1", "ro0", "x-", "xx", VM__Exec_Lea      }, /* lea p[ro0+...]  */
  {   BOXGOP_LEA,    "lea", 2, 'o',     "a2",  "a1", "xx", "xx", VM__Exec_Lea_OO   }, /* lea reg_o, o[ro0+...] */
  {  BOXGOP_PUSH,   "push", 1, 'o',     "a1",  NULL, "x-", "xx", VM__Exec_Push_O   }, /* push ro         */
  {   BOXGOP_POP,    "pop", 1, 'o',     NULL,  "a1", "x-", "xx", VM__Exec_Pop_O    }, /* pop ro          */
  {   BOXGOP_JMP,    "jmp", 1, 'i',     "a1",  NULL, "x-", "j-", VM__Exec_Jmp_I    }, /* jmp ri          */
  {    BOXGOP_JC,     "jc", 1, 'i', "a1,ri0",  NULL, "x-", "j-", VM__Exec_Jc_I     }, /* jc  ri          */
  {   BOXGOP_ADD,    "add", 1, 'o',    "ri0",  NULL, "x-", "xx", VM__Exec_Add_O    }, /* add ro          */
  {BOXGOP_ARINIT, "arinit", 1, 'i',     "a1", "ro0", "x-", "xx", VM__Exec_Arinit_I }, /* arinit ri       */
  {BOXGOP_ARSIZE, "arsize", 1, 'i', "a1,ro0",  NULL, "x-", "xx", VM__Exec_Arsize_I }, /* arsize ri       */
  {BOXGOP_ARADDR, "araddr", 2, 'i',
                             "a1,a2,ro0,ri0", "ri0", "xx", "xx", VM__Exec_Araddr_II}, /* araddr ri, ri */
  { BOXGOP_ARGET,  "arget", 2, 'o', "a2,ri0",  "a1", "xx", "xx", VM__Exec_Arget_OO }, /* arget reg_o, reg_o  */
  {BOXGOP_ARNEXT, "arnext", 2, 'o',     "a2",  "a1", "xx", "xx", VM__Exec_Arnext_OO}, /* arnext reg_o, reg_o */
  {BOXGOP_ARDEST, "ardest", 1, 'o',     "a1",  NULL, "x-", "xx", VM__Exec_Ardest_O }  /* ardest reg_o        */
};

/*

mov ro0, array
mov ro0, ro5
arsize i[ro0 + 8]



*  -> VM__GLPI
** -> VM__GLP_GLPI
*i -> VM__GLP_Imm
i  -> VM__Imm

** -> VM__D_GLPI_GLPI
*i -> VM__D_GLPI_Imm
j  -> VM__D_JMP
c  -> VM__D_CALL

*/

#define COMBINE_3CHAR(c1, c2, c3) \
  (((int) (c3)) | (((int) (c2)) << 8) | (((int) (c1)) << 16))

/** Return a BoxOpSignature from the corresponding string representation. */
static BoxOpSignature My_BoxOpSignature_From_String(const char *s) {
  switch (COMBINE_3CHAR(s[0], s[1], s[2])) {
  case COMBINE_3CHAR('-', '-', '\0'): return BOXOPSIGNATURE_NONE;
  case COMBINE_3CHAR('i', '-', '\0'): return BOXOPSIGNATURE_IMM;
  case COMBINE_3CHAR('x', '-', '\0'): return BOXOPSIGNATURE_ANY;
  case COMBINE_3CHAR('x', 'i', '\0'): return BOXOPSIGNATURE_ANY_IMM;
  case COMBINE_3CHAR('x', 'x', '\0'): return BOXOPSIGNATURE_ANY_ANY;
  default:
    printf("cannot classify '%s'!\n", s);
    assert(0);
  }
}

/** Count how many commas are present on the given string. */
static char My_Count_Commas(const char *s) {
  if (s == NULL)
    return 0;

  else {
    char count = 0;
    for(; *s != '\0'; s++)
      count += (*s == ',');
    return count + 1;
  }
}

static int My_Parse_Reg_List(const char **reg_list, char arg_type,
                             char io, BoxOpReg *r) {
  const char *s = *reg_list;

  if (s == NULL)
    return 0;

  else if (*s == '\0')
    return 0;

  else {
    char kind = *(s++), type, num_letter;

    if (kind == ',')
      kind = *(s++);

    if (kind == 'a') {
      type = arg_type;
      num_letter = *(s++);

    } else if (kind == 'r') {
      type = *(s++);
      num_letter = *(s++);

    } else {
      fprintf(stderr, "My_Parse_Reg_List: found char '%c', aborting!", kind);
      assert(0);
      return 0;
    }

    r->kind = kind;
    r->type = type;
    r->num  = Box_Hex_Digit_To_Int(num_letter);
    r->io   = io;
    *reg_list = s;
    return 1;
  }
}

static int My_Regs_Are_Equal(const BoxOpReg *a, const BoxOpReg *b) {
  return a->kind == b->kind && a->type == b->type && a->num == b->num;
}

void BoxOpTable_Build(BoxOpTable *ot) {
  int i, outside_idx, num_regs_to_alloc;
  BoxOpReg *reg;

  /* The following is what we want to achieve here:
   * The info table must contain BOX_NUM_OPS entries: one for each operation.
   * The first BOX_NUM_GOPS operations correspond exactly to the generic
   * operation (enum BoxGOp). This means that, when we get a BoxGOp we can use
   * it as the index to access the table.
   * For example: if the gop is BOXGOP_MOV, then ot->info[BOXGOP_MOV] refers
   * to one of the operations corresponding to BOXGOP_MOV. Let'say it refers
   * to BOXOP_MOV_II. Then all the other operations (such as BOXOP_MOV_RR,
   * BOXOP_MOV_PP, etc.) are linked in a chain starting from
   * ot->info[BOXGOP_MOV]->next and are physically stored in the part of the
   * ot->info array after the first BOX_NUM_GOPS operations.
   */

  /* To organize things this way we first clean the array */
  for(i = 0; i < BOX_NUM_GOPS; i++)
    ot->info[i].name = NULL; /* this is how we mark empty items */

  /* Now we populate the table */
  outside_idx = BOX_NUM_GOPS; /* index to items outside the lower table */
  num_regs_to_alloc = 0; /* Need to count the total num of input and output
                            args to allocate properly an array later */
  for(i = 0; i < BOX_NUM_OPS; i++) {
    BoxOpTable4Humans *h_ot = & op_table_for_humans[i];
    BoxGOp g_opcode = h_ot->g_opcode;
    BoxOpInfo *oi = & ot->info[g_opcode];

    /* We first check if the entry is free or not */
    if (oi->name == NULL) {
      /* It is! Then we work on the first part of the table! */
      oi->next = NULL;

    } else {
      /* No, it has been already occupied! We work outside the lower table! */
      BoxOpInfo *next = oi->next;
      oi->next = & ot->info[outside_idx++]; /* update the chain */
      oi = oi->next;
      oi->next = next;
    }

    oi->name = h_ot->name; /* This also marks the item as occupied */
    oi->opcode = i;
    oi->g_opcode = h_ot->g_opcode;
    oi->signature = My_BoxOpSignature_From_String(h_ot->assembler);
    oi->dasm = 0; /* change me */
    oi->arg_type = h_ot->arg_type;
    oi->num_args = h_ot->num_args;
    oi->num_inputs = My_Count_Commas(h_ot->input_regs);
    oi->num_outputs = My_Count_Commas(h_ot->output_regs);
    oi->executor = h_ot->executor;

    num_regs_to_alloc += oi->num_inputs + oi->num_outputs;
  }

  /* Now we can generate, for each operation, the list of input and output
   * registers
   */

  reg = ot->regs = BoxMem_Safe_Alloc(sizeof(BoxOpReg)*num_regs_to_alloc);

  for(i = 0; i < BOX_NUM_OPS; i++) {
    BoxOpInfo *oi = & ot->info[i];
    BoxOpTable4Humans *h_ot;
    const char *token;
    int num_regs, num_out_regs;

    assert(oi->name != NULL);
    h_ot = & op_table_for_humans[oi->opcode];

    /* Parse the string containing the output registers and transform it into
     * an array of BoxOpReg structures pointed by oi->regs
     */
    oi->regs = reg;
    token = h_ot->output_regs;
    num_regs = 0;
    while(My_Parse_Reg_List(& token, h_ot->arg_type, 'o', reg)) {
      ++num_regs;
      ++reg;
    }

    assert(num_regs == oi->num_outputs);
    num_out_regs = num_regs;

    /* Do a similar things for the input registers */
    token = h_ot->input_regs;
    while(My_Parse_Reg_List(& token, h_ot->arg_type, 'i', reg)) {
      int j, found;
      /* Check if this register was also an output register */
      found = 0;
      for(j = 0; j < num_out_regs; j++)
        if (My_Regs_Are_Equal(reg, & oi->regs[j])) {
          /* yes. then mark it as input/output */
          oi->regs[j].io = 'b';
          found = 1;
          break;
        }

      if (!found) {
        /* no. then add it as input only */
        ++num_regs;
        ++reg;
      }
    }

    assert(num_regs <= BOXOP_MAX_NUM_ARGS);

    oi->num_regs = num_regs;
  }
}

void BoxOpTable_Destroy(BoxOpTable *ot) {
  BoxMem_Free(ot->regs);
}

void BoxOpInfo_Print(FILE *out, BoxOpInfo *oi) {
  for(; oi != NULL; oi = oi->next) {
    int j;
    const char *sep = " ";
    fprintf(out, "  %s", oi->name);
    for(j = 0; j < oi->num_regs; j++) {
      const char *io;
      BoxOpReg *reg = & oi->regs[j];
      switch(reg->io) {
      case 'i': io = "i"; break;
      case 'o': io = "o"; break;
      case 'b': io = "i/o"; break;
      default:  io = "?"; break;
      }
      fprintf(out, "%s%c%c%d(%s)", sep, reg->kind, reg->type,
              (int) reg->num, io);
      sep = ", ";
    }
    fprintf(out, "\n");
  }
}

void BoxOpTable_Print(FILE *out, BoxOpTable *ot) {
  int i;
  for(i = 0; i < BOX_NUM_GOPS; i++) {
    BoxOpInfo *oi = & ot->info[i];
    fprintf(out, "Operations for '%s':\n", oi->name);
    BoxOpInfo_Print(out, oi);
  }
}
