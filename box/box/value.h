/****************************************************************************
 * Copyright (C) 2009 by Matteo Franchin                                    *
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

/** @file value.h
 * @brief Declaration Value, the central object in the Box compiler.
 *
 * A value object is used by the Box compiler to represent values.
 * It contains information about the actual register (global or local)
 * used to store/manage the object and about the type and the attributes
 * of the value. Integers, reals, etc. (such as '1', '1.23', ...) are translated
 * into Value objects. Variables (such as 'var1', ...) are translated into
 * Value objects. And many operations (such as the extraction of a member of
 * a structure, 'var1.member') are expressed in terms of Value objects.
 */

#ifndef _VALUE_H
#  define _VALUE_H

#  include "types.h"
#  include "container.h"

typedef enum {
  VALUEKIND_ERR,              /**< An error */
  VALUEKIND_IMM,              /**< A constant numerical value */
  VALUEKIND_TEMP,             /**< A temporary value */
  VALUEKIND_TARGET            /**< A target value (lvalue) */
} ValueKind;

typedef struct {
  int       num_ref;          /**< Number of references to this Value */
  BoxType   type;             /**< Type of the Value */
  BoxCont   cont;             /**< Value Container */
  ValueKind kind;             /**< Kind of Value */
  struct {
    unsigned int
            new_or_init   :1, /**< Created with Value_New? (or Value_Init?) */
            own_register  :1, /**< Need to release a register during
                                   finalisation? */
            own_reference :1, /**< Do we own a reference to the object? */
            ignore        :1; /**< Should be ignored when passed to a Box. */
  }         attr;             /**< Attributes for the Value */
} Value;

/** Initialise a Value 'v' assuming 'v' points to an already allocated memory
 * region able to contain a Value object.
 */
void Value_Init(Value *v);

/** Allocate a Value object and initialise it by calling Value_Init.
 * Notice that the object is aware of its allocation mode (whether it was
 * created with Value_Init or Value_New) and Value_Unlink will correctly
 * call a free() in case Value_New was used. free() won't be called if the
 * object was created with Value_Init.
 */
Value *Value_New(void);

/** Remove one reference to the Value object 'v', destroying it if there are
 * no more reference to the Value. The object is destroyed coherently to how
 * it was created (Value_New or Value_Init). If Value_New was used, then the
 * object is freed. If Value_Init was used free() is not called.
 */
void Value_Unlink(Value *v);

/** Add a reference to the specified Value. */
void Value_Link(Value *v);

typedef enum {
  VALCONTTYPE_IMM = 0,
  VALCONTTYPE_LREG, VALCONTTYPE_LVAR,
  VALCONTTYPE_GREG, VALCONTTYPE_GVAR,
  VALCONTTYPE_GPTR,
  VALCONTTYPE_LRPTR, VALCONTTYPE_LVPTR,
  VALCONTTYPE_ARG, VALCONTTYPE_STACK
} ValContType;

/** This type is used to specify a container (see the macros CONTAINER_...) */
typedef struct {
  ValContType type_of_container;
  int         which_one,
              addr;
} ValContainer;

void Value_Set_Imm_Char(Value *v, Char c);

void Value_Set_Imm_Int(Value *v, Int i);

void Value_Set_Imm_Real(Value *v, Real r);

#endif /* _VALUE_H */
