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
char BoxContType_To_Char(BoxContType t) {
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
    c->value.imm.box_char = (Char) va_arg(ap, Int); break;
  case READ_INT:
    c->value.imm.box_int = va_arg(ap, Int); break;
  case READ_REAL:
    c->value.imm.box_real = va_arg(ap, Real); break;
  case READ_POINT:
    c->value.imm.box_point = va_arg(ap, Point); break;
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
