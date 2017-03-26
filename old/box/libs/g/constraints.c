/****************************************************************************
 * Copyright (C) 2011 by Matteo Franchin                                    *
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

#include <stdio.h>
#include <math.h>
#include <assert.h>

#include <box/types.h>
#include <box/mem.h>
#include <box/str.h>
#include <box/vm_priv.h>

#include "obj.h"
#include "transform.h"
#include "constraints.h"


static void My_Set_GTransform(BoxGTransform *dst, BoxLibGTransform *src) {
  BoxReal sx = src->scale_factors.x,
          sy = src->scale_factors.y,
          s = sqrt(sx*sx + sy*sy);
  dst->translation = src->translation;
  dst->rotation_center = src->rotation_center;
  dst->rotation_angle = src->rotation_angle;
  dst->rotation_cos = cos(src->rotation_angle);
  dst->rotation_sin = sin(src->rotation_angle);
  dst->scale_factor = s;
  dst->scaling_cos = sx/s;
  dst->scaling_sin = sy/s;
  dst->scaling_angle = atan2(dst->scaling_sin, dst->scaling_cos);
}

static void My_Get_GTransform(BoxLibGTransform *dst, BoxGTransform *src) {
  dst->translation = src->translation;
  dst->rotation_center = src->rotation_center;
  dst->rotation_angle = src->rotation_angle;
  dst->scale_factors.x = src->scale_factor*src->scaling_cos;
  dst->scale_factors.y = src->scale_factor*src->scaling_sin;
}

static BoxTask My_Parse_Freedom_String(BoxStr *s, BoxGAllow *allow) {
  char *c_allow = BoxStr_To_C_String(s);
  if (c_allow != NULL) {
    BoxTask t = BoxGAllow_Of_String(allow, c_allow);
    Box_Mem_Free(c_allow);
    return t;

  } else
    return BOXTASK_FAILURE;
}

BoxTask Box_Lib_G_Constraints_At_Transform(BoxVMX *vm) {
  BoxLibGTransform *tr = BoxVMX_Get_Parent_Target(vm);
  BoxLibGConstraints *cs = BoxVMX_Get_Child_Target(vm);
  size_t num_constraints = BoxGObj_Get_Num(cs->constraints), i;
  BoxPoint *srcs, *dsts;
  BoxReal *weights;
  BoxGAllow allowed;

  if (My_Parse_Freedom_String(& cs->freedom, & allowed) != BOXTASK_OK) {
    BoxVM_Set_Fail_Msg(vm->vm,
                       "Error parsing string of allowed transformations");
    return BOXTASK_FAILURE;
  }
    
  srcs = Box_Mem_Safe_Alloc(sizeof(BoxPoint)*num_constraints);
  dsts = Box_Mem_Safe_Alloc(sizeof(BoxPoint)*num_constraints);
  weights = Box_Mem_Safe_Alloc(sizeof(BoxReal)*num_constraints);

  assert(srcs != NULL && dsts != NULL && weights != NULL);

  for (i = 0; i < num_constraints; i++) {
    int got_error = 1;
    BoxGObj *near = BoxGObj_Get(cs->constraints, i);
    if (BoxGObj_Get_Num(near) == 3) {
      BoxPoint   *src = BoxGObj_To(BoxGObj_Get(near, 0), BOXGOBJKIND_POINT),
                 *dst = BoxGObj_To(BoxGObj_Get(near, 1), BOXGOBJKIND_POINT);
      BoxReal *weight = BoxGObj_To(BoxGObj_Get(near, 2), BOXGOBJKIND_REAL);
      if (src != NULL && dst != NULL && weight != NULL) {
        srcs[i] = *src;
        dsts[i] = *dst;
        weights[i] = *weight;
        got_error = 0;
      }
    } 

    if (got_error) {
      BoxVM_Set_Fail_Msg(vm->vm, "Error in obj-ified constraints");
      return BOXTASK_FAILURE;
    }
  }

  {
    BoxGTransform transform;

    My_Set_GTransform(& transform, tr);
    BoxGAutoTransformErr err_number =
      BoxG_Auto_Transform(& transform, srcs, dsts, weights, num_constraints, allowed);

    Box_Mem_Free(srcs);
    Box_Mem_Free(dsts);
    Box_Mem_Free(weights);

    if (err_number) {
      const char *err_msg = BoxGAutoTransformErr_To_String(err_number);
      BoxVM_Set_Fail_Msg(vm->vm, err_msg);
      return BOXTASK_FAILURE;
    }

    My_Get_GTransform(tr, & transform);
    return BOXTASK_OK;
  }
}
