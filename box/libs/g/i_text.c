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

#include "types.h"
#include "virtmach.h"
#include "graphic.h"
#include "g.h"
#include "i_window.h"
#include "i_style.h"
#include "i_text.h"

Task window_text_begin(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->text.text = (char *) NULL;
  w->text.font = (char *) NULL;
  w->text.position.x = 0.0; w->text.position.y = 0.0;
  w->text.font_size = 4.2333333333333333;
  w->text.got.text = 0;
  w->text.got.font = 0;
  w->text.got.font_size = 0;
  w->text.got.color = 0;

  g_style_new(& w->text.style, & w->style);
  return Success;
}

static Task _sentence_end(Window *w, int *wrote_text) {
  int dummy;
  if (wrote_text == (int *) NULL) wrote_text = & dummy;
  *wrote_text = 0;
  if (w->text.got.text && w->text.text != (char *) NULL) {
    grp_window *cur_win = grp_win;
    grp_win = w->window;
    if (w->text.got.color) {
      Color *c = & w->text.color;
      grp_rfgcolor(c);
      w->text.got.color = 0;
    }

    if (w->text.got.font && w->text.font != (char *) NULL) {
      grp_font(w->text.font, w->text.font_size);

    } else {
      if (w->text.got.font_size && !w->text.got.font_size)
        g_warning("Ignoring font specification: got its size, "
                  "but not its name!");
    }

    grp_text(& w->text.position, w->text.text);
    *wrote_text = 1;
    grp_win = cur_win;
  }

  free(w->text.text); w->text.text = (char *) NULL;
  free(w->text.font); w->text.font = (char *) NULL;
  w->text.got.font = 0;
  w->text.got.text = 0;
  return Success;
}

Task window_text_end(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  int wrote_text;

  TASK( _sentence_end(w, & wrote_text) );
  if (wrote_text) {
    grp_window *cur_win = grp_win;
    grp_win = w->window;
    (void) g_rdraw(& w->text.style, & w->text.default_style, DRAW_WHEN_END);
    grp_win = cur_win;
  }

  g_style_clear(& w->text.style);
  return Success;
}

Task window_text_pause(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  int wrote_text;

  TASK( _sentence_end(w, & wrote_text) );
  if (wrote_text) {
    grp_window *cur_win = grp_win;
    grp_win = w->window;
    (void) g_rdraw(& w->text.style, & w->text.default_style, DRAW_WHEN_PAUSE);
    grp_win = cur_win;
  }
  return Success;
}

Task window_text_point(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->text.position = BOX_VM_ARG1(vmp, Point);
  return Success;
}

Task window_text_str(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  free(w->text.text);
  w->text.text = strdup(BOX_VM_ARGPTR1(vmp, char));
  w->text.got.text = 1;
  return Success;
}

Task window_text_style(VMProgram *vmp) {
  IStyle *s = BOX_VM_ARG(vmp, IStylePtr);
  SUBTYPE_OF_WINDOW(vmp, w);
  g_style_copy_selected(& w->text.style, & s->style, s->have);
  return Success;
}

Task window_text_font_str(VMProgram *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  free(w->text.font);
  w->text.font = strdup(BOX_VM_ARGPTR1(vmp, char));
  w->text.got.font = 1;
  return Success;
}

Task window_text_font_real(VMProgram *vmp) {
  SUBTYPE2_OF_WINDOW(vmp, w);
  w->text.font_size = BOX_VM_ARG1(vmp, Real);
  w->text.got.font_size = 1;
  return Success;
}
