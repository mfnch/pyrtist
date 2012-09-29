/****************************************************************************
 * Copyright (C) 2012 by Matteo Franchin                                    *
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
 * @file coremath.c
 * @brief Declaration of core mathematical functions for Box.
 *
 * This module registers the core mathematical functions for Box.
 */

#include <math.h>

#include <box/types.h>
#include <box/obj.h>
#include <box/core.h>
#include <box/combs.h>
#include <box/exception.h>
#include <box/callable.h>

#include <box/core_priv.h>


#define MY_DEFINE_FN_REAL_AT_REAL(dst_fn, src_fn)               \
  static BoxException *dst_fn(BoxPtr *parent, BoxPtr *child) {  \
    *((BoxReal *) BoxPtr_Get_Target(parent)) =                  \
      src_fn(*((BoxReal *) BoxPtr_Get_Target(child)));          \
    return NULL;                                                \
  }                                                             \

MY_DEFINE_FN_REAL_AT_REAL(My_Math_Cos, cos)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Sin, sin)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Tan, tan)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Asin, asin)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Acos, acos)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Atan, atan)

static BoxException *My_Math_Atan2(BoxPtr *parent, BoxPtr *child) {
  BoxPoint *p = BoxPtr_Get_Target(child);
  *((BoxReal *) BoxPtr_Get_Target(parent)) = atan2(p->y, p->x);
  return NULL;
}

MY_DEFINE_FN_REAL_AT_REAL(My_Math_Exp, exp)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Log, log)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Log10, log10)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Sqrt, sqrt)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Ceil, ceil)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Floor, floor)
MY_DEFINE_FN_REAL_AT_REAL(My_Math_Abs, fabs)

static BoxException *My_Math_Norm(BoxPtr *parent, BoxPtr *child) {
  BoxPoint *p = BoxPtr_Get_Target(child);
  *((BoxReal *) BoxPtr_Get_Target(parent)) = sqrt(p->x*p->x + p->y*p->y);
  return NULL;
}

static BoxException *My_Math_Norm2(BoxPtr *parent, BoxPtr *child) {
  BoxPoint *p = BoxPtr_Get_Target(child);
  *((BoxReal *) BoxPtr_Get_Target(parent)) = p->x*p->x + p->y*p->y;
  return NULL;
}

static BoxException *My_Min_Open(BoxPtr *parent, BoxPtr *child) {
  *((BoxReal *) BoxPtr_Get_Target(parent)) = BOXREAL_MAX;
  return NULL;
}

static BoxException *My_Min_Real(BoxPtr *parent, BoxPtr *child) {
  BoxReal *cp = BoxPtr_Get_Target(parent), c = *cp,
          x = *((BoxReal *) BoxPtr_Get_Target(child));
  *cp = (x < c) ? x : c;
  return NULL;
}

static BoxException *My_Max_Open(BoxPtr *parent, BoxPtr *child) {
  *((BoxReal *) BoxPtr_Get_Target(parent)) = BOXREAL_MIN;
  return NULL;
}

static BoxException *My_Max_Real(BoxPtr *parent, BoxPtr *child) {
  BoxReal *cp = BoxPtr_Get_Target(parent), c = *cp,
          x = *((BoxReal *) BoxPtr_Get_Target(child));
  *cp = (x > c) ? x : c;
  return NULL;
}

static BoxException *My_Vec_Real(BoxPtr *parent, BoxPtr *child) {
  BoxReal *angle = BoxPtr_Get_Target(child);
  BoxPoint *p = BoxPtr_Get_Target(parent);
  p->x = cos(*angle);
  p->y = sin(*angle);
  return NULL;
}

static BoxException *My_Point_At_Ort(BoxPtr *parent, BoxPtr *child) {
  BoxPoint *p_out = BoxPtr_Get_Target(parent);
  BoxPoint *p_in = BoxPtr_Get_Target(child);
  p_out->x = -p_in->y;
  p_out->y = p_in->x;
  return NULL;
}


/* Register basic mathematical functions. */
BoxBool BoxCoreTypes_Init_Math(BoxCoreTypes *ct) {
  struct {
    const char *name;
    BoxType    *out_type,
               *in_type;
    BoxCCall2  fn;

  } *row, table[] = {
    {"Cos", ct->Real_type, ct->Real_type, My_Math_Cos},
    {"Sin", ct->Real_type, ct->Real_type, My_Math_Sin},
    {"Tan", ct->Real_type, ct->Real_type, My_Math_Tan},
    {"Asin", ct->Real_type, ct->Real_type, My_Math_Asin},
    {"Acos", ct->Real_type, ct->Real_type, My_Math_Acos},
    {"Atan", ct->Real_type, ct->Real_type, My_Math_Atan},
    {"Atan2", ct->Real_type, ct->Point_type, My_Math_Atan2},
    {"Exp", ct->Real_type, ct->Real_type, My_Math_Exp},
    {"Log", ct->Real_type, ct->Real_type, My_Math_Log},
    {"Log10", ct->Real_type, ct->Real_type, My_Math_Log10},
    {"Sqrt", ct->Real_type, ct->Real_type, My_Math_Sqrt},
    {"Ceil", ct->Real_type, ct->Real_type, My_Math_Ceil},
    {"Floor", ct->Real_type, ct->Real_type, My_Math_Floor},
    {"Abs", ct->Real_type, ct->Real_type, My_Math_Abs},
    {"Norm", ct->Real_type, ct->Point_type, My_Math_Norm},
    {"Norm2", ct->Real_type, ct->Point_type, My_Math_Norm2},

    {"Vec", ct->Point_type, ct->Real_type, My_Vec_Real},
    {"Ort", ct->Point_type, ct->Point_type, My_Point_At_Ort},
    {NULL, NULL, NULL, NULL}
  };

  for (row = & table[0]; row->name; row++) {
    BoxType *t = BoxType_Create_Ident(BoxType_Link(row->out_type), row->name);

    if (t) {
      BoxCallable *cb = BoxCallable_Create_Undefined(t, row->in_type);
      cb = BoxCallable_Define_From_CCall2(cb, row->fn);
      if (cb) {
        if (!BoxType_Define_Combination(t, BOXCOMBTYPE_AT, row->in_type, cb)) {
          BoxSPtr_Unlink(t);
          return BOXBOOL_FALSE;
        }
      }

      BoxType_Add_Type(ct->root_type, t);

    } else
      return BOXBOOL_FALSE;
  }

  return BOXBOOL_TRUE;
}
