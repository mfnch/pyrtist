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

/* This window is a fake one, just to calculate the bounding box */

/* #define DEBUG */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "error.h"
#include "types.h"
#include "graphic.h"
#include "fig.h"
#include "autoput.h"
#include "cmd.h"
#include "bb.h"

/****************************************************************************
 * C-implementation of the Bounding Box object (BoxGBBox).
 * BoxGBBox corresponds to the BBox in the Box language.
 */

void BoxGBBox_Init(BoxGBBox *bb) {
  bb->num = 0;
  bb->min.x = bb->min.y = bb->max.x = bb->max.y = 0.0;
}

void BoxGBBox_Extend(BoxGBBox *bb, BoxPoint *p) {
  if (bb->num < 1) {
    assert(bb->num == 0);
    bb->min.x = bb->max.x = p->x;
    bb->min.y = bb->max.y = p->y;

  } else {
    if (p->x < bb->min.x) bb->min.x = p->x;
    if (p->y < bb->min.y) bb->min.y = p->y;
    if (p->x > bb->max.x) bb->max.x = p->x;
    if (p->y > bb->max.y) bb->max.y = p->y;
  }
  ++bb->num;
}

void BoxGBBox_Fuse(BoxGBBox *dst, BoxGBBox *src) {
  if (src->num != 0) {
    assert(src->num > 0);
    BoxGBBox_Extend(dst, & src->min);
    BoxGBBox_Extend(dst, & src->max);
    dst->num += src->num - 2;
  }
}

BoxReal BoxGBBox_Get_Volume(BoxGBBox *bb) {
  return (bb->max.x - bb->min.x)*(bb->max.y - bb->min.y);
}

void BoxGBBox_Extend_Margins(BoxGBBox *bb, BoxPoint *margin_min,
                             BoxPoint *margin_max) {
  bb->min.x -= margin_min->x;
  bb->min.y -= margin_min->y;
  bb->max.x += margin_max->x;
  bb->max.y += margin_max->y;
}

void BoxGBBox_Extend_Margin(BoxGBBox *bb, BoxReal margin) {
  bb->min.x -= margin;
  bb->min.y -= margin;
  bb->max.x += margin;
  bb->max.y += margin;
}

/***************************************************************************/

/** BB specific window-data (used to store the bounding box). */
typedef struct {
  int      enabled;
  BoxGBBox bb_local,
           bb_global;

} MyBBWinData;

/** Macro to access BB specific window data. */
#define MY_DATA(w) ((MyBBWinData *) (w)->data)

static void My_Got_Point(BoxGWin *w, BoxReal x, BoxReal y) {
  MyBBWinData *my_data = MY_DATA(w);
  if (my_data->enabled) {
    BoxPoint p = {x, y};
    BoxGBBox_Extend(& my_data->bb_local, & p);
  }
}

static void My_BB_Draw_Path(BoxGWin *w, DrawStyle *style) {
  BoxGBBox *bb_local = & MY_DATA(w)->bb_local;
  BoxGBBox *bb_global = & MY_DATA(w)->bb_global;
  int do_border = (style->bord_width > 0.0);
  Real scale = style->scale;
  if (do_border)
    BoxGBBox_Extend_Margin(bb_local, 0.5*scale*style->bord_width);

  BoxGBBox_Fuse(bb_global, bb_local);
  BoxGBBox_Init(bb_local);
}

static void My_BB_Add_Line_Path(BoxGWin *w, BoxPoint *a, BoxPoint *b) {
#ifdef DEBUG
  printf("line\n");
#endif
  My_Got_Point(w, a->x, a->y);
  My_Got_Point(w, b->x, b->y);
}

static void My_BB_Add_Join_Path(BoxGWin *w, Point *a, Point *b, Point *c) {
#ifdef DEBUG
  printf("cong\n");
#endif
  My_Got_Point(w, a->x, a->y); My_Got_Point(w, b->x, b->y);
  My_Got_Point(w, c->x, c->y); My_Got_Point(w, a->x + c->x - b->x, a->y + c->y - b->y);
}

static void My_BB_Add_Circle_Path(BoxGWin *w, Point *ctr, Point *a, Point *b) {
  Point va, vb;
#ifdef DEBUG
  printf("circle\n");
#endif
  va.x = a->x - ctr->x; va.y = a->y - ctr->y;
  vb.x = b->x - ctr->x; vb.y = b->y - ctr->y;

  My_Got_Point(w, a->x + vb.x, a->y + vb.y);
  My_Got_Point(w, a->x - vb.x, a->y - vb.y);
  My_Got_Point(w, ctr->x - va.x - vb.x, ctr->y - va.y - vb.y);
  My_Got_Point(w, ctr->x - va.x + vb.x, ctr->y - va.y + vb.y);

}

static void My_BB_Add_Text_Path(BoxGWin *w, BoxPoint *ctr, BoxPoint *left,
                                BoxPoint *up, BoxPoint *from,
                                const char *text) {
  My_Got_Point(w, ctr->x, ctr->y);
  My_Got_Point(w, left->x, left->y);
  My_Got_Point(w, up->x, up->y);
}

static void My_BB_Add_Fake_Point(BoxGWin *w, Point *p) {
  My_Got_Point(w, p->x, p->y);
}

/** Used by My_BB_Interpret to pass data to the iterator
 * My_BB_Interpret_Iter. 
 */
typedef struct {
  BoxGWin    *win;
  BoxGWinMap *map;

} MyInterpretData;

static BoxGErr My_BB_Interpret_Iter(BoxGCmd cmd, BoxGCmdSig sig, int num_args,
                                    BoxGCmdArgKind *kinds, void **args,
                                    BoxGCmdArg *aux, void *pass) {
  BoxGWin *w = ((MyInterpretData *) pass)->win;
  BoxGWinMap *map = ((MyInterpretData *) pass)->map;
  int do_include_points = 0;

  switch (cmd) {
  case BOXGCMD_EXT_SET_AUTO_BBOX:
    MY_DATA(w)->enabled = (int) *((BoxInt *) args[0]);
    break;

  case BOXGCMD_EXT_UNSET_BBOX:
    BoxGBBox_Init(& MY_DATA(w)->bb_local);
    BoxGBBox_Init(& MY_DATA(w)->bb_global);
    break;

  default:
    do_include_points = 1;
    break;
  }

  if (do_include_points) {
    int i;
    for (i = 0; i < num_args; i++) {
      void *arg = args[i];
      BoxGCmdArgKind kind = kinds[i];
      switch (kind) {
      case BOXGCMDARGKIND_POINT:
        {
          BoxPoint p;
          BoxGWinMap_Map_Point(map, & p, (BoxPoint *) arg);
          My_Got_Point(w, p.x, p.y);
        }
        break;
      default:
        break;
      }
    }
  }

  return BOXGERR_NO_ERR;
}

static BoxTask My_BB_Interpret(BoxGWin *w, BoxGObj *obj, BoxGWinMap *map) {
  MyInterpretData data;
  data.win = w;
  data.map = map;
  return BoxGCmdIter_Iter(My_BB_Interpret_Iter, obj, & data);
}

/** Set the default methods to the bb window */
static void bb_repair(BoxGWin *w) {
  BoxGWin_Block(w);
  w->draw_path = My_BB_Draw_Path;
  w->add_line_path = My_BB_Add_Line_Path;
  w->add_join_path = My_BB_Add_Join_Path;
  w->add_circle_path = My_BB_Add_Circle_Path;
  w->add_text_path = My_BB_Add_Text_Path;
  w->add_fake_point = My_BB_Add_Fake_Point;
  w->interpret = My_BB_Interpret;
}

int BoxGBBox_Compute(BoxGBBox *bbox, BoxGWin *figure) {
  BoxGWin bb;
  MyBBWinData bb_data;

  /* Set window-specific data (where the BBox will be stored). */
  bb_data.enabled = 1;
  BoxGBBox_Init(& bb_data.bb_global);
  BoxGBBox_Init(& bb_data.bb_local);
  bb.data = & bb_data;

  /* The procedure to handle the window are provided below: */
  bb.quiet = 1;
  bb.repair = bb_repair;
  bb.repair(& bb);
  bb.win_type_str = "bb";
  BoxGWin_Fig_Draw_Fig(& bb, figure);
  BoxGBBox_Fuse(& bb_data.bb_global, & bb_data.bb_local);
  *bbox = bb_data.bb_global;
  return (BoxGBBox_Get_Volume(& bb_data.bb_global) > 0.0);
}
