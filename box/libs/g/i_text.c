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
#include <assert.h>
#include <string.h>
#include <math.h>

#include "types.h"
#include <box/vm_priv.h>
#include "str.h"

#include "graphic.h"
#include "g.h"
#include "i_window.h"
#include "i_style.h"
#include "i_text.h"
#include "i_gradient.h"

BoxTask window_text_begin(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->text.text = (char *) NULL;
  w->text.font = (char *) NULL;
  w->text.position.x = 0.0; w->text.position.y = 0.0;
  w->text.direction.x = 1.0; w->text.direction.y = 0.0;
  w->text.from.x = 0.5; w->text.from.y = 0.5;
  w->text.font_size = 4.2333333333333333;
  w->text.got.text = 0;
  w->text.got.font = 0;
  w->text.got.font_size = 0;
  w->text.got.color = 0;

  g_style_new(& w->text.style, & w->style);
  return BOXTASK_OK;
}

BoxTask window_text_color(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->text.color = BOX_VM_ARG1(vmp, Color);
  w->text.got.color = 1;
  return BOXTASK_OK;
}

BoxTask window_text_gradient(BoxVMX *vmp) {
  return x_gradient(vmp);
}

static void _place_text(Window *w) {
  BoxPoint *ctr = & w->text.position, *dx = & w->text.direction,
        *from = & w->text.from, dy, right, up;
  BoxReal d, size = w->text.font_size;
  d = sqrt(dx->x*dx->x + dx->y*dx->y);
  if (d > 0) {
    dx->x /= d; dx->y /= d;
  } else {
    g_warning("Bad text direction, using (1, 0).");
    dx->x = 1.0; dx->y = 0.0;
  }
  dx->x *= size; dx->y *= size;
  dy.x = -dx->y; dy.y = dx->x;
  right.x = ctr->x + dx->x; right.y = ctr->y + dx->y;
  up.x    = ctr->x + dy.x;  up.y    = ctr->y + dy.y;
  BoxGWin_Add_Text_Path(w->window, ctr, & right, & up, from, w->text.text);
}

static BoxTask _sentence_end(Window *w, int *wrote_text) {
  int dummy;
  if (wrote_text == (int *) NULL) wrote_text = & dummy;
  *wrote_text = 0;
  if (w->text.got.text && w->text.text != (char *) NULL) {
    if (w->text.got.color) {
      BoxGWin_Set_Fg_Color(w->window, & w->text.color);
      w->text.got.color = 0;
    }

    if (w->text.got.font && w->text.font != (char *) NULL) {
      BoxGWin_Set_Font(w->window, w->text.font);

    } else {
      if (w->text.got.font_size && !w->text.got.font_size)
        g_warning("Ignoring font specification: got its size, "
                  "but not its name!");
    }

    _place_text(w);
    *wrote_text = 1;
  }

  free(w->text.text); w->text.text = (char *) NULL;
  free(w->text.font); w->text.font = (char *) NULL;
  w->text.got.font = 0;
  w->text.got.text = 0;
  return BOXTASK_OK;
}


BoxTask window_text_end(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  int wrote_text;

  BOXTASK( _sentence_end(w, & wrote_text) );
  if (wrote_text)
    (void) BoxGWin_Draw_With_Style(w->window, & w->text.style,
                                   & w->text.default_style, DRAW_WHEN_END);

  g_style_clear(& w->text.style);
  return BOXTASK_OK;
}

BoxTask window_text_pause(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  int wrote_text;

  BOXTASK( _sentence_end(w, & wrote_text) );
  if (wrote_text)
    (void) BoxGWin_Draw_With_Style(w->window, & w->text.style,
                                   & w->text.default_style, DRAW_WHEN_PAUSE);
  return BOXTASK_OK;
}

BoxTask window_text_point(BoxVMX *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->text.position = BOX_VM_ARG1(vmp, BoxPoint);
  return BOXTASK_OK;
}

BoxTask window_text_str(BoxVMX *vm) {
  SUBTYPE_OF_WINDOW(vm, w);
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);
  free(w->text.text);
  w->text.text = BoxStr_To_C_String(s);
  w->text.got.text = 1;
  return BOXTASK_OK;
}

BoxTask window_text_style(BoxVMX *vmp) {
  IStyle *s = BOX_VM_ARG(vmp, IStylePtr);
  SUBTYPE_OF_WINDOW(vmp, w);
  g_style_copy_selected(& w->text.style, & s->style, s->have);
  return BOXTASK_OK;
}

BoxTask window_text_font_str(BoxVMX *vm) {
  SUBTYPE2_OF_WINDOW(vm, w);
  BoxStr *s = BOX_VM_ARG_PTR(vm, BoxStr);
  free(w->text.font);
  w->text.font = strdup(s->ptr);
  w->text.got.font = 1;
  return BOXTASK_OK;
}

BoxTask window_text_font_real(BoxVMX *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  w->text.font_size = BOX_VM_ARG1(vmp, BoxReal);
  w->text.got.font_size = 1;
  return BOXTASK_OK;
}

BoxTask window_text_from_point(BoxVMX *vmp) {
  Window *w = BOX_VM_SUB2_PARENT(vmp, WindowPtr);
  w->text.from = BOX_VM_ARG1(vmp, BoxPoint);
  return BOXTASK_OK;
}
