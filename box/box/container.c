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

#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "mem.h"
#include "print.h"
#include "messages.h"
#include "container.h"

#if 0
static AsmCode asm_code_lea[] = {ASM_LEA_C, ASM_LEA_I, ASM_LEA_R, ASM_LEA_P};

static AsmCode asm_code_mov[] = {ASM_MOV_CC, ASM_MOV_II,
                                 ASM_MOV_RR, ASM_MOV_PP};

static void Prepare_Ptr_Access(const Cont *c) {
  Int ptr_reg = c->value.ptr.reg;
  if (c->categ == CAT_PTR && ptr_reg != 0) {
    Int addr_categ = c->value.ptr.greg ? CAT_GREG : CAT_LREG;
    Cmp_Assemble(ASM_MOV_OO, CAT_LREG, (Int) 0, addr_categ, ptr_reg);
  }
}

void Cont_Move(const Cont *dest, const Cont *src) {
  ContType t = dest->type;
  int src_is_ptr, dest_is_ptr, ro0_conflict, num_ptrs;

  assert(dest->categ != CAT_IMM); /* ...which wouldn't make sense! */

  if (dest->type != src->type) {
    if (dest->type != TYPE_OBJ) {
      MSG_FATAL("Cont_Move: can't move container type=%d to %d!",
                src->type, dest->type);
      return;
    }

    /* Here we must convert a non OBJ container into an OBJ container.
     * This is what could be done in this way:
     */
    MSG_FATAL("Cont_Move: not implemented yet!");
    return;
  }

  src_is_ptr = (src->categ == CAT_PTR);
  dest_is_ptr = (dest->categ == CAT_PTR);
  num_ptrs = src_is_ptr + dest_is_ptr;
  ro0_conflict = num_ptrs > 1 || (num_ptrs == 1 && src->categ == CAT_LREG
                                  && src->value.reg == 0);
  if (ro0_conflict) {
    if (t == TYPE_OBJ) {
      /* Here we use the stack to do the job:
       *  mov ro0, ro1
       *  push o[ro0+8]
       *  mov ro0, ro2
       *  pop o[ro0 + 16]
       */
      Prepare_Ptr_Access(src);
      Cmp_Assemble(ASM_PUSH_O, src->categ, src->value.any_int);
      Prepare_Ptr_Access(dest);
      Cmp_Assemble(ASM_POP_O, dest->categ, dest->value.any_int);
      return;

    } else {
      /* Here we use the zero register to do the job:
       *  mov ro0, ro1
       *  mov ri0, i[ro0+8]
       *  mov ro0, ro2
       *  mov i[ro0 + 16], ri0
       */
      assert(t>=0 && t<TYPE_OBJ);

      Prepare_Ptr_Access(src);
      Cmp_Assemble(asm_code_mov[t], CAT_LREG, (Int) 0,
                   src->categ, src->value.any_int);
      Prepare_Ptr_Access(dest);
      Cmp_Assemble(asm_code_mov[t], dest->categ, dest->value.any_int,
                   CAT_LREG, (Int) 0);
      return;
    }
  }

  /* L'espressione e' un puntatore, allora devo settare il puntatore
   * di riferimento, in modo da realizzare una cosa simile a:
   *   mov ro0, ro1         <-- setto il puntatore di riferimento (ro0)
   *   mov rr1, real[ro0+8] <-- prelevo il valore
   */
  Prepare_Ptr_Access(dest);
  Prepare_Ptr_Access(src);

  if (t != TYPE_OBJ) {
    int is_integer = (t==TYPE_CHAR) || (t==TYPE_INT);
    if (src->categ == CAT_IMM && !is_integer) {
      switch (t) {
       case TYPE_INT:
        Cmp_Assemble(ASM_MOV_Iimm, dest->categ, dest->value.any_int,
                     CAT_IMM, src->value.imm.boxint);
        return;

       case TYPE_REAL:
        Cmp_Assemble(ASM_MOV_Rimm, dest->categ, dest->value.any_int,
                     CAT_IMM, src->value.imm.boxreal);
        return;

       case TYPE_POINT:
        Cmp_Assemble(ASM_MOV_Pimm, dest->categ, dest->value.any_int,
                     CAT_IMM, src->value.imm.boxpoint);
        return;

       default:
        MSG_FATAL("Cont_Move: cannot deal with immediate values "
                  "for type=%d.", t);
        return;
      }

    } else {
      assert(t>=0 && t<TYPE_OBJ);
      Cmp_Assemble(asm_code_mov[t], dest->categ, dest->value.any_int,
                   src->categ, src->value.any_int);
      return;
    }

  } else {
    assert(src->categ != CAT_IMM);
    Cmp_Assemble(ASM_MOV_OO, dest->categ, dest->value.any_int,
                 src->categ, src->value.any_int);
    return;
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
  Prepare_Ptr_Access(src);

  if (src->type != TYPE_OBJ) {
    int t = src->type;
    assert(t>=0 && t<TYPE_OBJ);
    Cmp_Assemble(asm_code_lea[t], src->categ, src->value.any_int);
    Cont_Move(dest, & CONT_NEW_LREG(TYPE_OBJ, 0));
    return;

  } else {
    if (src_is_ptr) {
      if (dest_is_ptr) {
        Cmp_Assemble(ASM_LEA_OO, CAT_LREG, (Int) 0,
                     src->categ, src->value.any_int);
        Cmp_Assemble(ASM_PUSH_O, CAT_LREG, (Int) 0);
        Prepare_Ptr_Access(dest);
        Cmp_Assemble(ASM_POP_O, dest->categ, dest->value.any_int);
        return;

      } else {
        Cmp_Assemble(ASM_LEA_OO, dest->categ, dest->value.any_int,
                     src->categ, src->value.any_int);
        return;
      }

    } else {
      Prepare_Ptr_Access(dest);
      Cmp_Assemble(ASM_MOV_OO, dest->categ, dest->value.any_int,
                   src->categ, src->value.any_int);
      return;
    }
  }
}

void Cont_Ptr_Inc(Cont *ptr, Cont *offset) {
  assert(ptr->categ != CAT_IMM);
  assert(offset->type == TYPE_INT);

  if (offset->categ == CAT_IMM) {
    Int int_offset = offset->value.imm.boxint;
    switch(ptr->categ) {
    case CAT_PTR:
      ptr->value.ptr.offset += int_offset;
      return;

    default:
      assert(ptr->type == TYPE_OBJ);
      ptr->value.ptr.reg = ptr->value.any_int;
      ptr->value.ptr.offset = int_offset;
      ptr->value.ptr.greg = (ptr->categ == CAT_GREG);
      ptr->categ = CAT_PTR;
      return;
    }

  } else {
    MSG_FATAL("Cont_Ptr_Inc: not implemented yet!");
  }
}

void Cont_Ptr_Cast(Cont *ptr, ContType type) {
  assert(ptr->categ != CAT_IMM);
  if (type == TYPE_OBJ) {
    switch(ptr->categ) {
    case CAT_PTR:
      ptr->type = type;
      return;
    default:
      if (ptr->type == type) return;
      MSG_FATAL("Cont_Ptr_Cast: cannot cast type=%I to %I if the container "
                "is not a pointer.", ptr->type, type);
      return;
    }

  } else {
    switch(ptr->categ) {
    case CAT_PTR:
      ptr->type = type;
      return;

    default:
      assert(ptr->type == TYPE_OBJ);
      ptr->categ = CAT_PTR;
      ptr->type = type;
      ptr->value.ptr.reg = ptr->value.reg;
      ptr->value.ptr.offset = 0;
      ptr->value.ptr.greg = (ptr->categ == CAT_GREG);
      return;
    }
  }
}

#endif






/** Convert a container type character to a proper BoxType */
BoxContType BoxContType_From_Char(char type_char) {
  switch(type_char) {
  case 'c': return BOXCONTTYPE_CHAR;
  case 'i': return BOXCONTTYPE_INT;
  case 'r': return BOXCONTTYPE_REAL;
  case 'p': return BOXCONTTYPE_POINT;
  case 'o': return BOXCONTTYPE_PTR;
  default:                                /* error */
    MSG_FATAL("BoxType_From_Char: unrecognized type character '%c'.",
              type_char);
    assert(0);
  }
}

/** Convert a BoxType to a container type character (inverse of function
 * BoxContType_From_Char)
 */
char BoxContType_To_Char(BoxType t) {
  switch(t) {
  case BOXCONTTYPE_CHAR: return 'c';
  case BOXCONTTYPE_INT: return 'i';
  case BOXCONTTYPE_REAL: return 'r';
  case BOXCONTTYPE_POINT: return 'p';
  case BOXCONTTYPE_PTR: return 'o';
  case BOXCONTTYPE_OBJ: return 'o';
  default:
    return '?'; /* error */
    MSG_FATAL("BoxContType_To_Char: unrecognized container type %I.", t);
    assert(0);
  }
}

void BoxCont_Set(BoxCont *c, const char *cont_type, ...) {
  va_list ap; /* to handle the optional arguments (see stdarg.h) */
  BoxContCateg categ;
  BoxContType type;
  enum {READ_CHAR, READ_INT, READ_REAL, READ_POINT, READ_REG,
        READ_PTR, ERROR} action = ERROR;

  assert(strlen(cont_type) >= 2);

  switch(cont_type[1]) {
  case 'c': type = BOXCONTTYPE_CHAR;  action = READ_CHAR;  break; /* Char */
  case 'i': type = BOXCONTTYPE_INT;   action = READ_INT;   break; /* Int */
  case 'r': type = BOXCONTTYPE_REAL;  action = READ_REAL;  break; /* Real */
  case 'p': type = BOXCONTTYPE_POINT; action = READ_POINT; break; /* Point */
  case 'o': type = BOXCONTTYPE_PTR;   action = ERROR;      break; /* Ptr */
  default:                                /* error */
    MSG_FATAL("Cont_Set: unrecognized type for container '%c'.",
              cont_type[1]);
    assert(0);
  }

  switch(cont_type[0]) {
  case 'i': /* immediate */
    categ = BOXCONTCATEG_IMM; break;
  case 'r': /* local reg */
    categ = BOXCONTCATEG_LREG; action = READ_REG; break;
  case 'g': /* global reg */
    categ = BOXCONTCATEG_GREG; action = READ_REG; break;
  case 'p': /* pointer */
    categ = BOXCONTCATEG_PTR;  action = READ_PTR; break;
  default:  /* error */
    MSG_FATAL("Cont_Set: unrecognized container cathegory '%c'.",
              cont_type[0]);
    assert(0);
  }


  c->categ = categ;
  c->type = type;

  va_start(ap, cont_type);

  /* We should not return without calling va_end! */
  switch(action) {
  case READ_CHAR:
    c->value.imm.boxchar = (Char) va_arg(ap, Int); break;
  case READ_INT:
    c->value.imm.boxint = va_arg(ap, Int); break;
  case READ_REAL:
    c->value.imm.boxreal = va_arg(ap, Real); break;
  case READ_POINT:
    c->value.imm.boxpoint = va_arg(ap, Point); break;
  case READ_REG:
    c->value.reg = va_arg(ap, Int); break;
  case READ_PTR:
    c->value.ptr.reg = va_arg(ap, Int);
    c->value.ptr.offset = va_arg(ap, Int);
    c->value.ptr.is_greg = (cont_type[2] == 'g');
    /* ^^^ if strlen(cont_type) == 2, then cont_type[2] will be '\0'.
           that's fine. We'll assume local register in that case. */
    break;
  default:
    assert(0);
  }

  va_end(ap);
}

char *BoxCont_To_String(const BoxCont *c) {
  return Box_SPrintF("%c", BoxContType_To_Char(c->type));
}
