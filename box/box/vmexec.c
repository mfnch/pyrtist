/****************************************************************************
 * Copyright (C) 2008, 2009, 2010 by Matteo Franchin                        *
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

static void *VM__Exec_X_II(BoxVM *vm, int type_id, size_t type_size,
                           size_t *num_regs) {
  VMStatus *vmcur = vm->vmcur;
  BoxInt numvar = *((BoxInt *) vmcur->arg1),
         numreg = *((BoxInt *) vmcur->arg2),
         numtot = numvar + numreg + 1;

  if ((vmcur->alc[type_id] & 1) == 0) {
    if(numvar >= 0 && numtot > numvar) {
      void *ptr = calloc(numtot, type_size);
      if (ptr != NULL) {
        BoxVMRegs *regs = & vmcur->local[type_id];
        regs->min = -numvar;
        regs->max = numreg;
        regs->ptr = ptr + type_size*numvar;
        vmcur->alc[type_id] |= 1;
        if (num_regs != (size_t *) NULL)
          *num_regs = numtot;
        return ptr;

      } else
        MSG_FATAL("new(%d): Cannot allocate memory for registers.", type_id);

    } else
      MSG_FATAL("new(%d): Negative arguments.", type_id);

  } else
    MSG_FATAL("new(%d): Double register allocation.", type_id);

  vmcur->flags.error = vmcur->flags.exit = 1;
  return NULL;
}

static void VM__Exec_NewC_II(BoxVM *vm) {
  (void) VM__Exec_X_II(vm, BOXTYPE_CHAR, sizeof(BoxChar), NULL);
}

static void VM__Exec_NewI_II(BoxVM *vm) {
  (void) VM__Exec_X_II(vm, BOXTYPE_INT, sizeof(BoxInt), NULL);
}

static void VM__Exec_NewR_II(BoxVM *vm) {
  (void) VM__Exec_X_II(vm, BOXTYPE_REAL, sizeof(BoxReal), NULL);
}

static void VM__Exec_NewP_II(BoxVM *vm) {
  (void) VM__Exec_X_II(vm, BOXTYPE_POINT, sizeof(BoxPoint), NULL);
}

static void VM__Exec_NewO_II(BoxVM *vm) {
  size_t num_regs, i;
  BoxPtr *regs =
    (BoxPtr *) VM__Exec_X_II(vm, BOXTYPE_PTR, sizeof(BoxPtr), & num_regs);
  if (regs != NULL) {
    for(i = 0; i < num_regs; i++)
      BoxPtr_Nullify(regs + i);
  }
}

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
static void VM__Exec_Mov_OO(BoxVM *vm) {
  VMStatus *vmcur = vm->vmcur;
  *((BoxPtr *) vmcur->arg1) = *((BoxPtr *) vmcur->arg2);
  BoxPtr_Detach((BoxPtr *) vmcur->arg1);
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

static void VM__Exec_Create_I(BoxVM *vm) {
  VMStatus *vmcur = vm->vmcur;
  BoxInt id = *((Int *) vmcur->arg1);
  BoxPtr *obj = (Obj *) vmcur->local[TYPE_OBJ].ptr;
  BoxVM_Obj_Create(vm, obj, id);
  if (!BoxPtr_Is_Null(obj))
    return;
  MSG_FATAL("VM_Exec_Create_I: cannot create object with alloc-ID=%I.", id);
}

static void VM__Exec_Reloc_OO(BoxVM *vm) {
  VMStatus *vmcur = vm->vmcur;
  BoxPtr *dest = (BoxPtr *) vmcur->arg1,
         *src = (BoxPtr *) vmcur->arg2;
  if (BoxVM_Obj_Relocate(vm, dest, src) != BOXTASK_OK)
    return;
  vm->vmcur->flags.error = 1;
  vm->vmcur->flags.exit = 1;
}

static void VM__Exec_Malloc_II(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  BoxInt size = *((Int *) vmcur->arg1),
         id = *((Int *) vmcur->arg2);
  BoxPtr *obj = (Obj *) vmcur->local[TYPE_OBJ].ptr;
  BoxVM_Alloc(vmp, obj, size, id);
  if (!BoxPtr_Is_Null(obj))
    return;
  MSG_FATAL("VM_Exec_Malloc_II: memory request failed!");
}

static void VM__Exec_Mln_O(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  BoxVM_Link((Obj *) vmcur->arg1);
}

static void VM__Exec_MUnln_O(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  BoxVM_Unlink(vmp, (BoxPtr *) vmcur->arg1);
}

static void VM__Exec_MCopy_OO(BoxVM *vmp) {
  VMStatus *vmcur = vmp->vmcur;
  (void) memcpy(((Obj *) vmcur->arg1)->ptr,             /* destination */
                ((Obj *) vmcur->arg2)->ptr,             /* source */
                *((Int *) vmcur->local[TYPE_INT].ptr)); /* size */
}

static void VM__Exec_Shift_OO(BoxVM *vm) {
  VMStatus *vmcur = vm->vmcur;
  BoxPtr *arg1 = (BoxPtr *) vmcur->arg1,
         *arg2 = (BoxPtr *) vmcur->arg2;
  if (arg1 != arg2) {
    if (!BoxPtr_Is_Detached(arg1))
      BoxVM_Unlink(vm, arg1);
    *arg1 = *arg2;
    BoxPtr_Detach(arg2);
  }
}

static void VM__Exec_Ref_OO(BoxVM *vm) {
  VMStatus *vmcur = vm->vmcur;
  BoxPtr *arg1 = (BoxPtr *) vmcur->arg1,
         *arg2 = (BoxPtr *) vmcur->arg2;
  if (arg1 == arg2) {
    if (!BoxPtr_Is_Detached(arg1))
      BoxVM_Unlink(vm, arg1);
    *arg1 = *arg2;
    BoxVM_Link(arg2);
  }
}

static void VM__Exec_Null_O(BoxVM *vm) {
  BoxPtr_Nullify((BoxPtr *) vm->vmcur->arg1);
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
  {BOXGOP_CREATE, "create", 1, 'i',     "a1", "ro0", "x-", "xx", VM__Exec_Create_I }, /* create ri     */
  { BOXGOP_RELOC,  "reloc", 2, 'o',  "a1,a2",  "a2", "xx", "xx", VM__Exec_Reloc_OO }, /* reloc ro, ro  */
  {BOXGOP_MALLOC, "malloc", 2, 'i',  "a1,a2", "ro0", "xx", "xx", VM__Exec_Malloc_II}, /* malloc ri, ri */
  {   BOXGOP_MLN,    "mln", 1, 'o',     "a1",  NULL, "x-", "xx", VM__Exec_Mln_O    }, /* mln ro        */
  { BOXGOP_MUNLN,  "munln", 1, 'o',     "a1",  NULL, "x-", "xx", VM__Exec_MUnln_O  }, /* munln ro      */
  { BOXGOP_MCOPY,  "mcopy", 2, 'o',
                                 "a1,a2,ri0",  NULL, "xx", "xx", VM__Exec_MCopy_OO }, /* mcopy ro, ro    */
  { BOXGOP_SHIFT,  "shift", 2, 'o',     "a2",  "a1", "xx", "xx", VM__Exec_Shift_OO }, /* shift ro, ro    */
  {   BOXGOP_REF,    "ref", 2, 'o',     "a2",  "a1", "xx", "xx", VM__Exec_Ref_OO   }, /* ref ro, ro      */
  {  BOXGOP_NULL,   "null", 1, 'o',     NULL,  "a1", "x-", "xx", VM__Exec_Null_O   }, /* null ro         */
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


#define COMBINE_3CHAR(c1, c2, c3) \
  (((int) (c3)) | (((int) (c2)) << 8) | (((int) (c1)) << 16))

/** Return a BoxOpSignature from the corresponding string representation. */
static BoxOpSignature My_BoxOpSignature_From_Str(const char *s) {
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

BoxType My_Type_From_Char(char c) {
  switch(c) {
  case 'n': return BOXTYPE_NONE; break;
  case 'c': return BOXTYPE_CHAR; break;
  case 'i': return BOXTYPE_INT; break;
  case 'r': return BOXTYPE_REAL; break;
  case 'p': return BOXTYPE_POINT; break;
  case 'o': return BOXTYPE_PTR; break;
  default:
    MSG_FATAL("My_Type_From_Char: unknown type char '%c'", c);
    assert(0);
    return BOXTYPE_NONE;
  }
}

void *My_Executor_From_Str(const char *s) {
  switch(My_BoxOpSignature_From_Str(s)) {
  case BOXOPSIGNATURE_NONE: return NULL;
  case BOXOPSIGNATURE_IMM: return VM__Imm;
  case BOXOPSIGNATURE_ANY: return VM__GLPI;
  case BOXOPSIGNATURE_ANY_IMM: return VM__GLP_Imm;
  case BOXOPSIGNATURE_ANY_ANY: return VM__GLP_GLPI;
  default:
    MSG_FATAL("My_Executor_From_Str: unknown string '%s'", s);
    assert(0);
    return NULL;
  }
}

static void *My_Disassembler_From_Str(const char *s) {
  switch (COMBINE_3CHAR(s[0], s[1], s[2])) {
  case COMBINE_3CHAR('c', '-', '\0'): return VM__D_CALL;
  case COMBINE_3CHAR('x', 'i', '\0'): return VM__D_GLPI_Imm;
  case COMBINE_3CHAR('x', 'x', '\0'): return VM__D_GLPI_GLPI;
  case COMBINE_3CHAR('j', '-', '\0'): return VM__D_JMP;
  default:
    MSG_FATAL("My_Disassembler_From_Str: unknown string '%s'", s);
    assert(0);
    return NULL;
  }
}

const BoxVMInstrDesc *BoxVM_Get_Exec_Table(void) {
  static BoxVMInstrDesc the_optable[BOX_NUM_OPS],
                        *the_optable_ptr = NULL;

  if (the_optable_ptr != NULL)
    return the_optable_ptr;

  else {
    int i;

    for(i = 0; i < BOX_NUM_OPS; i++) {
      BoxVMInstrDesc *dest = & the_optable[i];
      BoxOpTable4Humans *src = & op_table_for_humans[i];

      dest->name = src->name;
      dest->numargs = src->num_args;
      dest->t_id = My_Type_From_Char(src->arg_type);
      dest->execute = src->executor;
      dest->get_args = My_Executor_From_Str(src->assembler);
      dest->disasm = My_Disassembler_From_Str(src->disassembler);
    }

    the_optable_ptr = the_optable;
    return the_optable_ptr;
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
    oi->signature = My_BoxOpSignature_From_Str(h_ot->assembler);
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
