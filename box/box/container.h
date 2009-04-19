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

/** @file container.h
 * @brief Generate the assembly code for expression manipulation.
 *
 * Some words.
 */

#ifndef _CONTAINER_H
#  define _CONTAINER_H

#  include "types.h"
#  include "virtmach.h"

/* These types for now are just duplicates of the types defined in virtmach.h
 * This has to be seen as a first attempt of create an interface for code
 * generation which is unbound to the target machine, even if for now we
 * support as a target only the box vm.
 *
 * IMPORTANT: the user who calls the ``Cont_*'' routines is expected to use
 *   the following enumerated constants (CONT_*), EVEN IF the file
 *   'container.c' uses the VM native names (CAT_GREG, CAT_LREG, etc.).
 *   Indeed the following declarations must be seen as a generic interface
 *   specification, while the implementation in 'container.c' refers
 *   specifically to the box virtual machine and its instruction set.
 */
typedef enum {
  CONT_GREG = CAT_GREG,
  CONT_LREG = CAT_LREG,
  CONT_PTR = CAT_PTR,
  CONT_IMM = CAT_IMM
} ContCateg;

typedef enum {
  BOXCONTCATEG_GREG = CAT_GREG,
  BOXCONTCATEG_LREG = CAT_LREG,
  BOXCONTCATEG_PTR = CAT_PTR,
  BOXCONTCATEG_IMM = CAT_IMM
} BoxContCateg;

typedef enum {
  CONT_CHAR  = BOXTYPE_CHAR,
  CONT_INT   = BOXTYPE_INT,
  CONT_REAL  = BOXTYPE_REAL,
  CONT_POINT = BOXTYPE_POINT,
  CONT_OBJ   = TYPE_OBJ
} ContType;

typedef enum {
  BOXCONTTYPE_CHAR  = BOXTYPE_CHAR,
  BOXCONTTYPE_INT   = BOXTYPE_INT,
  BOXCONTTYPE_REAL  = BOXTYPE_REAL,
  BOXCONTTYPE_POINT = BOXTYPE_POINT,
  BOXCONTTYPE_OBJ   = TYPE_OBJ
} BoxContType;

typedef struct {
  ContCateg  categ;       /**< Cathegory (immediate, register, pointer, ...)*/
  ContType   type;        /**< Type (CHAR, INT, REAL, ...) */
  union {
    BoxValue   imm;       /**< The value (if immediate) */
    BoxInt     reg,       /**< The register number (if register) */
               any_int;   /**< Used for integers, when the specific case is
                               not known (imm.boxint, reg or ptr.offset?) */
    struct {
      BoxInt     offset,  /**< Offset from the pointer */
                 reg;     /**< Register containing the pointer */
      unsigned int
                 greg :1; /**< Whether ptr_reg is a global/local register */
    }          ptr;       /**< Data necessary to identify a pointer Value */
  }          value;       /**< Union of all possible values for a Container */
} Cont;

typedef struct {
  ContCateg categ;          /**< Cathegory (immediate, register, ptr, etc ) */
  ContType  type;           /**< Type (CHAR, INT, REAL, ...) */
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
} BoxCont;

#if 0
#define CONT_NEW_LREG(type, num) \
  ((Cont) {CONT_LREG, (type), (num)})

#define CONT_NEW_LPTR(type, ptr_reg, offset) \
  ((Cont) {CONT_PTR, (type), (offset), (ptr_reg), (void *) 0, {0}})

#define CONT_NEW_INT(value) ((Cont) {CONT_IMM, CONT_INT, (value)})
#else
#define CONT_NEW_LREG(type, num) ((Cont) {})

#define CONT_NEW_LPTR(type, ptr_reg, offset) ((Cont) {})

#define CONT_NEW_INT(value) ((Cont) {})
#endif


/** Function to rapidly set the content of a container.
 * The cathegory and type of the container are specified through the string
 * that the user provides, the following arguments are used to set the value
 * of the container. For example:
 *
 *  Cont c;
 *  Cont_Set(& c, "ic", (Char) 'a');  --> immediate character with value 'a'
 *  Cont_Set(& c, "ii", (Int) 123);   --> immediate integer with value 123
 *  Cont_Set(& c, "ir", (Real) 1.23); --> immediate real with value 1.23
 *  Cont_Set(& c, "ri", 2); --> second local integer register (ri2)
 *  Cont_Set(& c, "rr", 3); --> third local real register (rr3)
 *  Cont_Set(& c, "go", 1); --> first global object register (gro1)
 *  Cont_Set(& c, "pc", 3, 8);  --> character pointed by ro3 + 8 (c[ro3 + 8])
 *  Cont_Set(& c, "pig", 1, 8); --> integer pointed by gro3 + 8 (i[gro1 + 8])
 *
 * Note that for pointers, a third character can be used to specify that the
 * base pointer is a global (and not a local register).
 */
void BoxCont_Set(BoxCont *c, const char *cont_type, ...);

/** Move the content of container 'src' to the container 'dest'.
 */
void Cont_Move(const Cont *dest, const Cont *src);

/** Create in 'dest' a pointer to the content of the container 'src'.
 * Consequently 'dest' has to have type==CONT_OBJ.
 */
void Cont_Ptr_Create(Cont *dest, Cont *src);

/** Increase the pointer container *ptr by the integer value stored
 * inside the container *offset. This can be both an immediate integer
 * or a integer register.
 */
void Cont_Ptr_Inc(Cont *ptr, Cont *offset);

/** Change the type of a container when possible (this operation does
 * not produce any output code).
 */
void Cont_Ptr_Cast(Cont *ptr, ContType type);

#endif
