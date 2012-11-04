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

/**
 * @file container.h
 * @brief Generate the assembly code for expression manipulation.
 *
 * Some words.
 */

#ifndef _BOX_CONTAINER_H
#  define _BOX_CONTAINER_H

#  include <box/types.h>


/**
 * @brief Enumeration of possible kinds of operands for Box VM instructions.
 */
typedef enum {
  BOXCONTCATEG_GREG = 0, /**< Global register (shared by all procedures). */
  BOXCONTCATEG_LREG = 1, /**< Local register (local to a procedure). */
  BOXCONTCATEG_PTR  = 2, /**< Pointer to region of memory. */
  BOXCONTCATEG_IMM  = 3  /**< Immediate value (integer, real, etc.) */
} BoxContCateg;

/**
 * @brief Enumeration of the possible types of operands.
 */
typedef enum {
  BOXCONTTYPE_CHAR  = BOXTYPEID_CHAR,
  BOXCONTTYPE_INT   = BOXTYPEID_INT,
  BOXCONTTYPE_REAL  = BOXTYPEID_REAL,
  BOXCONTTYPE_POINT = BOXTYPEID_POINT,
  BOXCONTTYPE_PTR   = BOXTYPEID_PTR,
  BOXCONTTYPE_OBJ   = BOXTYPEID_OBJ,
  BOXCONTTYPE_VOID  = BOXTYPEID_VOID
} BoxContType;

typedef struct BoxCont_struct BoxCont;

struct BoxCont_struct {
  BoxContCateg
              categ;          /**< Category (immediate, register, ptr, etc ) */
  BoxContType type;           /**< Type (CHAR, INT, REAL, ...) */
  union {
    BoxValue  imm;          /**< The value (if immediate) */
    BoxInt    reg;          /**< The register number (if register) */
    struct {
      BoxInt    offset,     /**< Offset from the pointer */
                reg;        /**< Register containing the pointer */
      unsigned int
                is_greg :1; /**< Whether ptr_reg is a global/local register */
    }         ptr;          /**< Data necessary to identify a pointer Value */
  }          value;         /**< Union of all possible vals for a Container */
};

/**
 * Convert a container type character to a proper #BoxType.
 */
BOXEXPORT BoxContType
BoxContType_From_Char(char type_char);

/**
 * Convert a BoxType to a container type character (inverse of function
 * BoxContType_From_Char())
 */
BOXEXPORT char
BoxContType_To_Char(BoxContType t);

/**
 * Function to rapidly set the content of a container.
 * The cathegory and type of the container are specified through the string
 * that the user provides, the following arguments are used to set the value
 * of the container. For example:
 * @code
 *  Cont c;
 *  Cont_Set(& c, "ic", (Char) 'a');  //--> immediate character with value 'a'
 *  Cont_Set(& c, "ii", (Int) 123);   //--> immediate integer with value 123
 *  Cont_Set(& c, "ir", (Real) 1.23); //--> immediate real with value 1.23
 *  Cont_Set(& c, "ri", 2); //--> second local integer register (ri2)
 *  Cont_Set(& c, "rr", 3); //--> third local real register (rr3)
 *  Cont_Set(& c, "go", 1); //--> first global object register (gro1)
 *  Cont_Set(& c, "pc", 3, 8);  //--> character pointed by ro3 + 8 (c[ro3 + 8])
 *  Cont_Set(& c, "pig", 1, 8); //--> integer pointed by gro3 + 8 (i[gro1 + 8])
 * @endcode
 * Note that for pointers, a third character can be used to specify that the
 * base pointer is a global (and not a local register).
 */
BOXEXPORT void
BoxCont_Set(BoxCont *c, const char *cont_type, ...);

/**
 * Return the string representation of the given container.
 */
BOXEXPORT char *
BoxCont_To_String(const BoxCont *c);

#endif /* _BOX_CONTAINER_H */
