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


#ifndef _BOX_LIBG_TRANSFORM_H
#  define _BOX_LIBG_TRANSFORM_H

#  include <box/types.h>
#  include "graphic.h"

/** Flags used to specify which transformations are allowed. */
typedef enum {
  BOXGALLOW_TRANSLATE_X = 0x1,  /**< Allow automatic translation along x */
  BOXGALLOW_TRANSLATE_Y = 0x2,  /**< Allow automatic translation along y */
  BOXGALLOW_TRANSLATE   = 0x3,  /**< Allow automatic translation in x, y */
  BOXGALLOW_ROTATE      = 0x4,  /**< Allow automatic rotation */
  BOXGALLOW_SCALE       = 0x8,  /**< Allow automatic scaling */
  BOXGALLOW_ANISOTROPIC = 0x10, /**< Allow different scaling in x and y */
  BOXGALLOW_INVERT      = 0x20, /**< Allow mirroring the figure */
  BOXGALLOW_DEFORM      = 0x30, /**< Allow mirroring and anisotropic scaling */
  BOXGALLOW_ALL         = 0x3f  /**< All flags or-ed together */

} BoxGAllow;

/** Structure containing a transformation in terms of translation vector,
 * rotation center and angle and scaling factors. From this structure
 * a transformation matrix can be obtained.
 */
typedef struct {
  BoxPoint translation,
           rotation_center;
  BoxReal  rotation_angle,
           rotation_cos,
           rotation_sin, 
           scale_factor,
           scaling_angle,
           scaling_cos,
           scaling_sin;

} BoxGTransform;

#if 0
void aput_identity_matrix(Real *matrix);
void aput_get(Point *rot_center, Point *trsl_vect,
              Real *rot_angle, Real *scale_x, Real *scale_y );
void aput_set(Point *rot_center, Point *trsl_vect,
              Real *rot_angle, Real *scale_x, Real *scale_y );
int aput_autoput(Point *F, Point *R, Real *weight, int n, int needed);
int aput_allow(const char *permissions, int *needed);
#endif

#endif /*_BOX_LIBG_TRANSFORM_H */
