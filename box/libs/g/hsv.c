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

/* Routines to convert RGB <--> HSV.
 * Based on explanations on http://en.wikipedia.org/wiki/HSL_color_space
 */

#include <assert.h>
#include <math.h>

#include "types.h"
#include "graphic.h"
#include "hsv.h"

#define SET_V_AND_S(maximum, minimum, variation) \
  variation = maximum - minimum; \
  hsv->v = maximum; \
  hsv->s = variation/maximum

#define TRUNCATE(x) ((x) > 0.0 ? floor(x): -floor(-(x)))

void HSV_Trunc(HSV *hsv) {
  hsv->h += 360.0*TRUNCATE(hsv->h/360.0);
  if (hsv->s < 0.0) hsv->s = 0.0;
  if (hsv->s > 1.0) hsv->s = 1.0;
  if (hsv->v < 0.0) hsv->v = 0.0;
  if (hsv->v > 1.0) hsv->v = 1.0;
}

void HSV_From_Color(HSV *hsv, Color *c) {
  BoxReal r = c->r, g = c->g, b = c->b;
  BoxReal h, var;

  hsv->a = c->a;
  switch((r >= g) | (g >= b) << 1 | (b >= r) << 2) {
  case 0:
    assert(0);
  case 1: /* r > b > g  */
    SET_V_AND_S(r, g, var);
    h = 60.0*(g - b)/var;
    hsv->h = (h >= 0.0) ? h : h + 360.0;
    return;
  case 2: /* g > r > b */
    SET_V_AND_S(g, b, var);
    hsv->h = 60.0*(b - r)/var + 120.0;
    return;
  case 3: /* r >= g >= b, r > b */
    SET_V_AND_S(r, b, var);
    h = 60.0*(g - b)/var;
    hsv->h = (h >= 0.0) ? h : h + 360.0;
    return;
  case 4: /* b > g > r */
    SET_V_AND_S(b, r, var);
    hsv->h = 60.0*(r - g)/var + 240.0;
    return;
  case 5: /* b >= r >= g, b > g */
    SET_V_AND_S(b, g, var);
    hsv->h = 60.0*(r - g)/var + 240.0;
    return;
  case 6: /* g >= b >= r, g > r */
    SET_V_AND_S(g, r, var);
    hsv->h = 60.0*(b - r)/var + 120.0;
    return;
  case 7: /* r == g == b */
    hsv->h = hsv->s = 0.0; hsv->v = r;
    return;
  default:
    assert(0);
  }
}

void HSV_To_Color(Color *c, HSV *hsv) {
  BoxReal h = hsv->h/60.0, s = hsv->s, v = hsv->v;
  BoxInt hi;

  c->a = hsv->a;
  hi = (BoxInt) TRUNCATE(h);
  h -= hi;
#define CALC_P() (v*(1.0 - s))
#define CALC_Q() (v*(1.0 - h*s))
#define CALC_T() (v*(1.0 - (1.0 - h)*s))
  switch(hi % 6) {
  case 0: c->r = v;        c->g = CALC_T(); c->b = CALC_P(); return;
  case 1: c->r = CALC_Q(); c->g = v;        c->b = CALC_P(); return;
  case 2: c->r = CALC_P(); c->g = v;        c->b = CALC_T(); return;
  case 3: c->r = CALC_P(); c->g = CALC_Q(); c->b = v;        return;
  case 4: c->r = CALC_T(); c->g = CALC_P(); c->b = v;        return;
  case 5: c->r = v;        c->g = CALC_P(); c->b = CALC_Q(); return;
  }
}
