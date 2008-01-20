/***************************************************************************
 *   Copyright (C) 2008 by Matteo Franchin                                 *
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

#include <assert.h>

#include "messages.h"
#include "compiler.h"
#include "container.h"

static AsmCode asm_code_lea[] = {ASM_LEA_C, ASM_LEA_I, ASM_LEA_R, ASM_LEA_P};

static AsmCode asm_code_mov[] = {ASM_MOV_CC, ASM_MOV_II,
                                 ASM_MOV_RR, ASM_MOV_PP};


static void prepare_ptr_access(Cont *c) {
  Int ptr_reg = c->ptr_reg;
  if (c->categ == CAT_PTR && ptr_reg != 0) {
    Int addr_categ = c->flags.ptr_is_greg ? CAT_GREG : CAT_LREG;
    Cmp_Assemble(ASM_MOV_OO, CAT_LREG, (Int) 0, addr_categ, c->ptr_reg);
  }
}

void Cont_Move(Cont *dest, Cont *src) {
  ContType t = dest->type;
  int src_is_ptr, dest_is_ptr, ro0_conflict, num_ptrs;

  assert(dest->categ != CAT_IMM); /* ...which wouldn't make sense! */

  if (dest->type != src->type) {
    if (dest->type != TYPE_OBJ) {
      MSG_FATAL("Cont_Mov: can't move container type=%d to %d!",
                src->type, dest->type);
      return;
    }

    /* Here we must convert a non OBJ container into an OBJ container.
     * This is what could be done in this way:
     */
    MSG_FATAL("Cont_Mov: not implemented yet!");
    return;
  }

  src_is_ptr = (src->categ == CAT_PTR);
  dest_is_ptr = (dest->categ == CAT_PTR);
  num_ptrs = src_is_ptr + dest_is_ptr;
  ro0_conflict = num_ptrs > 1 ||
                 (num_ptrs == 1 && src->categ == CAT_LREG && src->reg == 0);
  if (ro0_conflict) {
    if (t == TYPE_OBJ) {
      /* Here we use the stack to do the job:
       *  mov ro0, ro1
       *  push o[ro0+8]
       *  mov ro0, ro2
       *  pop o[ro0 + 16]
       */
      prepare_ptr_access(src);
      Cmp_Assemble(ASM_PUSH_O, src->categ, src->reg);
      prepare_ptr_access(dest);
      Cmp_Assemble(ASM_POP_O, dest->categ, dest->reg);
      return;

    } else {
      /* Here we use the zero register to do the job:
       *  mov ro0, ro1
       *  mov ri0, i[ro0+8]
       *  mov ro0, ro2
       *  mov i[ro0 + 16], ri0
       */
      assert(t>=0 && t<TYPE_OBJ);

      prepare_ptr_access(src);
      Cmp_Assemble(asm_code_mov[t], CAT_LREG, (Int) 0, src->categ, src->reg);
      prepare_ptr_access(dest);
      Cmp_Assemble(asm_code_mov[t], dest->categ, dest->reg, CAT_LREG, (Int) 0);
      return;
    }
  }

  /* L'espressione e' un puntatore, allora devo settare il puntatore
   * di riferimento, in modo da realizzare una cosa simile a:
   *   mov ro0, ro1         <-- setto il puntatore di riferimento (ro0)
   *   mov rr1, real[ro0+8] <-- prelevo il valore
   */
  prepare_ptr_access(dest);
  prepare_ptr_access(src);

  if (t != TYPE_OBJ) {
    int is_integer = (t==TYPE_CHAR) || (t==TYPE_INT);
    if (src->categ == CAT_IMM && !is_integer) {
      switch (t) {
       case TYPE_REAL:
        Cmp_Assemble(ASM_MOV_Rimm, dest->categ, dest->reg,
                     CAT_IMM, *((Real *) src->extra));
        return;

       case TYPE_POINT:
        Cmp_Assemble(ASM_MOV_Pimm, dest->categ, dest->reg,
                     CAT_IMM, *((Point *) src->extra));
        return;

       default:
        MSG_FATAL("Cont_Move: cannot deal with immediate values "
                  "for type=%d.", t);
        return;
      }

    } else {
      assert(t>=0 && t<TYPE_OBJ);
      Cmp_Assemble(asm_code_mov[t], dest->categ, dest->reg,
                   src->categ, src->reg);
      return;
    }

  } else {
    assert(src->categ != CAT_IMM);
    Cmp_Assemble(ASM_MOV_OO, dest->categ, dest->reg, src->categ, src->reg);
    return;

#if 0
    if (dest->categ == CAT_PTR) {
      Cmp_Assemble( ASM_LEA_OO, categ, reg, CAT_PTR, expr->value.i );
      return;

    } else { /* expr->categ == CAT_LREG, CAT_GREG */
      Cmp_Assemble(ASM_MOV_OO, categ, reg, expr->categ, expr->value.i);
      return;
    }
#endif
  }
}

void Cont_Ptr_Create(Cont *dest, Cont *src) {
  int src_is_ptr, dest_is_ptr, num_ptrs;

  assert(dest->categ != CAT_IMM); /* ...which wouldn't make sense! */

  if (src->categ == CAT_IMM) {
    MSG_FATAL("Cont_Ptr_Create: cannot create a pointer out of "
              "an immediate value.");
    return;
  }

  if (dest->type != TYPE_OBJ) {
    MSG_FATAL("Cont_Ptr_Create: cannot store a pointer into a "
              "register of type=%d", dest->type);
    return;
  }

  src_is_ptr = (src->categ == CAT_PTR);
  dest_is_ptr = (dest->categ == CAT_PTR);
  num_ptrs = src_is_ptr + dest_is_ptr;
  prepare_ptr_access(src);

  if (src->type != TYPE_OBJ) {
    int t = src->type;
    assert(t>=0 && t<TYPE_OBJ);
    Cmp_Assemble(asm_code_lea[t], src->categ, src->reg);
    Cont_Move(dest, & CONT_NEW_LREG(TYPE_OBJ, 0));
    return;

  } else {
    if (src_is_ptr) {
      if (dest_is_ptr) {
        Cmp_Assemble(ASM_LEA_OO, CAT_LREG, (Int) 0, src->categ, src->reg);
        Cmp_Assemble(ASM_PUSH_O, CAT_LREG, (Int) 0);
        prepare_ptr_access(dest);
        Cmp_Assemble(ASM_POP_O, dest->categ, dest->reg);
        return;

      } else {
        Cmp_Assemble(ASM_LEA_OO, dest->categ, dest->reg,
                     src->categ, src->reg);
        return;
      }

    } else {
      prepare_ptr_access(dest);
      Cmp_Assemble(ASM_MOV_OO, dest->categ, dest->reg, src->categ, src->reg);
      return;
    }
  }
}
