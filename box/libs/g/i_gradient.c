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
#include <stdio.h>
#include <math.h>

#include "types.h"
#include <box/vm_priv.h>
#include "str.h"
#include "i_gradient.h"
#include "i_window.h"
#include "graphic.h"
#include "g.h"

Task gradient_begin(BoxVMX *vmp) {
  GradientPtr *g_ptr = BOX_VM_THIS_PTR(vmp, GradientPtr);
  Gradient *g;
  g = *g_ptr = (Gradient *) malloc(sizeof(Gradient));
  if (g == (Gradient *) NULL) return BOXTASK_FAILURE;
  if (!buff_create(& g->items, sizeof(ColorGradItem), 8))
    return BOXTASK_FAILURE;
  g->got.type = 0;
  g->got.point1 = 0;
  g->got.point2 = 0;
  g->got.radius1 = 0;
  g->got.radius2 = 0;
  g->got.pause = 0;
  g->got.pos = 0;
  g->this_item.position = -1.0;
  g->gradient.extend = COLOR_GRAD_EXT_PAD;
  return BOXTASK_OK;
}

Task gradient_destroy(BoxVMX *vmp) {
  Gradient *g = BOX_VM_THIS(vmp, GradientPtr);
  buff_free(& g->items);
  free(g);
  return BOXTASK_OK;
}

static Task set_gradient_type(Gradient *g, ColorGradType t) {
  if (g->got.type && g->gradient.type != t) {
    g_error("Cannot change the gradient type!");
    return BOXTASK_FAILURE;
  }
  g->got.type = 1;
  g->gradient.type = t;
  return BOXTASK_OK;
}

Task gradient_string(BoxVMX *vm) {
  Gradient *g = BOX_VM_THIS(vm, GradientPtr);
  BoxStr *box_string = BOX_VM_ARG_PTR(vm, BoxStr);
  const char *ext_str = (char *) box_string->ptr;
  char *ext_styles[] = {"single", "repeated", "repeat",
                        "reflected", "reflect",
                        "padded", "pad", (char *) NULL};
  ColorGradExt es[] = {COLOR_GRAD_EXT_NONE,
                       COLOR_GRAD_EXT_REPEAT, COLOR_GRAD_EXT_REPEAT,
                       COLOR_GRAD_EXT_REFLECT, COLOR_GRAD_EXT_REFLECT,
                       COLOR_GRAD_EXT_PAD, COLOR_GRAD_EXT_PAD};
  int index = g_string_find_in_list(ext_styles, ext_str);
  if (index < 0) {
    printf("Invalid extend style for color gradient. Available styles are: ");
    g_string_list_print(stdout, ext_styles, ", ");
    printf(".\n");
    return BOXTASK_FAILURE;
  }
  g->gradient.extend = es[index];
  return BOXTASK_OK;
}

Task gradient_line_point(BoxVMX *vmp) {
  Gradient *g = BOX_VM_SUB_PARENT(vmp, GradientPtr);
  Point *p = BOX_VM_ARG1_PTR(vmp, Point);
  set_gradient_type(g, COLOR_GRAD_TYPE_LINEAR);
  if (!g->got.point1) {
    g->gradient.point1 = *p;
    g->got.point1 = 1;
    return BOXTASK_OK;

  } else if (!g->got.point2) {
    g->gradient.point2 = *p;
    g->got.point2 = 1;
    return BOXTASK_OK;

  } else {
    g_warning("Gradient.Line takes just two points: ignoring other "
              "given points!");
    return BOXTASK_OK;
  }
}

Task gradient_circle_point(BoxVMX *vmp) {
  Gradient *g = BOX_VM_SUB_PARENT(vmp, GradientPtr);
  Point *p = BOX_VM_ARG1_PTR(vmp, Point);
  set_gradient_type(g, COLOR_GRAD_TYPE_RADIAL);
  if (!g->got.pause) {
    if (g->got.point1) {
      g_warning("Already got the center for the first circle: "
                "ignoring this other value!");
      return BOXTASK_OK;
    }
    g->gradient.point2 = g->gradient.point1 = *p;
    g->got.point1 = 1;

  } else {
    if (g->got.point2) {
      g_warning("Already got the center for the second circle: "
                "ignoring this other value!");
      return BOXTASK_OK;
    }
    g->gradient.point2 = *p;
    g->got.point2 = 1;
  }
  return BOXTASK_OK;
}

Task gradient_circle_real(BoxVMX *vmp) {
  Gradient *g = BOX_VM_SUB_PARENT(vmp, GradientPtr);
  Real r = fabs(BOX_VM_ARG1(vmp, Real));

  set_gradient_type(g, COLOR_GRAD_TYPE_RADIAL);
  if (!g->got.pause) {
    if (g->got.radius1) {
      g_warning("Already got the radius of the first circle: "
                "ignoring this other value!");
      return BOXTASK_OK;
    }
    g->gradient.radius2 = g->gradient.radius1 = r;
    g->got.radius1 = 1;

  } else {
    if (g->got.radius2) {
      g_warning("Already got the radius of the second circle: "
                "ignoring this other value!");
      return BOXTASK_OK;
    }
    g->gradient.radius2 = r;
    g->got.radius2 = 1;
  }
  return BOXTASK_OK;
}

Task gradient_circle_pause(BoxVMX *vmp) {
  Gradient *g = BOX_VM_SUB_PARENT(vmp, GradientPtr);
  if (!(g->got.point1 && g->got.radius1)) {
    g_error("Gradient.Circle[] should get the center and radius "
            "of the first circle, before getting ';'.");
    return BOXTASK_FAILURE;
  }
  g->got.pause = 1;
  return BOXTASK_OK;
}

Task gradient_color(BoxVMX *vmp) {
  Gradient *g = BOX_VM_THIS(vmp, GradientPtr);
  Color *c = BOX_VM_ARG1_PTR(vmp, Color);
  g->this_item.color = *c;
  if (!buff_push(& g->items, & g->this_item)) return BOXTASK_FAILURE;
  g->got.pos = 0;
  g->this_item.position = -1.0;
  return BOXTASK_OK;
}

Task gradient_real(BoxVMX *vmp) {
  Gradient *g = BOX_VM_THIS(vmp, GradientPtr);
  Real r = BOX_VM_ARG1(vmp, Real);
  if (g->got.pos) {
    g_warning("Real@Gradient: You already specified a position "
              "for this Color: ignoring this other value!");
    return BOXTASK_OK;
  }
  if (r < 0.0 || r > 1.0) {
    g_error("Real@Gradient: The color position should be a "
            "real number between 0.0 and 1.0!");
    return BOXTASK_FAILURE;
  }
  g->got.pos = 1;
  g->this_item.position = r;
  return BOXTASK_OK;
}

Task gradient_end(BoxVMX *vmp) {
  Gradient *g = BOX_VM_THIS(vmp, GradientPtr);
  ColorGradItem *cgi;
  Int n = buff_numitems(& g->items);

  if (n < 2) {
    g_error("(])@Gradient: Incomplete gradient specification: "
            "Gradient should get at least two colors!");
    return BOXTASK_FAILURE;
  }

  if (!g->got.type) {
    g_error("(])@Gradient: Incomplete gradient specification: "
            "You should use Gradient.Line or Gradient.Circle!");
    return BOXTASK_FAILURE;
  }

  /* Reference points for the gradient (useful to transform correctly
   * the gradient when applying transformations of the coordinate system)
   */
  g->gradient.ref2 = g->gradient.ref1 = g->gradient.point1;
  g->gradient.ref1.x += 1.0;
  g->gradient.ref2.y += 1.0;

  if (n == 1) {
    cgi = buff_firstitemptr(& g->items, ColorGradItem);
    cgi->position = 0.5;

  } else {
    Int i;
    cgi = buff_lastitemptr(& g->items, ColorGradItem);
    if (cgi->position < 0.0) cgi->position = 1.0;
    cgi = buff_firstitemptr(& g->items, ColorGradItem);
    if (cgi->position < 0.0) cgi->position = 0.0;
    for(i = 1; i < n;) {
      Int a;
      Real pos_a, delta_pos;
      for(; i < n; i++) if (cgi[i].position < 0.0) break;
      a = i;
      for(; i < n; i++) if (cgi[i].position >= 0.0) break;
      pos_a = cgi[a-1].position;
      delta_pos = (cgi[i].position - pos_a)/(i - a + 1);
      for(; a < i; a++)
        cgi[a].position = (pos_a += delta_pos);
    }
  }
  g->gradient.num_items = n;
  g->gradient.items = buff_firstitemptr(& g->items, ColorGradItem);
  return BOXTASK_OK;
}

Task print_gradient(BoxVMX *vmp) {
  Gradient *g = BOX_VM_ARG1(vmp, GradientPtr);
  ColorGradItem *cgi = g->gradient.items;
  Int n = g->gradient.num_items, i;
  FILE *out = stdout;

  fprintf(out, "Gradient[");
  if (g->got.type) {
    if (g->gradient.type == COLOR_GRAD_TYPE_LINEAR) {
      fprintf(out, ".Line[");
      if (g->got.point1)
        fprintf(out, "("SReal", "SReal")",
                g->gradient.point1.x, g->gradient.point1.y);
      if (g->got.point2)
        fprintf(out, ", ("SReal", "SReal")",
                g->gradient.point2.x, g->gradient.point2.y);
      fprintf(out, "]");

    } else {
      fprintf(out, ".Circle[");
      if (g->got.point1)
        fprintf(out, "("SReal", "SReal"), "SReal,
                g->gradient.point1.x, g->gradient.point1.y,
                g->gradient.radius1);
      if (g->got.point2)
        fprintf(out, "; ("SReal", "SReal"), "SReal,
                g->gradient.point2.x, g->gradient.point2.y,
                g->gradient.radius2);
      fprintf(out, "]");
    }
  }

  for(i = 0; i < n; i++) {
    fprintf(out, ", "SReal", Color["SReal", "SReal", "SReal", "SReal"]",
            cgi[i].position, cgi[i].color.r, cgi[i].color.g,
            cgi[i].color.b, cgi[i].color.a);
  }
  fprintf(out, "]");
  return BOXTASK_OK;
}

Task x_gradient(BoxVMX *vmp) {
  Window *w = BOX_VM_SUB_PARENT(vmp, WindowPtr);
  Gradient *g = BOX_VM_ARG1(vmp, GradientPtr);
  BoxGWin_Set_Gradient(w->window, & g->gradient);
  return BOXTASK_OK;
}
