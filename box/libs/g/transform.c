/****************************************************************************
 * Copyright (C) 2008-2011 by Matteo Franchin                               *
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

/* This file implements an algorithm to automatically compute a transformation
 * matrix given a specification of allowed transformations and constraints.
 * The constraints are given as triples (source_point, target_point, weight),
 * where:
 *
 *   'source_point': is a point in the source figure (the one which is
 *                   being placed),
 *   'target_point': is a point in the destination figure,
 *         'weight': is a positive real number which decides how important the
 *                   constraint is.
 *
 * The algorithm will translate, rotate, scale or/and invert the source figure
 * (depending on what transformations are allowed) to make sure that all the
 * source points get as close as possible to the corresponding target points,
 * weighting each constraint by the appropriate weight.
 * The algorithm is non-iterative, it is an exact analytical solution to the
 * problem above.
 */

#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include <box/types.h>

#include "error.h"
#include "graphic.h"
#include "transform.h"

/*#define DEBUG*/


const char *BoxGAutoTransformErr_To_String(BoxGAutoTransformErr n) {
  switch (n) {
  case BOXGAUTOTRANSFORMERR_NO_ERR:
    return NULL;
  case BOXGAUTOTRANSFORMERR_NOT_ENOUGH_POINTS:
    return "Need more points for the required transformation.";
  case BOXGAUTOTRANSFORMERR_ZERO_WEIGHTS:
    return "Error: weights sum up to zero.";
  case BOXGAUTOTRANSFORMERR_NOT_IMPLEMENTED:
    return "Case not implemented, yet.";
  }

  return "Unknown error.";
}

/** From an initial transformation 'transform', 'n' constraints (provided in
 * 'src', 'dst' and 'weight') and a set of allowed transformations,
 * 'allowed_transformation', finds the one which best satisfies the
 * constraints. In particular, the objective is to minimise the function:
 *
 *   f(q) = sum_i {weight[i]*(T(q) src[i] - dst[i])^2} / sum_i {weight[i]}
 *
 * where T[q] is a transformation parametrized by q, which is a vector made
 * by all the available degrees of freedom. The objective is then to determine
 * 'q' and T(q). T(q) is then stored inside 'transform'.
 */
BoxGAutoTransformErr
BoxG_Auto_Transform(BoxGTransform *transform,
                    BoxPoint *src, BoxPoint *dst, BoxReal *weight, int n,
		    BoxGAllow allowed_transforms) {
  int i;
  BoxReal weights_sum;

  if (n < 1)
    return BOXGAUTOTRANSFORMERR_NOT_ENOUGH_POINTS;

#ifdef DEBUG
  for (i = 0; i < n; i++) {
    printf("src[%d] = (%g, %g)\t<--near[%g]-->\tdst[%d] = (%g, %g)\n...",
	   i, src[i].x, src[i].y, weight[i], i, dst[i].x, dst[i].y);
  }
#endif

  /* Compute the sum of the weights */
  weights_sum = 0.0;
  for (i = 0; i < n; i++)
    weights_sum += weight[i];

  if (weights_sum == 0.0)
    return BOXGAUTOTRANSFORMERR_ZERO_WEIGHTS;

  if (allowed_transforms & BOXGALLOW_TRANSLATE) {
    int allowed_translations = allowed_transforms & BOXGALLOW_TRANSLATE;

    /* One or both of the translational degrees of freedom have to be
     * handled automatically: we automatically determine Q and T.
     */

    /* Compute weighted averages of F and R: F_avg and R_avg */
    BoxPoint src_avg = (BoxPoint) {0.0, 0.0},
             dst_avg = (BoxPoint) {0.0, 0.0};

    for (i = 0; i < n; i++) {
      BoxReal w = weight[i];
      src_avg.x += src[i].x * w;
      src_avg.y += src[i].y * w;
      dst_avg.x += dst[i].x * w;
      dst_avg.y += dst[i].y * w;
    }

    src_avg.x /= weights_sum;
    src_avg.y /= weights_sum;
    dst_avg.x /= weights_sum;
    dst_avg.y /= weights_sum;

    /* Set the rotation center */
    transform->rotation_center = src_avg;

    /* Compute the translation vector */
    if (allowed_transforms == BOXGALLOW_TRANSLATE_X) {
      /* x: automatic, y: manual */
      transform->translation.x = dst_avg.x - src_avg.x;
      transform->translation.y -= src_avg.y;

    } else if (allowed_transforms == BOXGALLOW_TRANSLATE_Y) {
      /* x: manual, y: automatic */
      transform->translation.x -= src_avg.x;
      transform->translation.y = dst_avg.y - src_avg.y;

    } else {
      /* x, y: automatic */
      transform->translation.x = dst_avg.x - src_avg.x;
      transform->translation.y = dst_avg.y - src_avg.y;
    }
  }

  /* Exit if translating was the only thing we had to do */
  if ((allowed_transforms & BOXGALLOW_ALL & ~BOXGALLOW_TRANSLATE) == 0)
    return 0;

  else {	
    /* Now we compute all the averages we need in the next steps */
    BoxPoint g_avg, g2_avg, i_avg, j_avg;
    BoxPoint U = 
      (BoxPoint) {transform->translation.x + transform->rotation_center.x,
		  transform->translation.y + transform->rotation_center.y};

    for (i = 0; i < n; i++) {
      BoxReal w = weight[i];
      BoxPoint
	 g = (BoxPoint) {src[i].x - transform->rotation_center.x,
		         src[i].y - transform->rotation_center.y},

	wg = (BoxPoint) {w * g.x,
			 w * g.y},

         s = (BoxPoint) {dst[i].x - U.x,
			 dst[i].y - U.y};

      g2_avg.x += wg.x * g.x;
      g2_avg.y += wg.y * g.y;

      i_avg.x += wg.x * s.x;
      i_avg.y += wg.y * s.y;

      j_avg.x += wg.x * s.y;
      j_avg.y += wg.y * s.x;
    }

    i_avg.x /= weights_sum; /*(w = wsum * sg2x);*/
    j_avg.x /= weights_sum;
    i_avg.y /= weights_sum; /*(w = wsum * sg2y);*/
    j_avg.y = -(j_avg.y/weights_sum);
    g2_avg.x /= weights_sum;
    g2_avg.y /= weights_sum;
    g_avg.x = sqrt(g2_avg.x);
    g_avg.y = sqrt(g2_avg.y);

#ifdef DEBUG
    printf("g2x = %g\tg2y = %g\n...", g2_avg.x, g2_avg.y);
    printf("sg2x = %g\tsg2y = %g\n...", g_avg.x, g_avg.y);
    printf("ix = %g\tiy = %g\n...", i_avg.x, i_avg.y);
    printf("jx = %g\tjy = %g\n...", j_avg.x, j_avg.y);
#endif

    /* We now have three different cases to deal with */
    if ((allowed_transforms & BOXGALLOW_DEFORM) == 0) {
      /* CASE 1: Proportions are manually fixed. */
      BoxReal cos_tau = transform->scaling_cos,
              sin_tau = transform->scaling_sin;

      BoxReal A = i_avg.x * cos_tau + i_avg.y * sin_tau,
              B = j_avg.x * cos_tau + j_avg.y * sin_tau;

      if (allowed_transforms & BOXGALLOW_ROTATE) {
	/* Determino l'angolo di rotazione, se e' selezionata
	 * la rotazione automatica!
	 */
	BoxReal modAB = sqrt(A*A + B*B);
	transform->rotation_cos = A/modAB,
	transform->rotation_sin = B/modAB;
	transform->rotation_angle =
          atan2(transform->rotation_sin, transform->rotation_sin);

      } else {
	/* Se la rotazione e' manuale, conosco theta, ma non sin e cos! */
	transform->rotation_cos = cos(transform->rotation_angle);
	transform->rotation_sin = sin(transform->rotation_angle);
      }

      /* What is missing is just the scaling factor! */
      if (allowed_transforms & BOXGALLOW_SCALE) {

	BoxReal C = cos_tau*cos_tau*g2_avg.x + sin_tau*sin_tau*g2_avg.y;
	transform->scale_factor = 
          (A*transform->rotation_cos + B*transform->rotation_sin)/C;
      }
      
    } else if (allowed_transforms & BOXGALLOW_ANISOTROPIC) {
      /* CASE 2: Proportions have to be determined automatically. */
      return BOXGAUTOTRANSFORMERR_NOT_IMPLEMENTED;
      
    } else if (allowed_transforms & BOXGALLOW_INVERT) {
      /* CASE 3: Proportions are fixed, but can invert. */
      return BOXGAUTOTRANSFORMERR_NOT_IMPLEMENTED;      
    }
  }

  return 1;
}

BoxTask BoxGAllow_Of_String(BoxGAllow *allow, const char *string) {
  char p;
  enum {NORMAL, WAIT_COMPONENT} state = NORMAL;
  BoxGAllow my_allow = 0;
  int do_set = 1;

  p = tolower(*string);
  if (p == ' ')
    my_allow = *allow;

  for (;; p = tolower(*string)) {
    BoxGAllow queued_flag = 0;

    switch (state) {
    case NORMAL:
      switch (p) {
      case 't':
        state = WAIT_COMPONENT; break;
      case 'r':
        queued_flag = BOXGALLOW_ROTATE; break;
      case 's':
        queued_flag = BOXGALLOW_SCALE; break;
      case 'a':
        queued_flag = BOXGALLOW_ANISOTROPIC; break;
      case 'i':
        queued_flag = BOXGALLOW_INVERT; break;
      case ' ':
        break;
      case '+':
        do_set = 1; break;
      case '-':
        do_set = 0; break;
      case '\0':
        *allow = my_allow;
        return BOXTASK_OK;
      default:
        return BOXTASK_FAILURE;
      }
      ++string;
      break;

    case WAIT_COMPONENT:
      switch (p) {
      case 'x':
        queued_flag = BOXGALLOW_TRANSLATE_X;
        ++string; 
        break;
      case 'y':
        queued_flag = BOXGALLOW_TRANSLATE_Y;
        ++string;
        break;
      default:
        queued_flag = BOXGALLOW_TRANSLATE;
        break;
      }
      state = NORMAL;
      break;
    }

    /* Set or reset the queued flag */
    if (do_set)
      my_allow |= queued_flag;
    else
      my_allow &= ~queued_flag;
  }

  abort(); /* We should never get here! */
  return BOXTASK_FAILURE;
}

#if 0
/* DESCRIZIONE: Setta i parametri della trasformazione prima di avviare
 *  il calcolo automatico con aput_autoput.
 */
void aput_set(Point *rot_center, Point *trsl_vect,
              Real *rot_angle, Real *scale_x, Real *scale_y ) {
  register Real sx, sy;

  Qx = rot_center->x;
  Qy = rot_center->y;
  Tx = trsl_vect->x;
  Ty = trsl_vect->y;

  theta = *rot_angle;

  sx = *scale_x; sy = *scale_y;
  s = sqrt( sx*sx + sy*sy );
  cos_tau = sx/s; sin_tau = sy/s;

  return;
}

/* DESCRIZIONE: Preleva i parametri calcolati in modo automatico da aput_autoput
 */
void aput_get(Point *rot_center, Point *trsl_vect,
              Real *rot_angle, Real *scale_x, Real *scale_y ) {
  rot_center->x = Qx;
  rot_center->y = Qy;
  trsl_vect->x = Tx;
  trsl_vect->y = Ty;

  *rot_angle = theta;
  *scale_x = s * cos_tau;
  *scale_y = s * sin_tau;
}
#endif
