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
#include "virtmach.h"

#include "g.h"
#include "i_style.h"

/* NOTE: Since in style.bxh, we define Style = Ptr,
 * Box sees Style as a pointer. We therefore define IStylePtr,
 * which is actually the object Style as seen within the C side.
 */

Task style_begin(VMProgram *vmp) {
  IStylePtr *sp = BOX_VM_CURRENTPTR(vmp, IStylePtr), s;
  int i;
  s = *sp = (IStyle *) malloc(sizeof(IStyle));
  if (s == (IStyle *) NULL) return Failed;
  g_style_new(& s->style, G_STYLE_NONE);
  for(i = 0; i < G_STYLE_ATTR_NUM; i++) s->have[i] = 0;
  return Success;
}

Task style_destroy(VMProgram *vmp) {
  IStylePtr s = BOX_VM_CURRENT(vmp, IStylePtr);
  free(s);
  return Success;
}

Task style_fill_string(VMProgram *vmp) {
  IStylePtr s = BOX_VM_SUB_PARENT(vmp, IStylePtr);
  char *string = BOX_VM_ARG_PTR(vmp, char);
  char *style_strs[] = {"plain", "eo", "clip", "eoclip", (char *) NULL};
  FillStyle styles[] = {DRAW_FILL, DRAW_EOFILL, DRAW_CLIP, DRAW_EOCLIP};
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

Task style_border_color(VMProgram *vmp) {
  IStylePtr s = BOX_VM_SUB_PARENT(vmp, IStylePtr);
  Color *c = BOX_VM_ARG_PTR(vmp, Color);
  g_style_set_bord_color(& s->style, c);
  s->have[G_STYLE_ATTR_BORD_COLOR] = 1;
  return Success;
}

Task style_border_real(VMProgram *vmp) {
  IStylePtr s = BOX_VM_SUB_PARENT(vmp, IStylePtr);
  Real *r = BOX_VM_ARG_PTR(vmp, Real);
  g_style_set_bord_width(& s->style, r);
  s->have[G_STYLE_ATTR_BORD_WIDTH] = 1;
  return Success;
}

Task style_border_join(VMProgram *vmp) {
  IStylePtr s = BOX_VM_SUB_PARENT(vmp, IStylePtr);
  char *join_style = BOX_VM_ARG_PTR(vmp, char);
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

Task style_border_dash_begin(VMProgram *vmp) {
  return Success;
}

Task style_border_dash_real(VMProgram *vmp) {
  return Success;
}

Task style_border_miter_limit(VMProgram *vmp) {
  /*IStylePtr s = BOX_VM_SUB_PARENT(vmp, IStylePtr);
  Real *r = BOX_VM_ARG_PTR(vmp, Real);
  g_style_set_bord_width(& s->style, r);
  s->have[G_STYLE_ATTR_BORD_WIDTH] = 1;*/
  return Success;
}
