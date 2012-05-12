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

#include <stdlib.h>

#include "types.h"
#include <box/vm_private.h>
#include "str.h"

#include "g.h"
#include "i_style.h"

/* NOTE: Since in style.bxh, we define Style = Ptr,
 * Box sees Style as a pointer. We therefore define IStylePtr,
 * which is actually the object Style as seen within the C side.
 */

Task style_begin(BoxVM *vmp) {
  IStylePtr *sp = BOX_VM_THIS_PTR(vmp, IStylePtr), s;
  int i;
  s = *sp = (IStyle *) malloc(sizeof(IStyle));
  if (s == (IStyle *) NULL) return Failed;
  g_style_new(& s->style, G_STYLE_NONE);
  for(i = 0; i < G_STYLE_ATTR_NUM; i++) s->have[i] = 0;
  if (!buff_create(& s->dashes, sizeof(Real), 8)) return Failed;
  s->dash_offset_contest = -1;
  s->dash_offset = 0.0;
  return Success;
}

Task style_destroy(BoxVM *vmp) {
  IStylePtr s = BOX_VM_THIS(vmp, IStylePtr);
  g_style_clear(& s->style);
  buff_free(& s->dashes);
  free(s);
  return Success;
}

Task style_fill_string(BoxVM *vm) {
  IStylePtr s = BOX_VM_SUB_PARENT(vm, IStylePtr);
  BoxStr *box_string = BOX_VM_ARG_PTR(vm, BoxStr);

  const char *string = (char *) box_string->ptr;
  char *style_strs[] = {"void", "plain", "eo",
                        "clip", "eoclip", (char *) NULL};
  FillStyle styles[] = {FILLSTYLE_VOID, FILLSTYLE_PLAIN, FILLSTYLE_EO,
                        FILLSTYLE_CLIP, FILLSTYLE_EOCLIP};
  char *unset_strs[] = {"unset", "-", (char *) NULL};
  char *when_strs[] = {";", "]", (char *) NULL};
  DrawWhen  whens[] = {DRAW_WHEN_PAUSE, DRAW_WHEN_END};
  int index;

  if (g_string_find_in_list(unset_strs, string) >= 0) {
    g_style_unset_draw_when(& s->style);
    g_style_unset_fill_style(& s->style);
    return Success;
  }

  index = g_string_find_in_list(style_strs, string);
  if (index >= 0) {
    g_style_set_fill_style(& s->style, styles[index]);
    s->have[G_STYLE_ATTR_FILL_STYLE] = 1;
    return Success;
  }

  index = g_string_find_in_list(when_strs, string);
  if (index >= 0) {
    g_style_set_draw_when(& s->style, whens[index]);
    s->have[G_STYLE_ATTR_DRAW_WHEN] = 1;
    return Success;
  }

  /* Unrecognized string! */
  g_warning("Unknown fill style string in Style.Fill: doing nothing!");
  return Success;
}

Task style_border_color(BoxVM *vmp) {
  IStylePtr s = BOX_VM_SUB_PARENT(vmp, IStylePtr);
  Color *c = BOX_VM_ARG_PTR(vmp, Color);
  g_style_set_bord_color(& s->style, c);
  s->have[G_STYLE_ATTR_BORD_COLOR] = 1;
  return Success;
}

Task style_border_real(BoxVM *vmp) {
  IStylePtr s = BOX_VM_SUB_PARENT(vmp, IStylePtr);
  Real *r = BOX_VM_ARG_PTR(vmp, Real);
  g_style_set_bord_width(& s->style, r);
  s->have[G_STYLE_ATTR_BORD_WIDTH] = 1;
  return Success;
}

Task style_border_join(BoxVM *vm) {
  IStylePtr s = BOX_VM_SUB_PARENT(vm, IStylePtr);
  BoxStr *box_string = BOX_VM_ARG_PTR(vm, BoxStr);
  const char *join_style = (char *) box_string->ptr;
  char *join_styles[] = {"miter", "round", "bevel", (char *) NULL};
  JoinStyle js[] = {JOIN_STYLE_MITER, JOIN_STYLE_ROUND, JOIN_STYLE_BEVEL};
  int index = g_string_find_in_list(join_styles, join_style);
  if (index < 0) {
    printf("Invalid join style. Available styles are: ");
    g_string_list_print(stdout, join_styles, ", ");
    printf(".\n");
    return Failed;
  }
  g_style_set_bord_join_style(& s->style, js + index);
  s->have[G_STYLE_ATTR_BORD_JOIN_STYLE] = 1;
  return Success;
}

Task style_border_dash_begin(BoxVM *vmp) {
  IStylePtr s = BOX_VM_SUB2_PARENT(vmp, IStylePtr);
  s->dash_offset_contest = -1;
  return buff_clear(& s->dashes) ? Success : Failed;
}

Task style_border_dash_pause(BoxVM *vmp) {
  IStylePtr s = BOX_VM_SUB2_PARENT(vmp, IStylePtr);
  s->dash_offset_contest = 0;
  s->dash_offset = 0.0;
  return Success;
}

Task style_border_dash_real(BoxVM *vmp) {
  IStylePtr s = BOX_VM_SUB2_PARENT(vmp, IStylePtr);
  Real *r = BOX_VM_ARG_PTR(vmp, Real);
  switch(s->dash_offset_contest) {
  case -1: return buff_push(& s->dashes, r) ? Success : Failed;
  case  0: s->dash_offset = *r; ++s->dash_offset_contest; return Success;
  default:
    g_warning("Style.Border.Dash: Dash offset already specified: "
              "ignoring the second value!");
    return Success;
  }
}

Task style_border_dash_end(BoxVM *vmp) {
  IStylePtr s = BOX_VM_SUB2_PARENT(vmp, IStylePtr);
  Int n = buff_numitems(& s->dashes);
  Real *dashes = buff_firstitemptr(& s->dashes, Real);
  g_style_set_bord_dashes(& s->style, n, dashes, s->dash_offset);
  s->have[G_STYLE_ATTR_BORD_DASHES] = 1;
  return Success;
}

Task style_border_miter_limit(BoxVM *vmp) {
  IStylePtr s = BOX_VM_SUB2_PARENT(vmp, IStylePtr);
  Real *r = BOX_VM_ARG_PTR(vmp, Real);
  g_style_set_bord_miter_limit(& s->style, r);
  s->have[G_STYLE_ATTR_BORD_MITER_LIMIT] = 1;
  return Success;
}

Task style_border_cap_string(BoxVM *vm) {
  IStylePtr s = BOX_VM_SUB2_PARENT(vm, IStylePtr);
  BoxStr *box_string = BOX_VM_ARG_PTR(vm, BoxStr);
  const char *cap_str = (char *) box_string->ptr;
  char *cap_styles[] = {"butt", "round", "square", (char *) NULL};
  CapStyle cs[] = {CAP_STYLE_BUTT, CAP_STYLE_ROUND, CAP_STYLE_SQUARE};

  int index = g_string_find_in_list(cap_styles, cap_str);
  if (index < 0) {
    printf("Invalid cap style. Available styles are: ");
    g_string_list_print(stdout, cap_styles, ", ");
    printf(".\n");
    return Failed;
  }
  g_style_set_bord_cap(& s->style, cs + index);
  s->have[G_STYLE_ATTR_BORD_CAP] = 1;
  return Success;
}
