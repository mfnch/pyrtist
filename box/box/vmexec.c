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
 * @file vmexec.c
 * @brief Implementation of the Box VM instructions.
 *
 * This file contains the implementation of the Box virtual machine
 * instructions.
 */

/* Questo file contiene la funzioni che eseguono le istruzioni della macchina
 * virtuale (spesso denotata con VM).
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "messages.h"
#include "vm_priv.h"
#include "strutils.h"
#include "vmdasm.h"
#include "exception.h"
#include "vmop_priv.h"

/* This array lets us to obtain the size of a type by type index.
 * (Useful in what follows)
 */
const size_t size_of_type[NUM_TYPES] = {
  sizeof(BoxChar),
  sizeof(BoxInt),
  sizeof(BoxReal),
  sizeof(BoxPoint),
  sizeof(BoxPtr)
};

#define MY_COMBINE_CHARS(c1, c2, c3) \
  (((int) (c3)) | (((int) (c2)) << 8) | (((int) (c1)) << 16))

/** Return a BoxOpSignature from the corresponding string representation. */
static BoxOpSignature My_BoxOpSignature_From_Str(const char *s) {
  switch (MY_COMBINE_CHARS(s[0], s[1], s[2])) {
  case MY_COMBINE_CHARS('-', '-', '\0'): return BOXOPSIGNATURE_NONE;
  case MY_COMBINE_CHARS('i', '-', '\0'): return BOXOPSIGNATURE_IMM;
  case MY_COMBINE_CHARS('x', '-', '\0'): return BOXOPSIGNATURE_ANY;
  case MY_COMBINE_CHARS('x', 'i', '\0'): return BOXOPSIGNATURE_ANY_IMM;
  case MY_COMBINE_CHARS('x', 'x', '\0'): return BOXOPSIGNATURE_ANY_ANY;
  default:
    printf("cannot classify '%s'!\n", s);
    assert(0);
  }
}

/*****************************************************************************
 * IMPLEMENTATION OF VM INSTRUCTIONS                                         *
 *****************************************************************************/

static void My_Exec_Ret(BoxVMX *vmx, void *arg1, void *arg2) {
  vmx->flags.exit = 1;
}

static void My_Exec_Call_I(BoxVMX *vmx, void *arg1, void *arg2) {
  if (BoxVM_Module_Execute(vmx->vm, *((BoxInt *) arg1)) == BOXTASK_OK)
    return;
  vmx->flags.error = vmx->flags.exit = 1;
}

static void *My_Exec_X_II(BoxVMX *vmx, int type_id, size_t type_size,
                          size_t *num_regs, BoxInt numvar, BoxInt numreg) {
  BoxInt numtot = numvar + numreg + 1;

  if ((vmx->alc[type_id] & 1) == 0) {
    if(numvar >= 0 && numtot > numvar) {
      void *ptr = calloc(numtot, type_size);
      if (ptr != NULL) {
        BoxVMRegs *regs = & vmx->local[type_id];
        regs->min = -numvar;
        regs->max = numreg;
        regs->ptr = ptr + type_size*numvar;
        vmx->alc[type_id] |= 1;
        if (num_regs)
          *num_regs = numtot;
        return ptr;

      } else
        MSG_FATAL("new(%d): Cannot allocate memory for registers.", type_id);

    } else
      MSG_FATAL("new(%d): Negative arguments.", type_id);

  } else
    MSG_FATAL("new(%d): Double register allocation.", type_id);

  vmx->flags.error = vmx->flags.exit = 1;
  return NULL;
}

static void My_Exec_NewC_II(BoxVMX *vmx, void *arg1, void *arg2) {
  (void) My_Exec_X_II(vmx, BOXTYPEID_CHAR, sizeof(BoxChar), NULL,
                      *((BoxInt *) arg1), *((BoxInt *) arg2));
}

static void My_Exec_NewI_II(BoxVMX *vmx, void *arg1, void *arg2) {
  (void) My_Exec_X_II(vmx, BOXTYPEID_INT, sizeof(BoxInt), NULL,
                      *((BoxInt *) arg1), *((BoxInt *) arg2));
}

static void My_Exec_NewR_II(BoxVMX *vmx, void *arg1, void *arg2) {
  (void) My_Exec_X_II(vmx, BOXTYPEID_REAL, sizeof(BoxReal), NULL,
                      *((BoxInt *) arg1), *((BoxInt *) arg2));
}

static void My_Exec_NewP_II(BoxVMX *vmx, void *arg1, void *arg2) {
  (void) My_Exec_X_II(vmx, BOXTYPEID_POINT, sizeof(BoxPoint), NULL,
                      *((BoxInt *) arg1), *((BoxInt *) arg2));
}

static void My_Exec_NewO_II(BoxVMX *vmx, void *arg1, void *arg2) {
  size_t num_regs, i;
  BoxPtr *regs =
    (BoxPtr *) My_Exec_X_II(vmx, BOXTYPEID_PTR, sizeof(BoxPtr), & num_regs,
                            *((BoxInt *) arg1), *((BoxInt *) arg2));
  if (regs) {
    for(i = 0; i < num_regs; i++)
      BoxPtr_Nullify(regs + i);
  }
}

static void My_Exec_Mov_CC(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxChar *) arg1) = *((BoxChar *) arg2);
}

static void My_Exec_Mov_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) = *((BoxInt *) arg2);
}

static void My_Exec_Mov_RR(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxReal *) arg1) = *((BoxReal *) arg2);
}

static void My_Exec_Mov_PP(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxPoint *) arg1) = *((BoxPoint *) arg2);
}

static void My_Exec_Mov_OO(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxPtr *ptr1 = (BoxPtr *) arg1,
         *ptr2 = (BoxPtr *) arg2;
  if (ptr1 != ptr2) {
    assert(ptr1);
    if (!BoxPtr_Is_Detached(ptr1))
      (void) BoxPtr_Unlink(ptr1);
    *ptr1 = *ptr2;
    BoxPtr_Detach(ptr1);
  }
}

static void My_Exec_BNot_I(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) = ~ *((BoxInt *) arg1);
}

static void My_Exec_BAnd_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) &= *((BoxInt *) arg2);
}

static void My_Exec_BXor_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) ^= *((BoxInt *) arg2);
}

static void My_Exec_BOr_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) |= *((BoxInt *) arg2);
}

static void My_Exec_Shl_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) <<= *((BoxInt *) arg2);
}

static void My_Exec_Shr_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) >>= *((BoxInt *) arg2);
}

static void My_Exec_Inc_I(BoxVMX *vmx, void *arg1, void *arg2) {
  ++*((BoxInt *) arg1);
}

static void My_Exec_Inc_R(BoxVMX *vmx, void *arg1, void *arg2) {
  ++*((BoxReal *) arg1);
}

static void My_Exec_Dec_I(BoxVMX *vmx, void *arg1, void *arg2) {
  --*((BoxInt *) arg1);
}

static void My_Exec_Dec_R(BoxVMX *vmx, void *arg1, void *arg2) {
  --*((BoxReal *) arg1);
}

static void My_Exec_Pow_II(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxInt i, r = 1, b = *((BoxInt *) arg1),
         p = *((BoxInt *) arg2);
  for (i = 0; i < p; i++) r *= b;
  *((BoxInt *) arg1) = r;
}

static void My_Exec_Pow_RR(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxReal *) arg1) = pow(*((BoxReal *) arg1), *((BoxReal *) arg2));
}

static void My_Exec_Add_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) += *((BoxInt *) arg2);
}

static void My_Exec_Add_RR(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxReal *) arg1) += *((BoxReal *) arg2);
}

static void My_Exec_Add_PP(BoxVMX *vmx, void *arg1, void *arg2) {
  ((BoxPoint *) arg1)->x += ((BoxPoint *) arg2)->x;
  ((BoxPoint *) arg1)->y += ((BoxPoint *) arg2)->y;
}

static void My_Exec_Sub_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) -= *((BoxInt *) arg2);
}

static void My_Exec_Sub_RR(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxReal *) arg1) -= *((BoxReal *) arg2);
}

static void My_Exec_Sub_PP(BoxVMX *vmx, void *arg1, void *arg2) {
  ((BoxPoint *) arg1)->x -= ((BoxPoint *) arg2)->x;
  ((BoxPoint *) arg1)->y -= ((BoxPoint *) arg2)->y;
}

static void My_Exec_Mul_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) *= *((BoxInt *) arg2);
}

static void My_Exec_Mul_RR(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxReal *) arg1) *= *((BoxReal *) arg2);
}

static void My_Exec_Div_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) /= *((BoxInt *) arg2);
}

static void My_Exec_Div_RR(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxReal *) arg1) /= *((BoxReal *) arg2);
}

static void My_Exec_Rem_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) %= *((BoxInt *) arg2);
}

static void My_Exec_Neg_I(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) = -*((BoxInt *) arg1);
}

static void My_Exec_Neg_R(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxReal *) arg1) = -*((BoxReal *) arg1);
}

static void My_Exec_Neg_P(BoxVMX *vmx, void *arg1, void *arg2) {
  ((BoxPoint *) arg1)->x = -((BoxPoint *) arg1)->x;
  ((BoxPoint *) arg1)->y = -((BoxPoint *) arg1)->y;
}

static void My_Exec_PMulR_PR(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxReal r = *((BoxReal *) vmx->local[BOXTYPEID_REAL].ptr);
  ((BoxPoint *) arg1)->x *= r;
  ((BoxPoint *) arg1)->y *= r;
}
static void My_Exec_PDivR_PR(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxReal r = *((BoxReal *) vmx->local[BOXTYPEID_REAL].ptr);
  ((BoxPoint *) arg1)->x /= r;
  ((BoxPoint *) arg1)->y /= r;
}

static void My_Exec_Real_C(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxReal *) vmx->local[BOXTYPEID_REAL].ptr) =
    (BoxReal) *((BoxChar *) arg1);
}

static void My_Exec_Real_I(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxReal *) vmx->local[BOXTYPEID_REAL].ptr) =
    (BoxReal) *((BoxInt *) arg1);
}

static void My_Exec_Int_R(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr) =
    (BoxInt) *((BoxReal *) arg1);
}

static void My_Exec_Int_C(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr) =
    (BoxInt) *((BoxChar *) arg1);
}

static void My_Exec_Point_II(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxPoint *p = (BoxPoint *) vmx->local[BOXTYPEID_POINT].ptr;
  p->x = (BoxReal) *((BoxInt *) arg1);
  p->y = (BoxReal) *((BoxInt *) arg2);
}

static void My_Exec_Point_RR(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxPoint *p = (BoxPoint *) vmx->local[BOXTYPEID_POINT].ptr;
  p->x = *((BoxReal *) arg1);
  p->y = *((BoxReal *) arg2);
}

static void My_Exec_ProjX_P(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxReal *) vmx->local[BOXTYPEID_REAL].ptr) =
    ((BoxPoint *) arg1)->x;
}

static void My_Exec_ProjY_P(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxReal *) vmx->local[BOXTYPEID_REAL].ptr) =
    ((BoxPoint *) arg1)->y;
}

static void My_Exec_PPtrX_P(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxPtr *obj = (BoxPtr *) vmx->local[BOXTYPEID_PTR].ptr;
  obj->block = (void *) NULL;
  obj->ptr = & (((BoxPoint *) arg1)->x);
}

static void My_Exec_PPtrY_P(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxPtr *obj = (BoxPtr *) vmx->local[BOXTYPEID_PTR].ptr;
  obj->block = (void *) NULL;
  obj->ptr = & (((BoxPoint *) arg1)->y);
}

static void My_Exec_Eq_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) =
   *((BoxInt *) arg1) == *((BoxInt *) arg2);
}
static void My_Exec_Eq_RR(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr) =
   *((BoxReal *) arg1) == *((BoxReal *) arg2);
}
static void My_Exec_Eq_PP(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr) =
     ( ((BoxPoint *) arg1)->x == ((BoxPoint *) arg2)->x )
  && ( ((BoxPoint *) arg1)->y == ((BoxPoint *) arg2)->y );
}
static void My_Exec_Eq_OO(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr) =
     (((BoxPtr *) arg1)->ptr == ((BoxPtr *) arg2)->ptr);
}
static void My_Exec_Ne_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) =
   *((BoxInt *) arg1) != *((BoxInt *) arg2);
}
static void My_Exec_Ne_RR(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr) =
   *((BoxReal *) arg1) != *((BoxReal *) arg2);
}
static void My_Exec_Ne_PP(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr) =
     ( ((BoxPoint *) arg1)->x != ((BoxPoint *) arg2)->x )
  || ( ((BoxPoint *) arg1)->y != ((BoxPoint *) arg2)->y );
}
static void My_Exec_Ne_OO(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr) =
     (((BoxPtr *) arg1)->ptr != ((BoxPtr *) arg2)->ptr);
}

static void My_Exec_Lt_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) =
   *((BoxInt *) arg1) < *((BoxInt *) arg2);
}
static void My_Exec_Lt_RR(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr) =
   *((BoxReal *) arg1) < *((BoxReal *) arg2);
}
static void My_Exec_Le_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) =
   *((BoxInt *) arg1) <= *((BoxInt *) arg2);
}
static void My_Exec_Le_RR(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr) =
   *((BoxReal *) arg1) <= *((BoxReal *) arg2);
}
static void My_Exec_Gt_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) =
   *((BoxInt *) arg1) > *((BoxInt *) arg2);
}
static void My_Exec_Gt_RR(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr) =
   *((BoxReal *) arg1) > *((BoxReal *) arg2);
}
static void My_Exec_Ge_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) =
   *((BoxInt *) arg1) >= *((BoxInt *) arg2);
}
static void My_Exec_Ge_RR(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr) =
   *((BoxReal *) arg1) >= *((BoxReal *) arg2);
}

static void My_Exec_LNot_I(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) = ! *((BoxInt *) arg1);
}
static void My_Exec_LAnd_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) =
   *((BoxInt *) arg1) && *((BoxInt *) arg2);
}
static void My_Exec_LOr_II(BoxVMX *vmx, void *arg1, void *arg2) {
  *((BoxInt *) arg1) =
   *((BoxInt *) arg1) || *((BoxInt *) arg2);
}

static void My_Exec_Create_I(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxInt id = *((BoxInt *) arg1);
  BoxPtr *obj = (BoxPtr *) vmx->local[BOXTYPEID_PTR].ptr;
  BoxType *t = BoxVM_Get_Installed_Type(vmx->vm, id);
  if (BoxPtr_Create_Obj(obj, t))
    return;
  MSG_FATAL("My_Exec_Create_I: cannot create object with alloc-ID=%I.", id);
}

static void My_Exec_Reloc_OO(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxPtr *dest = (BoxPtr *) arg1,
         *src = (BoxPtr *) arg2;
  BoxInt id = *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr);

  BoxType *t = BoxVM_Get_Installed_Type(vmx->vm, (BoxTypeId) id);
  if (BoxPtr_Copy_Obj(dest, src, t))
    return;

  MSG_FATAL("My_Exec_Reloc_OO: failure copying object with alloc-ID=%I.", id);

  vmx->flags.error = 1;
  vmx->flags.exit = 1;
}

static void My_Exec_Mln_O(BoxVMX *vmx, void *arg1, void *arg2) {
  (void) BoxPtr_Link((BoxPtr *) arg1);
}

static void My_Exec_MUnln_O(BoxVMX *vmx, void *arg1, void *arg2) {
  (void) BoxPtr_Unlink((BoxPtr *) arg1);
  BoxPtr_Detach((BoxPtr *) arg1);
}

static void My_Exec_MCopy_OO(BoxVMX *vmx, void *arg1, void *arg2) {
  (void) memcpy(((BoxPtr *) arg1)->ptr,             /* destination */
                ((BoxPtr *) arg2)->ptr,             /* source */
                *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr)); /* size */
}

static void My_Exec_Shift_OO(BoxVMX *vmx, void *arg1, void *arg2) {
  if (arg1 != arg2) {
    if (!BoxPtr_Is_Detached((BoxPtr *) arg1))
      (void) BoxPtr_Unlink((BoxPtr *) arg1);
    *((BoxPtr *) arg1) = *((BoxPtr *) arg2);
    BoxPtr_Detach((BoxPtr *) arg2);
  }
}

static void My_Exec_Ref_OO(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxPtr *ptr1 = (BoxPtr *) arg1,
         *ptr2 = (BoxPtr *) arg2;
  if (ptr1 != ptr2) {
    assert(ptr1);

    if (!BoxPtr_Is_Detached(ptr1))
      (void) BoxPtr_Unlink(ptr1);

    *ptr1 = *ptr2;
    if (!BoxPtr_Is_Detached(ptr2))
      (void) BoxPtr_Link(ptr2);
  }
}

static void My_Exec_Null_O(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxPtr_Nullify((BoxPtr *) arg1);
}

static void My_Exec_Lea(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxPtr *obj = (BoxPtr *) vmx->local[BOXTYPEID_PTR].ptr;
  obj->block = NULL;
  obj->ptr = arg1;
}

static void My_Exec_Lea_OO(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxPtr *ptr1 = (BoxPtr *) arg1;
  if (!BoxPtr_Is_Detached(ptr1))
    (void) BoxPtr_Unlink(ptr1);

  ptr1->block = (void *) NULL;
  ptr1->ptr = arg2;
}

static void My_Exec_Push_O(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxArr_Push(& vmx->vm->stack, arg1);
}

static void My_Exec_Pop_O(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxArr_Pop(& vmx->vm->stack, arg1);
}

static void My_Exec_Jmp_I(BoxVMX *vmx, void *arg1, void *arg2) {
  vmx->op_size = *((BoxInt *) arg1);
}

static void My_Exec_Jc_I(BoxVMX *vmx, void *arg1, void *arg2) {
  if (*((BoxInt *) vmx->local[BOXTYPEID_INT].ptr))
    vmx->op_size = *((BoxInt *) arg1);
}

/**
 * @brief Instruction <tt>add roA, roB</tt>.
 *
 * The @c add VM instruciton creates a pointer @c roA from an existing pointer
 * @c roB, by translating @c roB by an offset (in byte) stored inside @c ri0.
 */
static void
My_Exec_Add_OO(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxPtr *ptr1 = (BoxPtr *) arg1, *ptr2 = (BoxPtr *) arg2;
  BoxInt offset = *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr);

  if (!BoxPtr_Is_Detached(ptr1))
    (void) BoxPtr_Unlink(ptr1);

  ptr1->block = ptr2->block;
  ptr1->ptr = ptr2->ptr + offset;
  (void) BoxPtr_Link(ptr1);
}

/**
 * @brief Instruction <tt>typeof int</tt>.
 *
 * <tt>typeof [stuff]</tt> is equivalent to <tt>mov ri0, [stuff]</tt>.
 * This instruction is used to load the type identifier in the register @c ro0
 * before calling other instructions such as @c create, @c reloc, @c box, etc.
 */
static void
My_Exec_Typeof_I(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxInt *ri0 = (BoxInt *) vmx->local[BOXTYPEID_INT].ptr;
  *ri0 = *((BoxInt *) arg1);
 }

/**
 * @brief Instruction <tt>box dstptr</tt>.
 *
 * This short version of the @c box VM instruction setups the Any object in
 * @c srcptr to point to a zero-size object with type stored in @c ro0.
 */
static void
My_Exec_Box_O(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxInt ri0 = *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr);
  BoxType *t = BoxVM_Get_Installed_Type(vmx->vm, ri0);
  BoxPtr *dst = (BoxPtr *) arg1;

  if (t) {
    BoxPtr src;
    BoxPtr_Nullify(& src);
    if (BoxAny_Box(dst, & src, t, BOXBOOL_FALSE))
      return;
    BoxVM_Set_Fail_Msg(vmx->vm, "Boxing operation failed");
  } else
    BoxVM_Set_Fail_Msg(vmx->vm, "Anomalous type in boxing operation");

  vmx->flags.error = vmx->flags.exit = 1;
}

/**
 * @brief Instruction <tt>box dstptr, srcptr</tt>.
 *
 * The @c box VM instruction converts the source object @c srcptr into a
 * destination @c ANY object in @c dstptr using the type in @c ro0.
 * Note that @c dstptr must be a pointer to an initialized @c ANY object.
 */
static void
My_Exec_Box_OO(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxInt ri0 = *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr);
  BoxType *t = BoxVM_Get_Installed_Type(vmx->vm, ri0);
  BoxPtr *dst = (BoxPtr *) arg1,
         *src = (BoxPtr *) arg2;

  if (t) {
    if (BoxAny_Box(dst, src, t, BOXBOOL_TRUE))
      return;
    BoxVM_Set_Fail_Msg(vmx->vm, "Boxing operation failed");
  } else
    BoxVM_Set_Fail_Msg(vmx->vm, "Anomalous type in boxing operation");

  vmx->flags.error = vmx->flags.exit = 1;
}

/**
 * @brief Instruction <tt>wbox dstptr, srcptr</tt>.
 *
 * The @c wbox VM instruction converts the source object @c srcptr into a
 * destination @c ANY object in @c dstptr using the type in @c ro0.
 * Note that @c dstptr must be a pointer to an initialized @c ANY object.
 * Contrary to the @c box instruction, @c wbox does not copy NULL-block
 * objects. This means that an object boxed with @c wbox should not escape
 * the current execution unit.
 */
static void
My_Exec_WBox_OO(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxInt ri0 = *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr);
  BoxType *t = BoxVM_Get_Installed_Type(vmx->vm, ri0);
  BoxPtr *dst = (BoxPtr *) arg1,
         *src = (BoxPtr *) arg2;

  if (t) {
    if (BoxAny_Box(dst, src, t, BOXBOOL_FALSE))
      return;
    BoxVM_Set_Fail_Msg(vmx->vm, "Weak boxing operation failed");
  } else
    BoxVM_Set_Fail_Msg(vmx->vm, "Anomalous type in boxing operation");

  vmx->flags.error = vmx->flags.exit = 1;
}

/**
 * @brief Instruction <tt>unbox dstptr, srcptr</tt>.
 *
 * The @c unbox VM instruction converts the source @c ANY object @c srcptr into
 * a destination object in @c dstptr of type @c ro0.
 * Note that the unboxing operation fails (raising an exception) if the type
 * of the boxed object is not consistent with what given in @c ro0.
 */
static void My_Exec_Unbox_OO(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxInt ri0 = *((BoxInt *) vmx->local[BOXTYPEID_INT].ptr);
  BoxType *t = BoxVM_Get_Installed_Type(vmx->vm, ri0);
  BoxPtr *dst = (BoxPtr *) arg1,
         *src = (BoxPtr *) arg2;

  if (t) {
    if (BoxAny_Unbox(dst, src, t))
      return;
    BoxVM_Set_Fail_Msg(vmx->vm, "Unboxing operation failed");
  } else
    BoxVM_Set_Fail_Msg(vmx->vm, "Anomalous type in unboxing operation");

  vmx->flags.error = vmx->flags.exit = 1;  
}

/**
 * @brief Instruction <tt>dycall parentany, childany</tt>.
 *
 * The @c dycall VM instruction allows calling a combination from two any
 * objects, thus effectively performing a dynamic call. The function uses the
 * type system to find an appropriate combination and calls it, raising an
 * exception if the combination is not found or if an exception was raised by
 * the called combination.
 */
static void My_Exec_Dycall_OO(BoxVMX *vmx, void *arg1, void *arg2) {
  BoxAny *parent = BoxPtr_Get_Target((BoxPtr *) arg1),
         *child = BoxPtr_Get_Target((BoxPtr *) arg2);
  BoxException *exception;
  if (Box_Combine_Any(parent, child, & exception)) {
    if (!exception)
      return;
    else {
      char *msg = BoxException_Get_Str(exception);
      BoxVM_Set_Fail_Msg(vmx->vm, msg);
      Box_Mem_Free(msg);
      BoxException_Destroy(exception);
    }

  } else {
    BoxType *t_parent = (parent) ? BoxAny_Get_Type(parent) : NULL;
    BoxType *t_child = (child) ? BoxAny_Get_Type(child) : NULL;
    char *s_parent = ((t_parent) ?
                      BoxType_Get_Repr(t_parent) : Box_Mem_Strdup("null"));
    char *s_child = ((t_child) ?
                      BoxType_Get_Repr(t_child) : Box_Mem_Strdup("null"));
    char *msg = Box_SPrintF("Cannot find combination %~s@%~s",
                            s_child, s_parent);
    BoxVM_Set_Fail_Msg(vmx->vm, msg);
    Box_Mem_Free(msg);
  }

  vmx->flags.error = vmx->flags.exit = 1;  
}


typedef struct {
  BoxGOp         g_opcode;        /**< Generic Opcode */
  const char     *name;           /**< Name of the operation */
  char           num_args;        /**< Number of explicit arguments */
  char           arg_type;        /**< Type of the explicit arguments */
  const char     *input_regs,     /**< List of */
                 *output_regs,    /**<*/
                 *assembler,      /**<*/
                 *disassembler;   /**<*/
  BoxVMOpExecutor executor;       /**< Instruction executor. */
} BoxOpTable4Humans;

static BoxOpTable4Humans op_table_for_humans[] = {
  {  BOXGOP_CALL,   "call", 1, 'i',     "a1",  NULL, "x-", "c-", My_Exec_Call_I   }, /* call ri       */
  {  BOXGOP_CALL,   "call", 1, 'i',     "a1",  NULL, "i-", "c-", My_Exec_Call_I   }, /* call ii       */
  {  BOXGOP_NEWC,   "newc", 2, 'i',  "a1,a2",  NULL, "xx", "xx", My_Exec_NewC_II  }, /* newc ii, ii   */
  {  BOXGOP_NEWC,   "newi", 2, 'i',  "a1,a2",  NULL, "xx", "xx", My_Exec_NewI_II  }, /* newi ii, ii   */
  {  BOXGOP_NEWC,   "newr", 2, 'i',  "a1,a2",  NULL, "xx", "xx", My_Exec_NewR_II  }, /* newr ii, ii   */
  {  BOXGOP_NEWC,   "newp", 2, 'i',  "a1,a2",  NULL, "xx", "xx", My_Exec_NewP_II  }, /* newp ii, ii   */
  {  BOXGOP_NEWC,   "newo", 2, 'i',  "a1,a2",  NULL, "xx", "xx", My_Exec_NewO_II  }, /* newo ii, ii   */
  {   BOXGOP_MOV,   "move", 2, 'c',     "a2",  "a1", "xi", "xi", My_Exec_Mov_CC   }, /* mov rc, ic    */
  {   BOXGOP_MOV,   "move", 2, 'i',     "a2",  "a1", "xi", "xi", My_Exec_Mov_II   }, /* mov ri, ii    */
  {   BOXGOP_MOV,   "move", 2, 'r',     "a2",  "a1", "xi", "xi", My_Exec_Mov_RR   }, /* mov rr, ir    */
  {   BOXGOP_MOV,   "move", 2, 'p',     "a2",  "a1", "xi", "xi", My_Exec_Mov_PP   }, /* mov rp, ip    */
  {   BOXGOP_MOV,   "move", 2, 'c',     "a2",  "a1", "xx", "xx", My_Exec_Mov_CC   }, /* mov rc, rc    */
  {   BOXGOP_MOV,   "move", 2, 'i',     "a2",  "a1", "xx", "xx", My_Exec_Mov_II   }, /* mov ri, ri    */
  {   BOXGOP_MOV,   "move", 2, 'r',     "a2",  "a1", "xx", "xx", My_Exec_Mov_RR   }, /* mov rr, rr    */
  {   BOXGOP_MOV,   "move", 2, 'p',     "a2",  "a1", "xx", "xx", My_Exec_Mov_PP   }, /* mov rp, rp    */
  {   BOXGOP_MOV,   "move", 2, 'o',     "a2",  "a1", "xx", "xx", My_Exec_Mov_OO   }, /* mov ro, ro    */
  {  BOXGOP_BNOT,   "bnot", 1, 'i',     "a1",  "a1", "x-", "xx", My_Exec_BNot_I   }, /* bnot ri       */
  {  BOXGOP_BAND,   "band", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_BAnd_II  }, /* band ri, ri   */
  {  BOXGOP_BXOR,   "bxor", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_BXor_II  }, /* bxor ri, ri   */
  {   BOXGOP_BOR,    "bor", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_BOr_II   }, /* bor ri, ri    */
  {   BOXGOP_SHL,    "shl", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_Shl_II   }, /* shl ri, ri    */
  {   BOXGOP_SHR,    "shr", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_Shr_II   }, /* shr ri, ri    */
  {   BOXGOP_INC,    "inc", 1, 'i',     "a1",  "a1", "x-", "xx", My_Exec_Inc_I    }, /* inc ri        */
  {   BOXGOP_INC,    "inc", 1, 'r',     "a1",  "a1", "x-", "xx", My_Exec_Inc_R    }, /* inc rr        */
  {   BOXGOP_DEC,    "dec", 1, 'i',     "a1",  "a1", "x-", "xx", My_Exec_Dec_I    }, /* dec ri        */
  {   BOXGOP_DEC,    "dec", 1, 'r',     "a1",  "a1", "x-", "xx", My_Exec_Dec_R    }, /* dec rr        */
  {   BOXGOP_POW,    "pow", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_Pow_II   }, /* pow ri, ri    */
  {   BOXGOP_POW,    "pow", 2, 'r',  "a1,a2",  "a1", "xx", "xx", My_Exec_Pow_RR   }, /* pow rr, rr    */
  {   BOXGOP_ADD,    "add", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_Add_II   }, /* add ri, ri    */
  {   BOXGOP_ADD,    "add", 2, 'r',  "a1,a2",  "a1", "xx", "xx", My_Exec_Add_RR   }, /* add rr, rr    */
  {   BOXGOP_ADD,    "add", 2, 'p',  "a1,a2",  "a1", "xx", "xx", My_Exec_Add_PP   }, /* add rp, rp    */
  {   BOXGOP_ADD,    "add", 2, 'o', "ri0,a2",  "a1", "xx", "xx", My_Exec_Add_OO   }, /* add ro, ro    */
  {   BOXGOP_SUB,    "sub", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_Sub_II   }, /* sub ri, ri    */
  {   BOXGOP_SUB,    "sub", 2, 'r',  "a1,a2",  "a1", "xx", "xx", My_Exec_Sub_RR   }, /* sub rr, rr    */
  {   BOXGOP_SUB,    "sub", 2, 'p',  "a1,a2",  "a1", "xx", "xx", My_Exec_Sub_PP   }, /* sub rp, rp    */
  {   BOXGOP_MUL,    "mul", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_Mul_II   }, /* mul ri, ri    */
  {   BOXGOP_MUL,    "mul", 2, 'r',  "a1,a2",  "a1", "xx", "xx", My_Exec_Mul_RR   }, /* mul rr, rr    */
  {   BOXGOP_DIV,    "div", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_Div_II   }, /* div ri, ri    */
  {   BOXGOP_DIV,    "div", 2, 'r',  "a1,a2",  "a1", "xx", "xx", My_Exec_Div_RR   }, /* div rr, rr    */
  {   BOXGOP_REM,    "rem", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_Rem_II   }, /* rem ri, ri    */
  {   BOXGOP_NEG,    "neg", 1, 'i',     "a1",  "a1", "x-", "xx", My_Exec_Neg_I    }, /* neg ri        */
  {   BOXGOP_NEG,    "neg", 1, 'r',     "a1",  "a1", "x-", "xx", My_Exec_Neg_R    }, /* neg rr        */
  {   BOXGOP_NEG,    "neg", 1, 'p',     "a1",  "a1", "x-", "xx", My_Exec_Neg_P    }, /* neg rp        */
  { BOXGOP_PMULR,  "pmulr", 1, 'p', "a1,rr0",  "a1", "x-", "xx", My_Exec_PMulR_PR }, /* pmulr rp      */
  { BOXGOP_PDIVR,  "pdivr", 1, 'p', "a1,rr0",  "a1", "x-", "xx", My_Exec_PDivR_PR }, /* pdivr rp      */
  {    BOXGOP_EQ,    "eq?", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_Eq_II    }, /* eq? ri, ri    */
  {    BOXGOP_EQ,    "eq?", 2, 'r',  "a1,a2", "ri0", "xx", "xx", My_Exec_Eq_RR    }, /* eq? rr, rr    */
  {    BOXGOP_EQ,    "eq?", 2, 'p',  "a1,a2", "ri0", "xx", "xx", My_Exec_Eq_PP    }, /* eq? rp, rp    */
  {    BOXGOP_EQ,    "eq?", 2, 'o',  "a1,a2", "ri0", "xx", "xx", My_Exec_Eq_OO    }, /* eq? ro, ro    */
  {    BOXGOP_NE,    "ne?", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_Ne_II    }, /* ne? ri, ri    */
  {    BOXGOP_NE,    "ne?", 2, 'r',  "a1,a2", "ri0", "xx", "xx", My_Exec_Ne_RR    }, /* ne? rr, rr    */
  {    BOXGOP_NE,    "ne?", 2, 'p',  "a1,a2", "ri0", "xx", "xx", My_Exec_Ne_PP    }, /* ne? rp, rp    */
  {    BOXGOP_NE,    "ne?", 2, 'o',  "a1,a2", "ri0", "xx", "xx", My_Exec_Ne_OO    }, /* ne? rp, rp    */
  {    BOXGOP_LT,    "lt?", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_Lt_II    }, /* lt? ri, ri    */
  {    BOXGOP_LT,    "lt?", 2, 'r',  "a1,a2", "ri0", "xx", "xx", My_Exec_Lt_RR    }, /* lt? rr, rr    */
  {    BOXGOP_LE,    "le?", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_Le_II    }, /* le? ri, ri    */
  {    BOXGOP_LE,    "le?", 2, 'r',  "a1,a2", "ri0", "xx", "xx", My_Exec_Le_RR    }, /* le? rr, rr    */
  {    BOXGOP_GT,    "gt?", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_Gt_II    }, /* gt? ri, ri    */
  {    BOXGOP_GT,    "gt?", 2, 'r',  "a1,a2", "ri0", "xx", "xx", My_Exec_Gt_RR    }, /* gt? rr, rr    */
  {    BOXGOP_GE,    "ge?", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_Ge_II    }, /* ge? ri, ri    */
  {    BOXGOP_GE,    "ge?", 2, 'r',  "a1,a2", "ri0", "xx", "xx", My_Exec_Ge_RR    }, /* ge? rr, rr    */
  {  BOXGOP_LNOT,   "lnot", 1, 'i',     "a1",  "a1", "x-", "xx", My_Exec_LNot_I   }, /* lnot ri       */
  {  BOXGOP_LAND,   "land", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_LAnd_II  }, /* land ri, ri   */
  {   BOXGOP_LOR,    "lor", 2, 'i',  "a1,a2",  "a1", "xx", "xx", My_Exec_LOr_II   }, /* lor  ri, ri   */
  {  BOXGOP_REAL,   "real", 1, 'c',     "a1", "rr0", "x-", "xx", My_Exec_Real_C   }, /* real rc       */
  {  BOXGOP_REAL,   "real", 1, 'i',     "a1", "rr0", "x-", "xx", My_Exec_Real_I   }, /* real ri       */
  {   BOXGOP_INT,    "int", 1, 'c',     "a1", "ri0", "x-", "xx", My_Exec_Int_C    }, /* int rc        */
  {   BOXGOP_INT,    "int", 1, 'r',     "a1", "ri0", "x-", "xx", My_Exec_Int_R    }, /* int rr        */
  { BOXGOP_POINT,  "point", 2, 'i',  "a1,a2", "rp0", "xx", "xx", My_Exec_Point_II }, /* point ri, ri  */
  { BOXGOP_POINT,  "point", 2, 'r',  "a1,a2", "rp0", "xx", "xx", My_Exec_Point_RR }, /* point rr, rr  */
  { BOXGOP_PROJX,  "projx", 1, 'p',     "a1", "rr0", "x-", "xx", My_Exec_ProjX_P  }, /* projx rp      */
  { BOXGOP_PROJY,  "projy", 1, 'p',     "a1", "rr0", "x-", "xx", My_Exec_ProjY_P  }, /* projy rp      */
  { BOXGOP_PPTRX,  "pptrx", 1, 'p',     "a1", "ro0", "x-", "xx", My_Exec_PPtrX_P  }, /* pptrx rp      */
  { BOXGOP_PPTRY,  "pptry", 1, 'p',     "a1", "ro0", "x-", "xx", My_Exec_PPtrY_P  }, /* pptry rp      */
  {   BOXGOP_RET, "return", 0, 'n',     NULL,  NULL, "--", "xx", My_Exec_Ret      }, /* ret           */
  {BOXGOP_CREATE, "create", 1, 'i',     "a1", "ro0", "x-", "xx", My_Exec_Create_I }, /* create ri     */
  {   BOXGOP_MLN,    "mln", 1, 'o',     "a1",  NULL, "x-", "xx", My_Exec_Mln_O    }, /* mln ro        */
  { BOXGOP_MUNLN,  "munln", 1, 'o',     "a1",  NULL, "x-", "xx", My_Exec_MUnln_O  }, /* munln ro      */
  { BOXGOP_MCOPY,  "mcopy", 2, 'o',
                                 "a1,a2,ri0",  NULL, "xx", "xx", My_Exec_MCopy_OO }, /* mcopy ro, ro  */
  { BOXGOP_RELOC,  "reloc", 2, 'o',
                                 "a1,a2,ri0",  NULL, "xx", "xx", My_Exec_Reloc_OO }, /* reloc ro, ro    */
  { BOXGOP_SHIFT,  "shift", 2, 'o',     "a2",  "a1", "xx", "xx", My_Exec_Shift_OO }, /* shift ro, ro    */
  {   BOXGOP_REF,    "ref", 2, 'o',     "a2",  "a1", "xx", "xx", My_Exec_Ref_OO   }, /* ref ro, ro      */
  {  BOXGOP_NULL,   "null", 1, 'o',     NULL,  "a1", "x-", "xx", My_Exec_Null_O   }, /* null ro         */
  {   BOXGOP_LEA,    "lea", 1, 'c',     "a1", "ro0", "x-", "xx", My_Exec_Lea      }, /* lea c[ro0+...]  */
  {   BOXGOP_LEA,    "lea", 1, 'i',     "a1", "ro0", "x-", "xx", My_Exec_Lea      }, /* lea i[ro0+...]  */
  {   BOXGOP_LEA,    "lea", 1, 'r',     "a1", "ro0", "x-", "xx", My_Exec_Lea      }, /* lea r[ro0+...]  */
  {   BOXGOP_LEA,    "lea", 1, 'p',     "a1", "ro0", "x-", "xx", My_Exec_Lea      }, /* lea p[ro0+...]  */
  {   BOXGOP_LEA,    "lea", 2, 'o',     "a2",  "a1", "xx", "xx", My_Exec_Lea_OO   }, /* lea reg_o, o[ro0+...] */
  {  BOXGOP_PUSH,   "push", 1, 'o',     "a1",  NULL, "x-", "xx", My_Exec_Push_O   }, /* push ro         */
  {   BOXGOP_POP,    "pop", 1, 'o',     NULL,  "a1", "x-", "xx", My_Exec_Pop_O    }, /* pop ro          */
  {   BOXGOP_JMP,    "jmp", 1, 'i',     "a1",  NULL, "x-", "j-", My_Exec_Jmp_I    }, /* jmp ri          */
  {    BOXGOP_JC,     "jc", 1, 'i', "a1,ri0",  NULL, "x-", "j-", My_Exec_Jc_I     }, /* jc  ri          */

  {BOXGOP_TYPEOF, "typeof", 1, 'i',     "a1", "ri0", "x-", "xx", My_Exec_Typeof_I }, /* typeof reg_i        */
  {   BOXGOP_BOX,    "box", 1, 'o',    "ri0",  "a1", "x-", "xx", My_Exec_Box_O    }, /* box reg_o           */
  {   BOXGOP_BOX,    "box", 2, 'o', "a2,ri0",  "a1", "xx", "xx", My_Exec_Box_OO   }, /* box reg_o, reg_o    */
  {  BOXGOP_WBOX,   "wbox", 2, 'o', "a2,ri0",  "a1", "xx", "xx", My_Exec_WBox_OO  }, /* wbox reg_o, reg_o   */
  { BOXGOP_UNBOX,  "unbox", 2, 'o', "a2,ri0",  "a1", "xx", "xx", My_Exec_Unbox_OO }, /* unbox reg_o, reg_o  */
  {BOXGOP_DYCALL, "dycall", 2, 'o',     "a2",  "a1", "xx", "xx", My_Exec_Dycall_OO}  /* dycall reg_o, reg_o */
};


BoxTypeId My_Type_From_Char(char c) {
  switch(c) {
  case 'n': return BOXTYPEID_NONE; break;
  case 'c': return BOXTYPEID_CHAR; break;
  case 'i': return BOXTYPEID_INT; break;
  case 'r': return BOXTYPEID_REAL; break;
  case 'p': return BOXTYPEID_POINT; break;
  case 'o': return BOXTYPEID_PTR; break;
  default:
    MSG_FATAL("My_Type_From_Char: unknown type char '%c'", c);
    assert(0);
    return BOXTYPEID_NONE;
  }
}

const BoxOpDesc *BoxVM_Get_Exec_Table(void) {
  static BoxOpDesc the_optable[BOX_NUM_OPS],
                   *the_optable_ptr = NULL;

  if (the_optable_ptr != NULL)
    return the_optable_ptr;

  else {
    int i;

    for(i = 0; i < BOX_NUM_OPS; i++) {
      BoxOpDesc *dest = & the_optable[i];
      BoxOpTable4Humans *src = & op_table_for_humans[i];
      BoxOpSignature sig = My_BoxOpSignature_From_Str(src->assembler);

      dest->name = src->name;
      dest->numargs = src->num_args;
      dest->t_id = My_Type_From_Char(src->arg_type);
      dest->execute = src->executor;

      switch(sig) {
      case BOXOPSIGNATURE_NONE: dest->num_args = 0; dest->has_data = 0; break;
      case BOXOPSIGNATURE_IMM:  dest->num_args = 0; dest->has_data = 1; break;
      case BOXOPSIGNATURE_ANY:  dest->num_args = 1; dest->has_data = 0; break;
      case BOXOPSIGNATURE_ANY_IMM: dest->num_args = 1; dest->has_data = 1; break;
      case BOXOPSIGNATURE_ANY_ANY: dest->num_args = 2; dest->has_data = 0; break;
      default:
        abort();
      }
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

  reg = ot->regs = Box_Mem_Safe_Alloc(sizeof(BoxOpReg)*num_regs_to_alloc);

  for(i = 0; i < BOX_NUM_OPS; i++) {
    BoxOpInfo *oi = & ot->info[i];
    BoxOpTable4Humans *h_ot;
    const char *token;
    int num_regs, num_out_regs;

    assert(oi->name);
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
  Box_Mem_Free(ot->regs);
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
