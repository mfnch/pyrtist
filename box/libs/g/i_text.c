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

#include "types.h"
#include "virtmach.h"
#include "graphic.h"
#include "g.h"
#include "i_window.h"
#include "i_text.h"

Task window_text_begin(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  w->text.text = (char *) NULL;
  w->text.position.x = 0.0; w->text.position.y = 0.0;
  w->text.got.text = 0;
  w->text.got.position = 0;
  return Success;
}

Task window_text_end(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  grp_window *cur_win = grp_win;

  grp_win = w->window;
  if (w->text.got.text) {
    if (w->text.got.color) {
      Color *c = & w->text.color;
      grp_rfgcolor(c->r, c->g, c->b);
      w->text.got.color = 0;
    }
    grp_text(& w->text.position, w->text.text);
    grp_rdraw();
    grp_rreset();

  }
  grp_win = cur_win;

  free(w->text.text); w->text.text = (char *) NULL;
  return Success;
}

Task window_text_str(VMProgram *vmp) {
  SUBTYPE_OF_WINDOW(vmp, w);
  free(w->text.text);
  w->text.text = BOX_VM_ARGPTR1(vmp, char);
  w->text.got.text = 1;
  return Success;
}
